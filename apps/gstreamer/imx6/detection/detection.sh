#!/bin/bash
set -e

CURRENT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

function init_variables() {
    readonly RESOURCES_DIR="${CURRENT_DIR}/resources"
    readonly POSTPROCESS_DIR="/usr/lib/hailo-post-processes"
    readonly DEFAULT_POSTPROCESS_SO="$POSTPROCESS_DIR/libmobilenet_ssd_post.so"
    readonly DEFAULT_NETWORK_NAME="mobilenet_ssd"
    readonly DEFAULT_VIDEO_SOURCE="/dev/video0"
    readonly DEFAULT_HEF_PATH="${RESOURCES_DIR}/ssd_mobilenet_v1.hef"
    readonly DEFAULT_JSON_CONFIG_PATH="null" 

    postprocess_so=$DEFAULT_POSTPROCESS_SO
    network_name=$DEFAULT_NETWORK_NAME
    input_source=$DEFAULT_VIDEO_SOURCE
    hef_path=$DEFAULT_HEF_PATH
    json_config_path=$DEFAULT_JSON_CONFIG_PATH 

    print_gst_launch_only=false
    additonal_parameters=""
}

function print_usage() {
    echo "IMX6 Detection pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help              Show this help"
    echo "  -i INPUT --input INPUT          Set the video source (default $input_source)"
    echo "  --show-fps          Print fps"
    echo "  --print-gst-launch  Print the ready gst-launch command without running it"
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
            additonal_parameters="-v | grep hailo_display"
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

opengl_convert="glupload ! \
                queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                glcolorconvert qos=false ! \
                queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                glcolorscale qos=false ! \
                queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                gldownload ! \
                video/x-raw,pixel-aspect-ratio=1/1,format=RGBA ! \
                queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                videoconvert name=pre_hailonet_videoconvert n-threads=2 qos=false ! \
                video/x-raw, format=RGB, width=300, height=300"

PIPELINE="gst-launch-1.0 \
          v4l2src device=$input_source ! video/x-raw,format=YUY2,width=640,height=480,framerate=30/1 ! \
          queue leaky=downstream max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
          $opengl_convert ! \
          queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
          hailonet hef-path=$hef_path batch-size=1 ! \
          queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
          hailofilter function-name=$network_name so-path=$postprocess_so config-path=$json_config_path qos=false ! \
          queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
          hailooverlay qos=false ! \
          queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
          videoconvert qos=false ! \
          queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
          fpsdisplaysink video-sink=autovideosink name=hailo_display sync=false text-overlay=false ${additonal_parameters}"

echo "Running $network_name"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
