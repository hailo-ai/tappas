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
    live_src=""
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
    echo "  --set-live-source LIVE_SOURCE   Use the actual source data given (example: /dev/video2). this flag is optional. if it's in use, num_of_sources is limited to 4."
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
            additional_parameters="-v | grep hailo_display"
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
        elif [ "$1" = "--set-live-source" ]; then
            shift
            echo "Setting live_src to $1"
            live_src=$1
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
    start_index=0
    identity=""
    if [ "$live_src" != "" ] && [ -e $live_src ]; then
        identity="identity single-segment=true sync=true !"
        sources+="v4l2src device=$live_src name=source_0 ! videoflip video-direction=horiz ! \
                queue name=hailo_preprocess_q_0 leaky=downstream max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
                videoconvert ! videobox autocrop=true ! video/x-raw,width=640,height=640,pixel-aspect-ratio=1/1 ! \
                $identity fun.sink_0 sid.src_0 ! queue name=comp_q_0 \
                leaky=downstream max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! comp.sink_0 "
        streamrouter_input_streams+=" src_0::input-streams=\"<sink_0>\""
        start_index=1
        if [ $num_of_src -gt 4 ]; then
            num_of_src=4
        fi
    fi

    for ((n = $start_index; n < $num_of_src; n++)); do
        sources+="uridecodebin3 uri=file://$RESOURCES_DIR/detection$n.mp4 \
                name=source_$n ! videorate ! video/x-raw,framerate=30/1 ! \
                queue name=hailo_preprocess_q_$n leaky=no max-size-buffers=5 max-size-bytes=0  \
                max-size-time=0 ! videoconvert ! videoscale method=0 add-borders=false ! \
                video/x-raw,width=640,height=640,pixel-aspect-ratio=1/1 ! $identity \
                fun.sink_$n sid.src_$n ! queue name=comp_q_$n leaky=downstream max-size-buffers=30 \
                max-size-bytes=0 max-size-time=0 ! comp.sink_$n "
         streamrouter_input_streams+=" src_$n::input-streams=\"<sink_$n>\""
    done
}

function main() {
    init_variables $@
    parse_args $@
    create_sources

    pipeline="gst-launch-1.0 \
           hailoroundrobin mode=0 name=fun ! \
           queue name=hailo_pre_infer_q_0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           hailonet hef-path=$hef_path device-count=$device_count scheduling-algorithm=0 nms-score-threshold=0.3 nms-iou-threshold=0.45 output-format-type=HAILO_FORMAT_TYPE_FLOAT32 ! \
           queue name=hailo_postprocess0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           hailofilter function-name=$network_name so-path=$POSTPROCESS_SO qos=false ! \
           queue name=hailo_draw0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           hailooverlay ! \
           hailostreamrouter name=sid $streamrouter_input_streams compositor name=comp start-time-selection=0 $compositor_locations ! \
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
