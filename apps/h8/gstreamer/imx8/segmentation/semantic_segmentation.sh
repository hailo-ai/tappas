#!/bin/bash
set -e

CURRENT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

function init_variables() {
    print_help_if_needed $@

    readonly RESOURCES_DIR="${CURRENT_DIR}/resources"
    readonly POSTPROCESS_DIR="/usr/lib/hailo-post-processes"
    readonly DEFAULT_POSTPROCESS_SO="$POSTPROCESS_DIR/libsemantic_segmentation.so"
    readonly DEFAULT_VIDEO_SOURCE="$RESOURCES_DIR/full_mov_slow.mp4"
    readonly DEFAULT_HEF_PATH="$RESOURCES_DIR/resnet18_fcn8_fhd.hef"

    input_source=$DEFAULT_VIDEO_SOURCE
    hef_path=$DEFAULT_HEF_PATH
    post_so=$DEFAULT_POSTPROCESS_SO

    print_gst_launch_only=false
    additional_parameters=""
    video_sink_element="autovideosink"
    overlay_width=452
    overlay_height=452
}

function print_usage() {
    echo "Semantic Segmentation pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                    Show this help"
    echo "  -i INPUT --input INPUT    Set the video source (only videos allowed) (default $input_source)"
    echo "  --show-fps                Print fps"
    echo "  --print-gst-launch        Print the ready gst-launch command without running it"
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
        elif [ "$1" = "--input" ] || [ "$1" = "-i" ]; then
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
    echo "Received invalid video source: $input_source - Only videos are allowed."
    exit 1
fi

opengl_covnert="glupload ! \
                glcolorscale qos=false ! \
                glcolorconvert qos=false ! \
                gldownload ! video/x-raw,pixel-aspect-ratio=1/1,format=RGBA ! \
                videoconvert name=pre_hailonet_videoconvert n-threads=4 qos=false ! video/x-raw, format=RGB"

PIPELINE="gst-launch-1.0 \
    filesrc location=$input_source ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    decodebin ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    $opengl_covnert ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$post_so qos=false ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoscale name=pre_overlay_scale n-threads=4 ! video/x-raw, width=$overlay_width, height=$overlay_height ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay qos=false ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=false text-overlay=false ${additional_parameters}"

echo "Running"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
