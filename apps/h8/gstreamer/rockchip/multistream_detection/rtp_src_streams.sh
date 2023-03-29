#!/bin/bash
set -e

readonly DEFAULT_IP="0.0.0.0"
readonly DEFAULT_PORT="5000"
readonly DEFAULT_CAPS="'application/x-rtp,format=NV12,width=1920,height=1080'"

ip_address=$DEFAULT_IP
port=$DEFAULT_PORT
caps=$DEFAULT_CAPS

function print_usage() {
    echo "Test RTP stream - pipeline usage:"
    echo "should be run as follow ./rtp_src_streams --host-ip IP_ADDRESS --port PORT --caps CAPS_STRING"
    echo ""
    echo "Options:"
    echo "  --help                          Show this help"
    echo "  --host-ip IP_ADDRESS            Change the host to where to get the rtp stream (default $DEFAULT_IP)"
    echo "  --port PORT                     Set the port to listen for the rtp stream (default $DEFAULT_PORT)"
    echo "  --caps CAPS_STRING              Set the input stream caps in order for correct decode (default $DEFAULT_CAPS)"
    exit 0
}

function print_help_if_needed() {
    while test $# -gt 0; do
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage
        elif [ "$1" = "--host-ip" ]; then
            ip_address="$2"
            shift
        elif [ "$1" = "--port" ]; then
            port="$2"
            shift
        elif [ "$1" = "--caps" ]; then
            caps="$2"
            shift
        else
            echo "Received invalid argument: $1. See expected arguments below:"
            print_usage
            exit 1
        fi

        shift
    done
}

print_help_if_needed $@


eval "gst-launch-1.0 \
      udpsrc address=$ip_address port=$port caps=$caps ! \
      rtph264depay ! \
      decodebin ! \
      queue max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
      fpsdisplaysink video-sink=autovideosink sync=false"