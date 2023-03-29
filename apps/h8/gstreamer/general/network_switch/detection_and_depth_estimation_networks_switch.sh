#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@

    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh

    # Basic Directories
    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes"
    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/network_switch/resources"

    # Default Video
    readonly DEFAULT_VIDEO="$RESOURCES_DIR/instance_segmentation.mp4"

    # Detection Macros
    readonly DETECTION_HEF="$RESOURCES_DIR/yolov5s.hef"
    readonly DETECTION_POST_SO="$POSTPROCESS_DIR/libyolo_post.so"
    readonly DETECTION_POST_FUNC="yolov5"
    readonly DEFAULT_DETECTION_JSON_CONFIG_PATH="$RESOURCES_DIR/configs/yolov5.json" 

    # Depth Estimation Macros
    readonly DEPTH_ESTIMATION_HEF="$RESOURCES_DIR/fast_depth.hef"
    readonly DEPTH_ESTIMATION_POST_SO="$POSTPROCESS_DIR/libdepth_estimation.so"

    input_source=$DEFAULT_VIDEO
    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
    sync_pipeline=false
    additional_parameters=""
    json_config_path=$DEFAULT_DETECTION_JSON_CONFIG_PATH 
}

function print_usage() {
    echo "Multistream Detection hailo - pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                   Show this help"
    echo "  -i INPUT --input INPUT   Set the video source (default $DEFAULT_VIDEO)"
    echo "  --show-fps               Printing fps"
    echo "  --print-gst-launch         Print the ready gst-launch command without running it"
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
            additional_parameters="-v | grep -e hailo_display -e hailodevicestats"
        elif [ "$1" = "--input" ] || [ "$1" == "-i" ]; then
            input_source="$2"
            shift
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
    source_element="v4l2src device=$input_source name=src_0 ! videoflip video-direction=horiz"
    sync_pipeline=true
else
    source_element="filesrc location=$input_source name=src_0 ! decodebin"
fi

DETECTION_PIPELINE="\
        videoscale n-threads=2 qos=false ! video/x-raw, width=640, height=640 ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailonet hef-path=$DETECTION_HEF vdevice-key=1 scheduling-algorithm=1 ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailofilter so-path=$DETECTION_POST_SO config-path=$json_config_path function-name=$DETECTION_POST_FUNC qos=false ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailooverlay qos=false ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        videoconvert n-threads=2 qos=false ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        fpsdisplaysink video-sink=$video_sink_element name=hailo_display2 sync=$sync_pipeline text-overlay=false"

DEPTH_ESTIMATION_PIPELINE="\
        aspectratiocrop aspect-ratio=1/1 ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        videoscale qos=false ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailonet hef-path=$DEPTH_ESTIMATION_HEF vdevice-key=1 scheduling-algorithm=1 ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailofilter so-path=$DEPTH_ESTIMATION_POST_SO qos=false ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailooverlay mask-overlay-n-threads=2 qos=false ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        videoscale qos=false ! video/x-raw, width=400, height=400 ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        videoconvert n-threads=2 qos=false ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=$sync_pipeline text-overlay=false ${additional_parameters}"

PIPELINE="gst-launch-1.0 \
    $source_element ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert n-threads=2 qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    tee name=t \
    t. ! \
        $DETECTION_PIPELINE
    t. ! \
        $DEPTH_ESTIMATION_PIPELINE"

echo "Running License Plate Recognition"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}