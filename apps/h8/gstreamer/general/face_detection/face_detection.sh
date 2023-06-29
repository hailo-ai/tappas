#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh

    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes"
    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/face_detection/resources"
    readonly DEFAULT_POSTPROCESS_SO="$POSTPROCESS_DIR/libface_detection_post.so"
    readonly SCRFD_POSTPROCESS_SO="$POSTPROCESS_DIR/libscrfd_post.so"
    readonly DEFAULT_VIDEO_SOURCE="$RESOURCES_DIR/face_detection.mp4"
    readonly DEFAULT_HEF_PATH="$RESOURCES_DIR/lightface_slim.hef"
    readonly DEFAULT_NETWORK_NAME="lightface"
    readonly DEFAULT_JSON_CONFIG_PATH="$RESOURCES_DIR/configs/lightface.json"


    network_name=$DEFAULT_NETWORK_NAME
    input_source=$DEFAULT_VIDEO_SOURCE
    hef_path=$DEFAULT_HEF_PATH
    postprocess_so=$DEFAULT_POSTPROCESS_SO
    sync_pipeline=false
    json_config_path=$DEFAULT_JSON_CONFIG_PATH


    print_gst_launch_only=false
    additional_parameters=""

    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
}

function print_usage() {
    echo "Face Detection pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                  Show this help"
    echo "  --network NETWORK       Set network to use. choose from [lightface, retinaface, scrfd], default is lightface"
    echo "  -i INPUT --input INPUT  Set the input source (default $input_source)"
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
        if [ $1 == "--network" ]; then
            if [ $2 == "retinaface" ]; then
                network_name="$2"
                hef_path="$RESOURCES_DIR/retinaface_mobilenet_v1.hef"
                json_config_path="$RESOURCES_DIR/configs/retinaface.json"
            elif [ $2 == "scrfd" ]; then
                network_name="scrfd_10g"
                hef_path="$RESOURCES_DIR/scrfd_10g.hef"
                json_config_path="$RESOURCES_DIR/configs/scrfd.json"
                postprocess_so="$SCRFD_POSTPROCESS_SO"
            elif [ $2 != "lightface" ]; then
                echo "Received invalid network: $2. See expected arguments below:"
                print_usage
                exit 1
            fi
            shift
        elif [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
        elif [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            additional_parameters="-v | grep hailo_display"
        elif [ "$1" = "--input" ] || [ "$1" == "-i" ]; then
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
    source_element="v4l2src device=$input_source name=src_0 ! videoflip video-direction=horiz"
else
    source_element="filesrc location=$input_source name=src_0 ! decodebin"
fi

PIPELINE="gst-launch-1.0 \
    $source_element ! videoconvert ! tee name=t hailomuxer name=mux \
    t. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! mux. \
    t. ! videoscale ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter function-name=$network_name so-path=$postprocess_so config-path=$json_config_path qos=false ! mux. \
    mux. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=$sync_pipeline text-overlay=false ${additional_parameters}"

echo "Running $network_name"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
