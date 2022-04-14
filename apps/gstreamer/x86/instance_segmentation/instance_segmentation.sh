#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../scripts/misc/checks_before_run.sh

    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/gstreamer/x86/libs/"
    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/gstreamer/x86/instance_segmentation/resources"

    readonly DEFAULT_POSTPROCESS_SO="$POSTPROCESS_DIR/libyolact_post.so"
    readonly DEFAULT_DRAW_SO="$POSTPROCESS_DIR/libdetection_draw.so"
    readonly DEFAULT_VIDEO_SOURCE="$TAPPAS_WORKSPACE/apps/gstreamer/x86/instance_segmentation/resources/instance_segmentation.mp4"
    readonly DEFAULT_BATCH_SIZE="1"
    readonly DEFAULT_HEF_PATH="$RESOURCES_DIR/yolact_regnetx_800mf_fpn_20classes.hef"

    postprocess_so=$DEFAULT_POSTPROCESS_SO
    video_source=$DEFAULT_VIDEO_SOURCE
    batch_size=$DEFAULT_BATCH_SIZE
    hef_path=$DEFAULT_HEF_PATH
    draw_so=$DEFAULT_DRAW_SO

    print_gst_launch_only=false
    additonal_parameters=""

    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
    hailo_bus_id=$(hailortcli scan | awk '{ print $NF }' | tail -n 1)
}

function print_usage() {
    echo "Sanity hailo pipeline usage:"
    echo ""
    echo "Options:"
    echo "  -h --help               Show this help"
    echo "  -i INPUT --input INPUT  Set the video source (default $video_source)"
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
            additonal_parameters="-v 2>&1 | grep hailo_display"
        elif [ "$1" = "--input" ] || [ "$1" == "-i" ]; then
            video_source="$2"
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
if [[ $video_source =~ "/dev/video" ]]; then
    source_element="v4l2src device=$video_source name=src_0 ! videoflip video-direction=horiz"
else
    source_element="filesrc location=$video_source name=src_0 ! decodebin"
fi

PIPELINE="gst-launch-1.0 \
    $source_element ! \
    videoscale ! video/x-raw, pixel-aspect-ratio=1/1 ! videoconvert ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true qos=false batch-size=$batch_size ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$postprocess_so qos=false debug=False ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$draw_so qos=false debug=False ! \
    videoconvert ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=false text-overlay=false ${additonal_parameters}"

echo "Running $network_name"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
