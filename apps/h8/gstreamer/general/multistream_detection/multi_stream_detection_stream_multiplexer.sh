#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh

    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/multistream_detection/resources"
    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes/"
    readonly POSTPROCESS_SO="$POSTPROCESS_DIR/libyolo_hailortpp_post.so"
    readonly DEFAULT_HEF_PATH="$RESOURCES_DIR/yolov5m_wo_spp_60p.hef"
    readonly DEFAULT_NETWORK_NAME="yolov5"
    readonly MAX_NUM_OF_DEVICES=4

    hef_path=$DEFAULT_HEF_PATH
    network_name=$DEFAULT_NETWORK_NAME
    num_of_src=12
    additional_parameters=""
    sources=""
    compositor_locations="sink_0::xpos=0 sink_0::ypos=0 sink_1::xpos=640 sink_1::ypos=0 sink_2::xpos=1280 sink_2::ypos=0 sink_3::xpos=1920 sink_3::ypos=0 sink_4::xpos=0 sink_4::ypos=640 sink_5::xpos=640 sink_5::ypos=640 sink_6::xpos=1280 sink_6::ypos=640 sink_7::xpos=1920 sink_7::ypos=640 sink_8::xpos=0 sink_8::ypos=1280 sink_9::xpos=640 sink_9::ypos=1280 sink_10::xpos=1280 sink_10::ypos=1280 sink_11::xpos=1920 sink_11::ypos=1280 sink_12::xpos=0 sink_12::ypos=1920 sink_13::xpos=640 sink_13::ypos=1920 sink_14::xpos=1280 sink_14::ypos=1920 sink_15::xpos=1920 sink_15::ypos=1920"
    print_gst_launch_only=false
    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")

    max_devices=$(lspci -d 1e60: | wc -l)

    if (($max_devices == 0)); then
        echo "Error: No devices found."
        exit 1
    fi
    
    if (($max_devices > $MAX_NUM_OF_DEVICES)); then
        max_devices=$MAX_NUM_OF_DEVICES
    fi

    device_count=$max_devices
}

function print_usage() {
    echo "Multistream Detection hailo - pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                          Show this help"
    echo "  --show-fps                      Printing fps"
    echo "  --network NETWORK               Set network to use. choose from [yolov5, yolox, yolov8], default is yolov5"
    echo "  --num-of-sources NUM            Setting number of sources to given input (default value is 12, maximum value is 16)"
    echo "  --device-count NUM              Number of devices to use, maximum allowed is $max_devices"
    echo "  --print-gst-launch              Print the ready gst-launch command without running it"
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
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage
            exit 0
        elif [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
        elif [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            additional_parameters="-v 2>&1 | grep hailo_display"
        elif [ $1 == "--network" ]; then
            if [ $2 == "yolov8" ]; then
                network_name="yolov8m"
                hef_path="$RESOURCES_DIR/yolov8m.hef"
            elif [ $2 == "yolox" ]; then
                network_name="yolox"
                hef_path="$RESOURCES_DIR/yolovx_l_leaky.hef"
            elif [ $2 != "yolov5" ]; then
                echo "Received invalid network: $2. See expected arguments below:"
                print_usage
                exit 1
            fi
            shift
        elif [ "$1" = "--num-of-sources" ]; then
            shift
            echo "Setting number of sources to $1"
            num_of_src=$1
        elif [ "$1" = "--device-count" ]; then
            device_count="$2"
            re='^[1-9][0-9]*$'
            if ! [[ $device_count =~ $re ]]; then
                echo "Invaild argument: Requested $device_count devices is not a positive number."
                exit 1
            elif (($device_count > $max_devices)); then
                echo "Invaild argument: Requested $device_count devices while only a maximum of $max_devices is allowed."
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

function create_sources() {
    for ((n = 0; n < $num_of_src; n++)); do
        sources+="uridecodebin3 uri=file://$RESOURCES_DIR/detection$n.mp4 name=source_$n ! \
                queue name=hailo_preprocess_q_$n leaky=no max-size-buffers=5 max-size-bytes=0  \
                max-size-time=0 ! videoconvert qos=false ! videoscale qos=false ! \
                video/x-raw, pixel-aspect-ratio=1/1 ! \
                queue name=hailo_pre_infer_q_$n leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                hailonet hef-path=$hef_path device-count=$device_count scheduling-algorithm=1 batch-size=1 vdevice-group-id=1 nms-score-threshold=0.3 nms-iou-threshold=0.45 output-format-type=HAILO_FORMAT_TYPE_FLOAT32 ! \
                queue name=hailo_postprocess0_$n leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                hailofilter function-name=$network_name so-path=$POSTPROCESS_SO qos=false ! \
                queue name=hailo_draw0_$n leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                hailooverlay ! \
                queue name=comp_q_$n leaky=downstream max-size-buffers=30 \
                max-size-bytes=0 max-size-time=0 ! comp.sink_$n "
    done
}

function main() {
    init_variables $@
    parse_args $@
    create_sources

    pipeline="gst-launch-1.0 \
           compositor name=comp start-time-selection=0 $compositor_locations ! \
           queue name=hailo_video_q_0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           videoconvert ! queue name=hailo_display_q_0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=false text-overlay=false \
           $sources ${additional_parameters}"

    echo "Running $network_name"
    echo ${pipeline}
    if [ "$print_gst_launch_only" = true ]; then
        exit 0
    fi

    echo "Running Pipeline..."
    eval "${pipeline}"

}

main $@
