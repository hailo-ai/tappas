#!/bin/bash
set -e

function init_variables() {
    readonly DEFAULT_MAX_CAMERA_WIDTH="1920"
    readonly DEFAULT_MAX_CAMERA_HEGIHT="1080"
    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/cascading_networks/resources"
    readonly DEFAULT_DETECTION_VIDEO_SOURCE="$RESOURCES_DIR/face_detection.mp4"

    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh

    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes"
    readonly CROPING_ALGORITHMS_DIR="$POSTPROCESS_DIR/cropping_algorithms"
    readonly DEFAULT_DETECTION_POSTPROCESS_SO="$POSTPROCESS_DIR/libface_detection_post.so"
    readonly DEFAULT_HEF_PATH="$RESOURCES_DIR/joined_lightface_slim_tddfa_mobilenet_v1.hef"
    readonly DEFAULT_LANDMARKS_POSTPROCESS_SO="$POSTPROCESS_DIR/libfacial_landmarks_post.so"
    readonly DEFAULT_CROP_SO="$CROPING_ALGORITHMS_DIR/lib3ddfa.so"
    readonly DEFAULT_VDEVICE_KEY="1"

    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
    hef_path=$DEFAULT_HEF_PATH
    detection_postprocess_so=$DEFAULT_DETECTION_POSTPROCESS_SO
    landmarks_postprocess_so=$DEFAULT_LANDMARKS_POSTPROCESS_SO
    input_source=$DEFAULT_DETECTION_VIDEO_SOURCE
    crop_so=$DEFAULT_CROP_SO

    internal_offset=false
    print_gst_launch_only=false
    additional_parameters=""

    max_camera_width=$DEFAULT_MAX_CAMERA_WIDTH
    max_camera_height=$DEFAULT_MAX_CAMERA_HEGIHT
}

function print_usage() {
    echo "Sanity hailo pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                                  Show this help"
    echo "  -i INPUT --input INPUT                  Set the input source (default $DEFAULT_DETECTION_VIDEO_SOURCE)"
    echo "  --show-fps                              Print fps"
    echo "  --max-camera-resolution WIDTHxHEIGHT    The maximun input resolution from camera as an input (default ${DEFAULT_MAX_CAMERA_WIDTH}x${DEFAULT_MAX_CAMERA_HEGIHT})"
    echo "  --print-gst-launch                      Print the ready gst-launch command without running it"
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
        elif [ "$1" = "--max-camera-resolution" ]; then
            shift
            max_camera_width=$(echo $1 | awk -F'x' '{print $1}')
            max_camera_height=$(echo $1 | awk -F'x' '{print $2}')
            echo "The maximum resolution to use for camera as an input is $1"
        else
            echo "Received invalid argument: $1. See expected arguments below:"
            print_usage
            exit 1
        fi

        shift
    done
}

function find_max_resolution() {
    resolutions=($(ffmpeg -f video4linux2 -list_formats all -i $input_source 2>&1 | grep JPEG | grep -oE '\b[0-9]+x[0-9]+\b' | tr '\n' ' '))
    max_resolution="0x0"

    for res in "${resolutions[@]}"
    do
        if [[ "$res" =~ ^([0-9]+)x([0-9]+)$ ]]; then
            if (( ${BASH_REMATCH[1]} <= $max_camera_width && ${BASH_REMATCH[2]} <= $max_camera_height )); then
              if (( ${BASH_REMATCH[1]} >= ${max_resolution%x*} || ${BASH_REMATCH[2]} >= ${max_resolution#*x} )); then
                max_resolution="$res"
              fi
            fi
        fi
    done

    width=$(echo $max_resolution | awk -F'x' '{print $1}')
    height=$(echo $max_resolution | awk -F'x' '{print $2}')
}

init_variables $@
parse_args $@

# If the video provided is from a camera
if [[ $input_source =~ "/dev/video" ]]; then
    find_max_resolution
    
    source_element="v4l2src device=$input_source name=src_0 ! image/jpeg, width=$width, height=$height !  \
                    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
                    decodebin ! \
                    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
                    videoflip video-direction=horiz ! videoconvert qos=false"
    internal_offset=false
else
    source_element="filesrc location=$input_source name=src_0 ! decodebin ! videoconvert qos=false"
    internal_offset=true
fi

FACE_DETECTION_PIPELINE="videoscale qos=false ! \
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

PIPELINE="gst-launch-1.0 $source_element ! tee name=t hailomuxer name=hmux \
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
