#!/bin/bash
set -e

CURRENT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

function init_variables() {
    print_help_if_needed
    readonly RESOURCES_DIR="${CURRENT_DIR}/resources"
    readonly POSTPROCESS_DIR="/usr/lib/hailo-post-processes"

    readonly CROPING_ALGORITHMS_DIR="$POSTPROCESS_DIR/cropping_algorithms"
    readonly DEFAULT_DETECTION_POSTPROCESS_SO="$POSTPROCESS_DIR/libface_detection_post.so"
    readonly DEFAULT_DETECTION_VIDEO_SOURCE="$RESOURCES_DIR/face_detection.mp4"
    readonly DEFAULT_HEF_PATH="$RESOURCES_DIR/joined_lightface_slim_tddfa_mobilenet_v1.hef"
    readonly DEFAULT_LANDMARKS_POSTPROCESS_SO="$POSTPROCESS_DIR/libfacial_landmarks_post.so"
    readonly DEFAULT_CROP_SO="$CROPING_ALGORITHMS_DIR/lib3ddfa.so"
    readonly DEFAULT_VDEVICE_KEY="1"

    video_sink_element="autovideosink"
    hef_path=$DEFAULT_HEF_PATH
    detection_postprocess_so=$DEFAULT_DETECTION_POSTPROCESS_SO
    landmarks_postprocess_so=$DEFAULT_LANDMARKS_POSTPROCESS_SO
    input_source=$DEFAULT_DETECTION_VIDEO_SOURCE
    crop_so=$DEFAULT_CROP_SO

    internal_offset=false
    print_gst_launch_only=false
    additional_parameters=""
    sync_pipeline=false
    width=1280
    height=720
}

function print_usage() {
    echo "Sanity hailo pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                  Show this help"
    echo "  -i INPUT --input INPUT  Set the input source - Could be path to video file or a video device path (default is $input_source)"
    echo "  --show-fps              Print fps"
    echo "  --print-gst-launch      Print the ready gst-launch command without running it"
    exit 0
}

function print_help_if_needed() {
    while test $# -gt 0; do
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage
        fi

        shift
    done
}

function parse_args() {
    while test $# -gt 0; do
        if [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
        elif [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            additional_parameters="-v | grep hailo_display"
        elif [ "$1" = "--input" ] || [ "$1" == "-i" ]; then
            input_source="$2"
            shift
        else
            echo "Received invalid argument: $1. See expected arguments below:"
            print_usage
            exit 1
        fi

        shift
    done
}

init_variables $@
parse_args $@

opengl_covnert="glupload ! \
                queue max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
                glcolorscale qos=false ! \
                queue max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
                glcolorconvert qos=false ! \
                queue max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
                gldownload ! video/x-raw,pixel-aspect-ratio=1/1,format=RGBA ! \
                queue max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
                videoconvert name=pre_hailonet_videoconvert n-threads=2 qos=false ! video/x-raw, format=RGB, width=$width, height=$height"

# If the video provided is from a camera
if [[ $input_source =~ "/dev/video" ]]; then
    source_element="v4l2src device=$input_source name=src_0 ! video/x-raw, format=YUY2, width=$width, height=$height, framerate=30/1 ! videoflip video-direction=horiz ! videoconvert qos=false n-threads=4"
    internal_offset=false
else
    source_element="filesrc location=$input_source name=src_0 ! decodebin ! queue ! $opengl_covnert"
    internal_offset=true
fi

FACE_DETECTION_PIPELINE="videoscale qos=false n-threads=3 ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailonet net-name=joined_lightface_slim_tddfa_mobilenet_v1/lightface_slim \
    hef-path=$hef_path is-active=true scheduling-algorithm=0 vdevice-key=$DEFAULT_VDEVICE_KEY ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$detection_postprocess_so function-name=lightface qos=false ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0"

FACIAL_LANDMARKS_PIPELINE="queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailonet net-name=joined_lightface_slim_tddfa_mobilenet_v1/tddfa_mobilenet_v1 \
    hef-path=$hef_path is-active=true scheduling-algorithm=0 vdevice-key=$DEFAULT_VDEVICE_KEY ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailofilter function-name=facial_landmarks_merged so-path=$landmarks_postprocess_so qos=false ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0"

PIPELINE="gst-launch-1.0 $source_element ! queue ! tee name=t hailomuxer name=hmux \
    t. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! hmux. \
    t. ! $FACE_DETECTION_PIPELINE ! hmux. \
    hmux. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailocropper so-path=$crop_so function-name=create_crops internal-offset=$internal_offset name=cropper \
    hailoaggregator name=agg \
    cropper. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! agg. \
    cropper. ! $FACIAL_LANDMARKS_PIPELINE ! agg. \
    agg. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay qos=false ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! videoconvert ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=false text-overlay=false $additional_parameters"

echo "Running $network_name"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
