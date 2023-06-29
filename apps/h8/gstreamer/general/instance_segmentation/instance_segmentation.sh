#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh

    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes"
    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/instance_segmentation/resources"

    readonly DEFAULT_POSTPROCESS_SO="$POSTPROCESS_DIR/libyolov5seg_post.so"
    readonly DEFAULT_VIDEO_SOURCE="$RESOURCES_DIR/instance_segmentation.mp4"
    readonly DEFAULT_HEF_PATH="$RESOURCES_DIR/yolov5n_seg.hef"
    readonly DEFAULT_NETWORK_NAME="yolov5seg"
    readonly json_config_path="$RESOURCES_DIR/configs/yolov5seg.json"

    postprocess_so=$DEFAULT_POSTPROCESS_SO
    network_name=$DEFAULT_NETWORK_NAME
    video_source=$DEFAULT_VIDEO_SOURCE
    hef_path=$DEFAULT_HEF_PATH

    print_gst_launch_only=false
    additional_parameters=""

    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
}

function print_usage() {
    echo "Sanity hailo pipeline usage:"
    echo ""
    echo "Options:"
    echo "  -h --help               Show this help"
    echo "  --network NETWORK       Set network to use. choose from [yolov5seg, yolact_20_classes, yolact1_6gf, yolact800mf],"
    echo "                          default network is yolov5seg"
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
        elif [ "$1" = "--network" ]; then
            if [ "$2" = "yolov5seg" ]; then
                network_name="yolov5seg"
                postprocess_so="$POSTPROCESS_DIR/libyolov5seg_post.so"
                hef_path="$RESOURCES_DIR/yolov5n_seg.hef"
            elif [ "$2" = "yolact800mf" ]; then
                network_name="yolact800mf"
                postprocess_so="$POSTPROCESS_DIR/libyolact_post.so"
                hef_path="$RESOURCES_DIR/yolact_regnetx_800mf.hef"
            elif [ "$2" = "yolact1_6gf" ]; then
                network_name="yolact1_6gf"
                postprocess_so="$POSTPROCESS_DIR/libyolact_post.so"
                hef_path="$RESOURCES_DIR/yolact_regnetx_1.6gf.hef"
            elif [ "$2" = "yolact_20_classes" ]; then
                network_name="yolact_20_classes"
                postprocess_so="$POSTPROCESS_DIR/libyolact_post.so"
                hef_path="$RESOURCES_DIR/yolact_regnetx_800mf_fpn_20classes.hef"
            else
                echo "Received invalid network: $2. See expected arguments below here:"
                print_usage
                exit 1
            fi
            shift
        elif [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            additional_parameters="-v | grep hailo_display"
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
    hailonet hef-path=$hef_path ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter function-name=$network_name so-path=$postprocess_so config-path=$json_config_path qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=false text-overlay=false ${additional_parameters}"

echo "Running $network_name"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
