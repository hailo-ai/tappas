#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh --check-python-packages

    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/python/resources"
    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes/"
    readonly DEFAULT_POSTPROCESS_MODULE="$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/python/classification.py"
    readonly DEFAULT_VIDEO_SOURCE="$RESOURCES_DIR/classification_movie.mp4"
    readonly DEFAULT_HEF_PATH="$RESOURCES_DIR/resnet_v1_50.hef"

    hef_path=$DEFAULT_HEF_PATH
    input_source=$DEFAULT_VIDEO_SOURCE
    postprocess_module=$DEFAULT_POSTPROCESS_MODULE
    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")

    print_gst_launch_only=false
    additional_parameters=""
}

function print_usage() {
    echo "Classification pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                  Show this help"
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
        if [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            additional_parameters="-v | grep hailo_display"
        elif [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
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
    echo "Received invalid video source: $input_source - Only videos are allowed."
    exit 1
fi

PIPELINE="gst-launch-1.0 \
    filesrc location=$input_source ! decodebin ! \
    videoscale ! video/x-raw, pixel-aspect-ratio=1/1 ! \
    videoconvert ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailopython qos=false module=$postprocess_module ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=false text-overlay=false ${additional_parameters}"

echo "Running"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
