#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh

    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes"
    readonly CROPING_ALGORITHMS_DIR="$POSTPROCESS_DIR/cropping_algorithms"
    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/raspberrypi/cascading_networks/resources"

    readonly DEFAULT_DETECTION_POSTPROCESS_SO="$POSTPROCESS_DIR/libface_detection_post.so"
    readonly DEFAULT_DETECTION_VIDEO_SOURCE="$RESOURCES_DIR/face_detection.mp4"
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
    camera_input_width=1280
    camera_input_height=720
    camera_framerate="40/1"
    camera_input_format="RGB"
}

function print_usage() {
    echo "Sanity hailo pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                  Show this help"
    echo "  -i INPUT --input INPUT  Set the input video file path (default $input_source)"
    echo "  --show-fps              Print fps"
    echo "  --print-gst-launch      Print the ready gst-launch command without running it"
    exit 0
}

function validate_rpi_platform_camera_support() {
    os_id=$(cat /etc/os-release | grep ^ID=)
    if [ "$os_id" != "ID=debian" ]; then
        echo "Camera device supported only in Debian release of Raspberry PI"
        exit 1
    fi

    identify_v4l=$(gst-launch-1.0 v4l2src device=$input_source num-buffers=1 ! fakesink 2>&1 | grep 'Cannot identify device' | wc -l)
    if [ "$identify_v4l" -eq 1 ]; then
        echo "Cannot identify v4l device '$input_source'"
        exit 1
    fi

    num_of_cam_devices_sup=$(gst-launch-1.0 --gst-debug=v4l2src:5 v4l2src device=$input_source num-buffers=1 ! fakesink 2>&1 | sed -une '/caps of src/ s/[:;] /\n/gp' | grep $camera_input_format | wc -l)
    if [ "$num_of_cam_devices_sup" -eq 0 ]; then
        echo "v4l device '$input_source' is not supporting requested input format '$camera_input_format'"
        exit 1
    fi
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

# If the video provided is from a camera
if [[ $input_source =~ "/dev/video" ]]; then
    validate_rpi_platform_camera_support
    source_element="v4l2src device=$input_source name=src_0 !  video/x-raw, format=$camera_input_format, width=$camera_input_width, height=$camera_input_height, framerate=$camera_framerate, pixel-aspect-ratio=1/1 ! \
                queue max-size-buffers=5 max-size-bytes=0 max-size-time=0"
else
    source_element="filesrc location=$input_source name=src_0 ! qtdemux ! \
                    queue max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
                    h264parse ! \
                    queue max-size-buffers=5 max-size-bytes=0 max-size-time=0 !  \
                    avdec_h264 ! \
                    queue max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
                    videoconvert n-threads=3 qos=false"
    internal_offset=true
fi

FACE_DETECTION_PIPELINE="videoscale n-threads=4 qos=false ! \
    queue max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailonet net-name=joined_lightface_slim_tddfa_mobilenet_v1/lightface_slim \
    hef-path=$hef_path is-active=true scheduling-algorithm=0 vdevice-key=$DEFAULT_VDEVICE_KEY ! \
    queue max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$detection_postprocess_so function-name=lightface qos=false ! \
    queue max-size-buffers=3 max-size-bytes=0 max-size-time=0"

FACIAL_LANDMARKS_PIPELINE="queue max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailonet net-name=joined_lightface_slim_tddfa_mobilenet_v1/tddfa_mobilenet_v1 \
    hef-path=$hef_path is-active=true scheduling-algorithm=0 vdevice-key=$DEFAULT_VDEVICE_KEY ! \
    queue max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailofilter function-name=facial_landmarks_merged so-path=$landmarks_postprocess_so qos=false ! \
    queue max-size-buffers=3 max-size-bytes=0 max-size-time=0"

PIPELINE="gst-launch-1.0 $source_element ! \
    queue max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    tee name=t hailomuxer name=hmux \
    t. ! queue max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! hmux. \
    t. ! $FACE_DETECTION_PIPELINE ! hmux. \
    hmux. ! queue max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailocropper so-path=$crop_so function-name=create_crops internal-offset=$internal_offset name=cropper \
    hailoaggregator name=agg \
    cropper. ! queue max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! agg. \
    cropper. ! $FACIAL_LANDMARKS_PIPELINE ! agg. \
    agg. ! queue max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay qos=false ! \
    queue max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! videoconvert n-threads=2 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display sync=false text-overlay=false $additional_parameters"

echo "Running $network_name"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
