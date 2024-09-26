#!/bin/bash
set -e

CURRENT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

function init_variables() {
    readonly RESOURCES_DIR="/home/root/apps/basic_security_camera_streaming/resources"
    readonly DEFAULT_VIDEO_SOURCE="/dev/video0"
    readonly DEFAULT_FRONTEND_CONFIG_FILE_PATH="$RESOURCES_DIR/configs/frontend_config_single_stream_4k.json"
    readonly DEFAULT_ENCODER_CONFIG_PATH="$RESOURCES_DIR/configs/encoder_config_4k_single_stream_osd.json"
    readonly DEFAULT_UDP_PORT=5000
    readonly DEFAULT_UDP_HOST_IP="10.0.0.2"
    readonly DEFAULT_BITRATE=25000000
    readonly DEFAULT_FRAMERATE="30/1"

    input_source=$DEFAULT_VIDEO_SOURCE
    encoder_config_path=$DEFAULT_ENCODER_CONFIG_PATH
    frontend_config_file_path=$DEFAULT_FRONTEND_CONFIG_FILE_PATH
    udp_port=$DEFAULT_UDP_PORT
    udp_host_ip=$DEFAULT_UDP_HOST_IP
    framerate=$DEFAULT_FRAMERATE
    

    num_buffers_if_jpeg=10
    property_num_buffers=""

    bitrate=$DEFAULT_BITRATE

    max_buffers_size=5
    sync_pipeline=false
    print_gst_launch_only=false
    additional_parameters=""
}

function print_usage() {
    echo "Hailo15 OSD pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                  Show this help"
    echo "  -i INPUT --input INPUT  Set the camera source (default $input_source)"
    echo "  --show-fps              Print fps"
    echo "  --print-gst-launch      Print the ready gst-launch command without running it"
    echo "  --udp-port              Set the udp port (default $udp_port)"
    echo "  --udp-host-ip           Set the udp host ip (default $udp_host_ip)"
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
            additional_parameters="-v | grep hailo_display"
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

UDP_SINK="udpsink host=$udp_host_ip port=$udp_port"

PIPELINE="gst-launch-1.0 \
        hailofrontendbinsrc config-file-path=$frontend_config_file_path name=frontend \
        frontend. ! \
        queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
        hailoencodebin config-file-path=$encoder_config_path ! h264parse config-interval=-1 ! \
        video/x-h264,framerate=30/1 ! \
        tee name=udp_tee \
        udp_tee. ! \
            queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
            rtph264pay ! 'application/x-rtp, media=(string)video, encoding-name=(string)H264' ! \
            fpsdisplaysink fps-update-interval=2000 video-sink='$UDP_SINK' name=udp_sink sync=$sync_pipeline text-overlay=false \
        udp_tee. ! \
            queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
            fpsdisplaysink fps-update-interval=2000 video-sink=fakesink name=hailo_display sync=$sync_pipeline text-overlay=false \
        ${additional_parameters}"

echo "Running $network_name"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
