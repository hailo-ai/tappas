#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../scripts/misc/checks_before_run.sh

    readonly SRC_0="rtsp://<IP>/?h264x=4 <user-id> <user-pw>"
    readonly SRC_1="rtsp://<IP>/?h264x=4 <user-id> <user-pw>"
    readonly SRC_2="rtsp://<IP>/?h264x=4 <user-id> <user-pw>"
    readonly SRC_3="rtsp://<IP>/?h264x=4 <user-id> <user-pw>"
    readonly SRC_4="rtsp://<IP>/?h264x=4 <user-id> <user-pw>"
    readonly SRC_5="rtsp://<IP>/?h264x=4 <user-id> <user-pw>"
    readonly SRC_6="rtsp://<IP>/?h264x=4 <user-id> <user-pw>"
    readonly SRC_7="rtsp://<IP>/?h264x=4 <user-id> <user-pw>"

    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/gstreamer/general/multistream_detection/resources"
    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/gstreamer/libs/post_processes/"
    readonly POSTPROCESS_SO="$POSTPROCESS_DIR/libyolo_post.so"
    readonly STREAM_DISPLAY_SIZE=640
    readonly HEF_PATH="$RESOURCES_DIR/yolov5m_wo_spp_60p.hef"
    readonly DEFAULT_JSON_CONFIG_PATH="$RESOURCES_DIR/configs/yolov5.json" 

    num_of_src=8
    debug=false
    gst_top_command=""
    additonal_parameters=""
    screen_width=0
    screen_height=0
    fpsdisplaysink_elemet=""
    rtsp_sources=""
    compositor_locations="sink_0::xpos=0 sink_0::ypos=0 sink_1::xpos=640 sink_1::ypos=0 sink_2::xpos=1280 sink_2::ypos=0 sink_3::xpos=1920 sink_3::ypos=0 sink_4::xpos=0 sink_4::ypos=640 sink_5::xpos=640 sink_5::ypos=640 sink_6::xpos=1280 sink_6::ypos=640 sink_7::xpos=1920 sink_7::ypos=640"
    print_gst_launch_only=false
    decode_scale_elements="decodebin ! queue leaky=downstream max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! videoscale n-threads=8 ! video/x-raw,pixel-aspect-ratio=1/1"
    json_config_path=$DEFAULT_JSON_CONFIG_PATH 

    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
}

function print_usage() {
    echo "Multistream Detection RTSP - pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                  Show this help"
    echo "  --debug                 Setting debug mode. using gst-top to print time and memory consuming elements"
    echo "  --show-fps              Printing fps"
    echo "  --num-of-sources NUM    Setting number of rtsp sources to given input (default value is 8)"
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
        if [ "$1" = "--debug" ]; then
            echo "Setting debug mode. using gst-top to print time and memory consuming elements"
            debug=true
            gst_top_command="SHOW_FPS=1 GST_DEBUG=GST_TRACER:13 GST_DEBUG_DUMP_TRACE_DIR=. gst-top-1.0"
        elif [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
        elif [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            additonal_parameters="-v | grep hailo_display"
        elif [ "$1" = "--num-of-sources" ]; then
            shift
            echo "Setting number of rtsp sources to $1"
            num_of_src=$1
        else
            echo "Received invalid argument: $1. See expected arguments below:"
            print_usage
            exit 1
        fi
        shift
    done
}

function create_rtsp_sources() {
    for ((n = 0; n < $num_of_src; n++)); do
        src_name="SRC_${n}"
        src_name="${!src_name}"
        rtsp_sources+="rtspsrc location=$src_name name=source_$n message-forward=true ! \
                          rtph264depay ! \
                          queue name=hailo_preprocess_q_$n leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
                          $decode_scale_elements ! videoconvert n-threads=8 ! \
                          video/x-raw,pixel-aspect-ratio=1/1 ! \
                          fun.sink_$n sid.src_$n ! \
                          queue name=comp_q_$n leaky=downstream max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
                          comp.sink_$n "
    done
}

function configure_fpsdisplaysink_element() {
    ubuntu_version=$(lsb_release -r | awk '{print $2}' | awk -F'.' '{print $1}')

    if [ $ubuntu_version -eq 22 ]; then
        fpsdisplaysink_elemet="fpsdisplaysink video-sink='$video_sink_element window-width=$screen_width window-height=$screen_height'"
    else
        fpsdisplaysink_elemet="fpsdisplaysink video-sink='$video_sink_element' window-width=$screen_width window-height=$screen_height"
    fi
    fpsdisplaysink_elemet+=" name=hailo_display sync=false text-overlay=false"
}

function determine_screen_size() {
    screen_width=(4 * $STREAM_DISPLAY_SIZE)
    if [ "$num_of_src" -lt "4" ]; then
        screen_width=($num_of_src * $STREAM_DISPLAY_SIZE)
    fi
    screen_height=($(($num_of_src+3)) / $STREAM_DISPLAY_SIZE*4)
}

function main() {
    init_variables $@
    parse_args $@

    create_rtsp_sources

    determine_screen_size
    configure_fpsdisplaysink_element

    pipeline="$gst_top_command gst-launch-1.0 \
         funnel name=fun ! \
         queue name=hailo_pre_infer_q_0 leaky=downstream max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
         hailonet  hef-path=$HEF_PATH ! \
         queue name=hailo_postprocess0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
         hailofilter so-path=$POSTPROCESS_SO config-path=$json_config_path qos=false ! \
         queue name=hailo_draw0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
         hailooverlay ! streamiddemux name=sid \
         compositor name=comp start-time-selection=0 $compositor_locations ! videoscale n-threads=8 name=disp_scale ! video/x-raw,width=$screen_width,height=$screen_height ! \
         $fpsdisplaysink_elemet \
         $rtsp_sources ${additonal_parameters}"

    echo ${pipeline}
    if [ "$print_gst_launch_only" = true ]; then
        exit 0
    fi

    echo "Running Pipeline..."
    eval "${pipeline}"

    if [ "$debug" = true ]; then
        if [ -f "gst-top.gsttrace" ]; then
            gst-report-1.0 gst-top.gsttrace >pipeline_report.txt
            gst-report-1.0 --dot gst-top.gsttrace | dot -Tsvg >pipeline_graph.svg
            echo "Most time and memory consuming elements:"
            cat pipeline_report.txt | head -n 4
            echo "Full pipeline report saved to pipeline_report.txt and pipeline_graph.svg"
        else
            echo "Pipeline report creation failed"
        fi
    fi
}

main $@
