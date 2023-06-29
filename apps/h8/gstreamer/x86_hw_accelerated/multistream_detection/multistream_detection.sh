#!/bin/bash
set -e


function init_per_ubuntu_version() {
    ubuntu_version=$(lsb_release -r | awk '{print $2}' | awk -F'.' '{print $1}')
    if [ $ubuntu_version -eq 22 ]; then
        readonly DECODER_FORMAT="RGBA"
    elif [ $ubuntu_version -eq 20 ]; then
        readonly DECODER_FORMAT="I420"
    else
        log_error "ubuntu version $ubuntu_version is not supported. Supporting only ubuntu 20 or ubuntu 22"
        exit 1
    fi
}


function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh --check-vaapi

    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/x86_hw_accelerated/multistream_detection/resources"
    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes/"
    readonly POSTPROCESS_SO="$POSTPROCESS_DIR/libyolo_post.so"
    readonly HEF_PATH="$RESOURCES_DIR/yolov5s_personface_rgba.hef"
    readonly DEFAULT_JSON_CONFIG_PATH="$RESOURCES_DIR/yolov5_personface.json"
    readonly MAX_NUM_OF_SOURCES=8
    num_of_src=4
    additional_parameters=""
    sources=""
    compositor_locations="sink_0::xpos=0 sink_0::ypos=0 sink_1::xpos=640 sink_1::ypos=0 sink_2::xpos=0 sink_2::ypos=640 sink_3::xpos=640 sink_3::ypos=640 sink_4::xpos=1280 sink_4::ypos=0 sink_5::xpos=1280 sink_5::ypos=640 sink_6::xpos=1920 sink_6::ypos=0 sink_7::xpos=1920 sink_7::ypos=640"
    print_gst_launch_only=false
    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
    json_config_path=$DEFAULT_JSON_CONFIG_PATH
    sync_pipeline=false
    init_per_ubuntu_version    
}

function print_usage() {
    echo "Multistream Detection hailo - pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                          Show this help"
    echo "  --show-fps                      Printing fps"
    echo "  --num-of-sources NUM            Setting number of sources to given input (default value is $num_of_src, maximum value is $MAX_NUM_OF_SOURCES)"
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
        elif [ "$1" = "--num-of-sources" ]; then
            if [ "$2" -lt 1 ] || [ "$2" -gt $MAX_NUM_OF_SOURCES ]; then
                echo "Invalid argument received: num-of-sources must be between 1-$MAX_NUM_OF_SOURCES"
                exit 1
            fi
            shift
            echo "Setting number of sources to $1"
            num_of_src=$1
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
    for ((n = $start_index; n < $num_of_src; n++)); do
        sources+="filesrc location=$RESOURCES_DIR/video$n.mp4 name=source_$n ! \
                queue name=pre_decode_q_$n leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                qtdemux ! vaapidecodebin ! video/x-raw,format=$DECODER_FORMAT,width=1280,height=720 ! \
                queue name=hailo_preprocess_q_$n leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
                videoconvert qos=false ! videoscale method=0 add-borders=false qos=false ! \
                video/x-raw,width=640,height=640,pixel-aspect-ratio=1/1 ! \
                queue name=hailo_prestream_mux_q_$n leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                fun.sink_$n sid.src_$n ! queue name=pre_comp_videoconvert_q_$n leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                videoconvert name=pre_comp_videoconvert_$n qos=false ! \
                queue name=comp_q_$n leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! comp.sink_$n "
        streamrouter_input_streams+=" src_$n::input-streams=\"<sink_$n>\""
        
    done
}

function main() {
    init_variables $@
    streamrouter_input_streams=""
    parse_args $@
    create_sources

    pipeline="gst-launch-1.0 \
           hailoroundrobin mode=1 name=fun ! \
           queue name=hailo_pre_infer_q_0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           hailonet hef-path=$HEF_PATH ! \
           queue name=hailo_postprocess0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           hailofilter so-path=$POSTPROCESS_SO config-path=$json_config_path qos=false ! \
           queue name=hailo_draw0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           hailooverlay qos=false ! \
           hailostreamrouter name=sid $streamrouter_input_streams \
           compositor name=comp start-time-selection=0 $compositor_locations ! \
           queue name=hailo_display_q_0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=$sync_pipeline text-overlay=false \
           $sources ${additional_parameters}"

    echo ${pipeline}
    if [ "$print_gst_launch_only" = true ]; then
        exit 0
    fi

    echo "Running Pipeline..."
    eval "${pipeline}"

}

main $@
