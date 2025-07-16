#!/bin/bash
set -e

function print_usage() {
    echo "Udp receive:"
    echo ""
    echo "Options:"
    echo "  --help                  Show this help"
    echo "  -i INPUT --input INPUT  Set the camera source (default $input_source)"
    echo "  --show-fps              Print fps"
    echo "  --time-to-run           Set the time to run the pipeline (default 60 seconds)"
    echo "  --udp-port              Set the udp port (default $udp_port)"
    echo "  --encode-type          Set the encode type [h265,h264] (default $encode_type)"
    echo "  --gst-log-file          Set the gst log file (default $gst_log_file)"
    echo "  --print-gst-launch      Print the ready gst-launch command without running it"
    exit 0
}

time_to_run=60
udp_port=5000
encode_type="h265"
bitrate_element_filter="udpsrc"
latency_element_filter="sink"
gst_log_file="/tmp/gst.log"
gst_tracers="bitrate;framerate;interlatency"

function parse_args() {
    while test $# -gt 0; do
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage
            exit 0
        elif [ "$1" = "--time-to-run" ] || [ "$1" == "-t" ]; then
            time_to_run=$2
            shift
        elif [ "$1" = "--udp-port" ] || [ "$1" == "-p" ]; then
            udp_port=$2
            shift
        elif [ "$1" = "--gst-log-file" ]; then
            gst_log_file=$2
            shift
        elif [ "$1" = "--tracers" ]; then
            gst_tracers=$2
            shift
        elif [ "$1" = "--encode-type" ]; then
            encode_type=$2
            shift
        else
            echo "Received invalid argument: $1. See expected arguments below:"
            print_usage
            exit 1
        fi

        shift
    done
}


parse_args $@

#check encode type
if [ "$encode_type" == "h265" ]; then
    pipeline="gst-launch-1.0 -v udpsrc port=$udp_port ! application/x-rtp,encoding-name=H265 ! queue ! rtph265depay ! queue ! h265parse ! avdec_h265 ! queue ! videoconvert n-threads=8 ! fpsdisplaysink text-overlay=false sync=false"
elif [ "$encode_type" == "h264" ]; then
    pipeline="gst-launch-1.0 -v udpsrc port=$udp_port ! application/x-rtp,encoding-name=H264 ! queue ! rtph264depay ! queue ! h264parse ! avdec_h264 ! queue ! videoconvert n-threads=8 ! fpsdisplaysink text-overlay=false sync=false"
else
    echo "encode type $encode_type is not supported"
    exit 1
fi


echo $pipeline
GST_DEBUG_FILE="$gst_log_file" GST_DEBUG="GST_TRACER:7" GST_TRACERS="$gst_tracers" timeout $time_to_run $pipeline