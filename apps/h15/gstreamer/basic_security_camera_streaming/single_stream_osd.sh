#!/bin/bash
set -e

CURRENT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

function init_variables() {
    readonly RESOURCES_DIR="/home/root/apps/detection/resources"
    readonly DEFAULT_VIDEO_SOURCE="/dev/video0"
    readonly DEFAULT_JSON_CONFIG_PATH="$RESOURCES_DIR/configs/osd.json"
    readonly DEFAULT_FRONTEND_CONFIG_FILE_PATH="$RESOURCES_DIR/configs/frontend_config.json"
    readonly DEFAULT_UDP_PORT=5000
    readonly DEFAULT_UDP_HOST_IP="10.0.0.2"
    readonly DEFAULT_BITRATE=25000000
    readonly DEFAULT_FRAMERATE="30/1"

    input_source=$DEFAULT_VIDEO_SOURCE
    json_config_path=$DEFAULT_JSON_CONFIG_PATH
    frontend_config_file_path=$DEFAULT_FRONTEND_CONFIG_FILE_PATH
    udp_port=$DEFAULT_UDP_PORT
    udp_host_ip=$DEFAULT_UDP_HOST_IP
    framerate=$DEFAULT_FRAMERATE
    

    num_buffers_if_jpeg=15
    property_num_buffers=""
    jpeg_n_threads=3

    bitrate=$DEFAULT_BITRATE

    use_hailojpeg=false
    max_buffers_size=5
    sync_pipeline=false
    print_gst_launch_only=false
    additional_parameters=""

    # Limit the encoding bitrate to 10Mbps to support weak host.
    # Comment this out if you encounter a large latency in the host side.
    # Tune the value down to reach the desired latency (will decrease the video quality).
    # ----------------------------------------------
    # bitrate=10000000
    # ----------------------------------------------
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
    echo "  --use-hailojpeg         Use Hailo multithreaded jpeg encoding, in that case the jpeg frames will be saved in current directory and number of buffers is set to $num_buffers_if_jpeg"
    exit 0
}

function parse_args() {
    while test $# -gt 0; do
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage
            exit 0
        elif [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
        elif [ "$1" = "--use-hailojpeg" ]; then
            use_hailojpeg=true
            property_num_buffers="num-buffers=$num_buffers_if_jpeg"
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
PIPELINE=""
BASE_PIPELINE="v4l2src device=$input_source io-mode=mmap $property_num_buffers ! video/x-raw,format=NV12,width=3840,height=2160, framerate=$framerate ! \
    queue leaky=downstream max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
    hailofrontend config-file-path=$frontend_config_file_path name=frontend \
    frontend. ! \
    queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
    hailoosd config-path=$json_config_path ! \
    queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 "

if [ "$use_hailojpeg" = true ]; then
    PIPELINE="gst-launch-1.0 \
        $BASE_PIPELINE ! \
        hailojpegenc n-threads=$jpeg_n_threads ! 
        tee name=jpeg_tee \
        jpeg_tee. ! \
            queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
            multifilesink location=frame%d.jpg \
        jpeg_tee. ! \
            queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
            fpsdisplaysink video-sink=fakesink name=hailo_display sync=$sync_pipeline text-overlay=false \
        ${additional_parameters}"
else
    PIPELINE="gst-launch-1.0 \
        $BASE_PIPELINE ! \
        hailoh264enc bitrate=$bitrate ! h264parse config-interval=-1 ! \
        video/x-h264,framerate=30/1 ! \
        tee name=udp_tee \
        udp_tee. ! \
            queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
            rtph264pay ! 'application/x-rtp, media=(string)video, encoding-name=(string)H264' ! \
            fpsdisplaysink video-sink='$UDP_SINK' name=udp_sink sync=$sync_pipeline text-overlay=false \
        udp_tee. ! \
            queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
            fpsdisplaysink video-sink=fakesink name=hailo_display sync=$sync_pipeline text-overlay=false \
        ${additional_parameters}"
fi

echo "Running $network_name"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
