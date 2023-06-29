#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh

    readonly SRC_0="rtsp://<IP>/?h264x=4 <user-id> <user-pw>"
    readonly SRC_1="rtsp://<IP>/?h264x=4 <user-id> <user-pw>"
    readonly SRC_2="rtsp://<IP>/?h264x=4 <user-id> <user-pw>"
    readonly SRC_3="rtsp://<IP>/?h264x=4 <user-id> <user-pw>"
    readonly SRC_4="rtsp://<IP>/?h264x=4 <user-id> <user-pw>"
    readonly SRC_5="rtsp://<IP>/?h264x=4 <user-id> <user-pw>"
    readonly SRC_6="rtsp://<IP>/?h264x=4 <user-id> <user-pw>"
    readonly SRC_7="rtsp://<IP>/?h264x=4 <user-id> <user-pw>"

    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/rockchip/multistream_detection/resources"
    readonly CONFIGS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/rockchip/multistream_detection/resources/configs"
    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes/"
    readonly POSTPROCESS_SO="$POSTPROCESS_DIR/libyolo_hailortpp_post.so"
    readonly HEF_PATH="$RESOURCES_DIR/yolov5s_nv12.hef"
    readonly OVERLAY_PIPELINE="queue name=hailo_stream leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! hailooverlay !"
    readonly DEFAULT_IP_ADDRESS="127.0.0.1"

    num_of_src=10
    additonal_parameters=""
    sources=""
    streamrouter_input_streams=""
    print_gst_launch_only=false
    overlay_element=""
    ip_address=$DEFAULT_IP_ADDRESS
}

function print_usage() {
    echo "Multistream Detection hailo - pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                          Show this help"
    echo "  --num-of-sources NUM            Setting number of sources to given input (default value is 10, maximum value is 10)"
    echo "  --print-gst-launch              Print the ready gst-launch command without running it"
    echo "  --use-overlay                   If set the pipeline will run overlay over the outgoing streams (affect performance)."
    echo "  --udp-sink-ip IP_ADDRESS          Sets the ip for streaming out the RTSP, by default $DEFAULT_IP_ADDRESS ."
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
        elif [ "$1" = "--num-of-sources" ]; then
            shift
            echo "Setting number of sources to $1"
            num_of_src=$1
        elif [ "$1" = "--use-overlay" ]; then
            echo "Using overlay over the output image"
            overlay_element=$OVERLAY_PIPELINE
        elif [ "$1" = "--udp-sink-ip" ]; then
            shift
            echo "Sending the output streams to ip $1"
            ip_address=$1
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
        src_name="SRC_${n}"
        src_name="${!src_name}"
        sources+="rtspsrc location=$src_name name=source_$n message-forward=true ! \
                  rtph264depay ! \
                  queue name=hailo_before_parse_$n leaky=no max-size-buffers=2 max-size-bytes=0 max-size-time=0 ! \
                  h264parse ! \
                  queue name=hailo_before_mpp_$n leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! \
                  mppvideodec arm-afbc=false ! \
                  queue name=hailo_before_funnel_$n leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! \
                  fun.sink_$n sid.src_$n ! \
                  queue name=before_export_$n leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! \
                  hailoexportfile location='$TAPPAS_WORKSPACE/metadata_$n.json' name=export_$n ! \
                  queue name=enc_$n leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! \
                  mpph264enc header-mode=1 rc-mode=0 profile=high level=40 max-reenc=0 ! \
                  queue name=comp_rtp_$n leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! \
                  rtph264pay mtu=2800 aggregate-mode=2 config-interval=10 pt=96 ! udpsink host=$ip_address port=500$n sync=true "
        streamrouter_input_streams+=" src_$n::input-streams=\"<sink_$n>\""
    done
}

function main() {
    init_variables $@
    parse_args $@
    create_sources
    pipeline="gst-launch-1.0 \
             hailoroundrobin mode=1 name=fun ! \
             queue name=hailo_pre_tee_q_0 leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
             tee name=t hailomuxer name=hmux \
             t. ! queue name=bypass leaky=no max-size-buffers=20 max-size-bytes=0 max-size-time=0 ! hmux. \
             t. ! queue name=hailo_pre_video_q_0 leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
             videoscale qos=false !  \
             video/x-raw, format=NV12, width=640, height=640 ! \
             queue name=hailo_pre_infer_q_0 leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
             hailonet hef-path=$HEF_PATH is-active=true batch-size=8 ! \
             queue name=hailo_postprocess0 leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
             hailofilter so-path=$POSTPROCESS_SO qos=false ! \
             hmux. hmux. ! \
             $overlay_element \
             queue name=hailo_over leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
             hailostreamrouter name=sid $streamrouter_input_streams \
             $sources "

    echo ${pipeline}
    if [ "$print_gst_launch_only" = true ]; then
        exit 0
    fi

    echo "Running Pipeline..."
    eval "${pipeline}"

}

main $@
