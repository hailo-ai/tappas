#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh

    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes/"
    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/century/resources"

    readonly DEFAULT_POSTPROCESS_SO="$POSTPROCESS_DIR/libyolo_hailortpp_post.so"
    readonly DEFAULT_VIDEO_SOURCE="$RESOURCES_DIR/detection_5m.mp4"
    readonly DEFAULT_HEF_PATH="$RESOURCES_DIR/yolov5m_wo_spp_60p.hef"
    readonly DEVICE_COUNT=4
    readonly DEVICE_PREFIX="[-]"
    readonly DEFAULT_NETWORK_NAME="yolov5"

    network_name=$DEFAULT_NETWORK_NAME
    input_source=$DEFAULT_VIDEO_SOURCE
    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
    postprocess_so=$DEFAULT_POSTPROCESS_SO
    device_count=$DEVICE_COUNT
    input_source=$DEFAULT_VIDEO_SOURCE
    hef_path=$DEFAULT_HEF_PATH

    print_gst_launch_only=false
    additional_parameters=""
    sync_pipeline=false
    max_devices=$(lspci -d 1e60: | wc -l)

    if (($max_devices == 0)); then
        echo "Error: No devices found."
        exit 1
    fi

    device_count=$max_devices
}

function print_usage() {
    echo "Detection pipeline usage:"
    echo ""
    echo "Options:"
    echo "  -h --help                  Show this help"
    echo "  -i INPUT --input INPUT     Set the input source (default $input_source)"
    echo "  --network NETWORK          Set network to use. choose from [yolox, yolov5m], default is yolov5"
    echo "  --device-count             Number of devices to use"
    echo "  --show-fps                 Print fps"
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
        if [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
        elif [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            additional_parameters="-v | grep -e hailo_display -e hailodevicestats"
        elif [ $1 == "--network" ]; then
            if [ $2 == "yolox" ]; then
                network_name="yolox"
                hef_path="$RESOURCES_DIR/yolovx_l_leaky.hef"
            elif [ $2 != "yolov5m" ]; then
                echo "Received invalid network: $2. See expected arguments below:"
                print_usage
                exit 1
            fi
            shift
        elif [ "$1" = "--input" ] || [ "$1" == "-i" ]; then
            input_source="$2"
            shift
        elif [ "$1" = "--device-count" ]; then
            device_count="$2"
            re='^[0-9]+$'
            if ! [[ $device_count =~ $re ]]; then
                echo "Invaild argument: Requested $device_count devices is not a positive number."
                exit 1
            elif (($device_count > $max_devices)); then
                echo "Invaild argument: Requested $device_count devices while only $max_devices are available."
                exit 1
            fi
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
    source_element="v4l2src device=$input_source name=src_0 ! videoflip video-direction=horiz"
else
    source_element="filesrc location=$input_source name=src_0 ! decodebin"
fi

PIPELINE="gst-launch-1.0 \
    $source_element ! videoconvert qos=false ! \
    videoscale qos=false ! video/x-raw, pixel-aspect-ratio=1/1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path device-count=$device_count scheduling-algorithm=0 is-active=true ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter function-name=$network_name so-path=$postprocess_so qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert qos=false n-threads=4 ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=$sync_pipeline text-overlay=false ${additional_parameters}"

echo "Running $network_name"
echo ${PIPELINE}
if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

available_devices=$(hailortcli scan | grep $DEVICE_PREFIX | wc -l)
if (($device_count > $available_devices)); then
    echo "Got only $available_devices devices when $device_count are needed"
    exit 1
fi

eval ${PIPELINE}
