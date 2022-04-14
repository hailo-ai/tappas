#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../scripts/misc/checks_before_run.sh

    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/gstreamer/x86/multistream_multidevice/resources"
    readonly DETECTION_HEF_PATH="$RESOURCES_DIR/yolov5m.hef"
    readonly POSE_ESTIMATION_HEF_PATH="$RESOURCES_DIR/centerpose_regnetx_1.6gf_fpn.hef"

    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/gstreamer/x86/libs/"
    readonly DETECTION_POSTPROCESS_SO="$POSTPROCESS_DIR/libyolo_post.so"
    readonly POSE_ESTIMATION_POSTPROCESS_SO="$POSTPROCESS_DIR/libcenterpose_post.so"
    readonly POSE_ESTIMATION_POSTPROCESS_FUNCTION_NAME="centerpose"
    readonly DETECTION_POSTPROCESS_FUNCTION_NAME="yolov5_no_persons"
    readonly DRAW_SO="$POSTPROCESS_DIR/libdetection_draw.so"

    num_of_src=8
    debug=false
    gst_top_command=""
    additonal_parameters=""
    decode_scale_elements="decodebin ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! videoscale n-threads=8 ! video/x-raw,pixel-aspect-ratio=1/1"
    compositor_locations="sink_0::xpos=0 sink_0::ypos=0 sink_1::xpos=640 sink_1::ypos=0 sink_2::xpos=1280 sink_2::ypos=0 sink_3::xpos=1920 sink_3::ypos=0 sink_4::xpos=0 sink_4::ypos=640 sink_5::xpos=640 sink_5::ypos=640 sink_6::xpos=1280 sink_6::ypos=640 sink_7::xpos=1920 sink_7::ypos=640"
    print_gst_launch_only=false
    stats_elements=""
    debug_stats_export=""

    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
}

function print_usage() {
    echo "Video Detection and Pose Estimation - pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                          Show this help"
    echo "  --debug                         Setting debug mode. using gst-top to print time and memory consuming elements"
    echo "  --show-fps                      Printing fps"
    echo "  --num-of-sources NUM            Setting number of video sources to given input (default value is 8)"
    echo "  --print-gst-launch              Print the ready gst-launch command without running it"
    echo "  --print-devices-stats           Print the power and temperature measured"
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
            additonal_parameters="-v 2>&1 | grep -e hailo_display -e hailodevicestats"
        elif [ "$1" = "--print-devices-stats" ]; then
            stats_elements="hailodevicestats device-id=${DEVICE1} hailodevicestats device-id=${DEVICE2}"
            debug_stats_export="GST_DEBUG=hailodevicestats:5"
        elif [ "$1" = "--num-of-sources" ]; then
            shift
            echo "Setting number of video sources to $1"
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
        input_source="$RESOURCES_DIR/detection${n}.mp4"
        input_sources+="filesrc location=$input_source ! \
                          queue name=hailo_preprocess_q_$n leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                          $decode_scale_elements ! videoconvert n-threads=8 ! \
                          video/x-raw,pixel-aspect-ratio=1/1 ! \
                          fun.sink_$n sid.src_$n ! \
                          queue name=comp_q_$n leaky=downstream max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                          comp.sink_$n "
    done
}

function autodetect_devices() {
    devices="$(hailortcli scan | awk '{ print $NF }' | tail -n+2)"
    DEVICE1="$(echo ${devices} | awk '{ print $1 }')"
    DEVICE2="$(echo ${devices} | awk '{ print $2 }')"
}

function main() {
    init_variables $@
    parse_args $@
    autodetect_devices
    create_sources

    pipeline="$gst_top_command ${debug_stats_export} gst-launch-1.0 ${stats_elements} \
         funnel name=fun ! queue name=hailo_pre_split leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! tee name=splitter \
         hailomuxer name=hailomuxer ! queue name=hailo_draw0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
         hailofilter so-path=$DRAW_SO qos=false debug=False ! streamiddemux name=sid \
         splitter. ! queue name=hailo_pre_infer_q_1 leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
         hailonet hef-path=$DETECTION_HEF_PATH debug=False qos=false is-active=true device-id=$DEVICE1 ! \
         queue name=hailo_postprocess0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
         hailofilter so-path=$DETECTION_POSTPROCESS_SO function-name=$DETECTION_POSTPROCESS_FUNCTION_NAME qos=false debug=False ! hailomuxer. \
         splitter. ! queue name=hailo_pre_infer_q_0 leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
         hailonet hef-path=$POSE_ESTIMATION_HEF_PATH debug=False qos=false is-active=true device-id=$DEVICE2 ! \
         queue name=hailo_postprocess1 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
         hailofilter so-path=$POSE_ESTIMATION_POSTPROCESS_SO function-name=$POSE_ESTIMATION_POSTPROCESS_FUNCTION_NAME qos=false debug=False ! hailomuxer. \
         compositor name=comp start-time-selection=0 $compositor_locations ! \
	     queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
         fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=false text-overlay=false \
         $input_sources ${additonal_parameters}"

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
