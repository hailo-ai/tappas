#!/bin/bash
set -e

CURRENT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

function init_variables() {
    readonly RESOURCES_DIR="${CURRENT_DIR}/resources"
    readonly CONFIGS_DIR="${RESOURCES_DIR}/configs"
    readonly POSTPROCESS_DIR="/usr/lib/hailo-post-processes"
    readonly DEFAULT_POSTPROCESS_SO="$POSTPROCESS_DIR/libyolo_post.so"
    readonly DEFAULT_NETWORK_NAME="yolov5"
    readonly DEFAULT_VIDEO_SOURCE="${RESOURCES_DIR}/detection0.mp4"
    readonly DEFAULT_HEF_PATH="${RESOURCES_DIR}/yolov5s_personface_rgba.hef"
    readonly DEFAULT_JSON_CONFIG_PATH="${CONFIGS_DIR}/yolov5_personface.json"

    postprocess_so=$DEFAULT_POSTPROCESS_SO
    network_name=$DEFAULT_NETWORK_NAME
    input_source=$DEFAULT_VIDEO_SOURCE
    hef_path=$DEFAULT_HEF_PATH
    json_config_path=$DEFAULT_JSON_CONFIG_PATH 

    print_gst_launch_only=false
    additional_parameters=""
}

function print_usage() {
    echo "IMX6 Detection pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                          Show this help"
    echo "  -i INPUT --input INPUT          Set the video source (default $input_source)"
    echo "  --show-fps                      Print fps"
    echo "  --print-gst-launch              Print the ready gst-launch command without running it"
    exit 0
}

function parse_args() {
    while test $# -gt 0; do
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage
            exit 0
        elif [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
        elif [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            additional_parameters="-v | grep hailo_display"
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
    source_element="imxv4l2videosrc device=$input_source name=src_0 ! "
else
    source_element="filesrc location=$input_source name=src_0 ! \
                    qtdemux ! \
                    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                    h264parse ! imxvpudec ! "
fi

PIPELINE="gst-launch-1.0 \
          $source_element \
          queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
          imxg2dvideotransform ! video/x-raw, format=RGBA ! \
          queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
          hailonet hef-path=$hef_path ! \
          queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
          hailofilter function-name=$network_name config-path=$json_config_path so-path=$postprocess_so qos=false ! \
          queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
          hailooverlay qos=false ! \
          queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
          fpsdisplaysink video-sink=imxipuvideosink name=hailo_display sync=false text-overlay=false ${additional_parameters}"



echo "Running $network_name"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
