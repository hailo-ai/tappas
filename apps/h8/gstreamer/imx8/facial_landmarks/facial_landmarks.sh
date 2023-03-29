#!/bin/bash
set -e

CURRENT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

function init_variables() {
    print_help_if_needed $@

    readonly POSTPROCESS_DIR="/usr/lib/hailo-post-processes"
    readonly RESOURCES_DIR="${CURRENT_DIR}/resources"
    readonly DEFAULT_POSTPROCESS_SO="$POSTPROCESS_DIR/libfacial_landmarks_post.so"
    readonly DEFAULT_VIDEO_SOURCE="$RESOURCES_DIR/faces_120_120.mp4"
    readonly DEFAULT_HEF_PATH="${RESOURCES_DIR}/tddfa_mobilenet_v1.hef"

    hef_path=$DEFAULT_HEF_PATH
    input_source=$DEFAULT_VIDEO_SOURCE
    postprocess_so=$DEFAULT_POSTPROCESS_SO

    print_gst_launch_only=false
    additional_parameters=""

    video_sink_element="autovideosink"
    hailo_bus_id=$(hailortcli scan | awk '{ print $NF }' | tail -n 1)
}

function print_usage() {
    echo "IMX8 Face Landmarks pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                  Show this help"
    echo "  -i INPUT --input INPUT  Set the video source (only videos allowed) (default $input_source)"
    echo "  --show-fps              Printing fps"
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

PIPELINE="gst-launch-1.0 \
    filesrc location=$input_source name=src_0 ! decodebin ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    glupload ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    glcolorscale ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    gldownload ! video/x-raw,pixel-aspect-ratio=1/1,format=RGBA ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert name=pre_hailonet_videoconvert n-threads=4 qos=false ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$postprocess_so qos=false ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! videoconvert qos=false ! \
    queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    fpsdisplaysink video-sink='$video_sink_element window-width=600 window-height=600' name=hailo_display sync=true text-overlay=false ${additional_parameters}"

echo ${PIPELINE}
if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi
eval ${PIPELINE}
