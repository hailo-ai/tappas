#!/bin/bash
set -e

CURRENT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

function init_variables() {
    readonly RESOURCES_DIR="${CURRENT_DIR}/resources"
    readonly POSTPROCESS_DIR="/usr/lib/hailo-post-processes"
    readonly DEFAULT_POSTPROCESS_SO="$POSTPROCESS_DIR/libyolo_post.so"
    readonly DEFAULT_HEF_PATH="/home/root/apps/multistream_detection/resources/yolov5s_personface_nv12_no_ddr.hef"
    readonly DEFAULT_JSON_CONFIG_PATH="/home/root/apps/multistream_detection/resources/configs/yolov5_personface.json"

    postprocess_so=$DEFAULT_POSTPROCESS_SO
    hef_path=$DEFAULT_HEF_PATH
    json_config_path=$DEFAULT_JSON_CONFIG_PATH 
    sync_pipeline=false
    num_of_src=6
    compositor_locations="sink_0::xpos=0 sink_0::ypos=0 sink_1::xpos=640 sink_1::ypos=0 sink_2::xpos=1280 sink_2::ypos=0 sink_3::xpos=1920 sink_3::ypos=0 sink_4::xpos=0 sink_4::ypos=640 sink_5::xpos=640 sink_5::ypos=640 sink_6::xpos=1280 sink_6::ypos=640 sink_7::xpos=1920 sink_7::ypos=640 sink_8::xpos=0 sink_8::ypos=1280 "    
    print_gst_launch_only=false
    videosink="autovideosink"
    additonal_parameters=""
}

function print_usage() {
    echo "Multistream Detection hailo - pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                          Show this help"
    echo "  --show-fps                      Printing fps"
    echo "  --fakesink                      Run the application without display"
    echo "  --num-of-sources NUM            Setting number of sources to given input (default and maximum value is 6)"
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
            #grep all hailo_display but filter out GstGhostPad:
            additonal_parameters="-v | grep hailo_display | grep -v GstGhostPad"
            additonal_parameters="-v | grep hailo_display"

        elif [ "$1" = "--fakesink" ]; then
            echo "Running without display"
            videosink="fakesink"
            sync_pipeline=true 
        elif [ "$1" = "--num-of-sources" ]; then
            shift
            if [ $1 -gt 6 ]; then
                echo "Received number of sources: $1, but maximum number of sources is 8"
                exit 1
            fi
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
    for ((n = 0; n < $num_of_src; n++)); do
        sources+="filesrc location=/home/root/apps/multistream_detection/resources/detection$n.mp4 ! \
        queue name=dec_$n max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! decodebin ! \
	    video/x-raw, format=NV12 ! fun.sink_$n sid.src_$n ! \
        queue name=displ_$n leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
	    fpsdisplaysink video-sink=$videosink name=hailo_display$n sync=$sync_pipeline text-overlay=false "
    done
}

function main() {
    init_variables $@
    parse_args $@
    create_sources

    pipeline="gst-launch-1.0 \
           funnel name=fun ! \
           queue name=net max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           hailonet hef-path=$hef_path ! \
           queue name=filter leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           hailofilter so-path=$postprocess_so config-path=$json_config_path qos=false ! \
           queue name=overlay leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           hailooverlay ! \
           queue name=sid_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           streamiddemux name=sid $sources ${additonal_parameters}"

    echo ${pipeline}
    if [ "$print_gst_launch_only" = true ]; then
        exit 0
    fi

    echo "Running Pipeline..."
    eval "${pipeline}"

}

main $@

