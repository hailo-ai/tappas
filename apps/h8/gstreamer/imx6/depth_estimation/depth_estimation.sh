#!/bin/bash
set -e

CURRENT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

function init_variables() {
    print_help_if_needed $@

    readonly RESOURCES_DIR="${CURRENT_DIR}/resources"
    readonly POSTPROCESS_DIR="/usr/lib/hailo-post-processes"
    readonly DEFAULT_POST_SO="$POSTPROCESS_DIR/libdepth_estimation.so"
    readonly DEFAULT_VIDEO_SOURCE="$RESOURCES_DIR/instance_segmentation.mp4"
    readonly DEFAULT_HEF_PATH="$RESOURCES_DIR/fast_depth.hef"

    input_source=$DEFAULT_VIDEO_SOURCE
    hef_path=$DEFAULT_HEF_PATH
    post_so=$DEFAULT_POST_SO
    print_gst_launch_only=false
    additional_parameters=""
    video_sink_element="autovideosink"
}

function print_usage() {
    echo "Depth estimation pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                   Show this help"
    echo "  --show-fps               Print fps"
    echo "  --print-gst-launch       Print the ready gst-launch command without running it"
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

opengl_convert="glupload ! \
                queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                glcolorscale qos=false ! \
                queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                glcolorconvert qos=false ! \
                queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                gldownload ! video/x-raw,pixel-aspect-ratio=1/1,format=RGBA ! \
                queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                videoconvert name=pre_hailonet_videoconvert n-threads=2 qos=false ! \
                video/x-raw, format=RGB, width=224, height=224"

PIPELINE="gst-launch-1.0 \
    filesrc location=$input_source name=src_0 ! decodebin ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    $opengl_convert ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    aspectratiocrop aspect-ratio=1/1 ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoscale ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$post_so qos=false ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay qos=false ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert qos=false ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=false text-overlay=false ${additional_parameters}"

echo "Running"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
