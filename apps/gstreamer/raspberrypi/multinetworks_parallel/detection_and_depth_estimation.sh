#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../scripts/misc/checks_before_run.sh

    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/gstreamer/x86/libs"
    readonly DEFAULT_DRAW_SO="$POSTPROCESS_DIR/libdepth_estimation.so"
    readonly DEFAULT_VIDEO_SOURCE="$TAPPAS_WORKSPACE/apps/gstreamer/x86/multinetworks_parallel/resources/instance_segmentation.mp4"
    readonly DEFAULT_HEF_PATH="$TAPPAS_WORKSPACE/apps/gstreamer/x86/multinetworks_parallel/resources/joined_fast_depth_ssd_mobilenet_v1_no_alls.hef"

    video_source=$DEFAULT_VIDEO_SOURCE
    hef_path=$DEFAULT_HEF_PATH
    depth_estimation_draw_so=$DEFAULT_DRAW_SO
    detection_post_so="$POSTPROCESS_DIR/libmobilenet_ssd_post.so"
    detection_draw_so="$POSTPROCESS_DIR/libdetection_draw.so"

    depth_estimation_net_name="joined_fast_depth_ssd_mobilenet_v1_no_alls/fast_depth"
    detection_net_name="joined_fast_depth_ssd_mobilenet_v1_no_alls/ssd_mobilenet_v1_no_alls"

    print_gst_launch_only=false
    additonal_parameters=""
    compositor_locations="sink_0::xpos=0 sink_0::ypos=0 sink_1::xpos=300 sink_1::ypos=0"

    hailo_bus_id=$(hailortcli scan | awk '{ print $NF }' | tail -n 1)
}

function print_usage() {
    echo "Detection and Depth estimation pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                  Show this help"
    echo "  -i INPUT --input INPUT  Set the input video file path (default $input_source)"
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
            additonal_parameters="-v 2>&1 | grep hailo_display"
        elif [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
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
if [[ $input_source =~ "/dev/video" ]]; then
    echo "Received invalid argument: $input_source. Live input sources are currently not supported."
    exit 1
else
    source_element="filesrc location=$video_source name=src_0 ! qtdemux ! h264parse ! avdec_h264 "
    sync_pipeline=true
fi

PIPELINE="gst-launch-1.0 \
    $source_element ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert n-threads=8 !
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    tee name=t ! \
    aspectratiocrop aspect-ratio=1/1 ! \
    queue ! videoscale n-threads=8 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true net-name=$depth_estimation_net_name qos=false batch-size=1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$depth_estimation_draw_so qos=false debug=False ! videoconvert n-threads=8 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display sync=false text-overlay=false \
    t. ! \
    videoscale n-threads=8 ! queue ! \
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true net-name=$detection_net_name qos=false batch-size=1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter2 so-path=$detection_post_so function-name=mobilenet_ssd_merged qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! videoconvert n-threads=8 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display2 sync=false text-overlay=false ${additonal_parameters} "

echo "Running"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
