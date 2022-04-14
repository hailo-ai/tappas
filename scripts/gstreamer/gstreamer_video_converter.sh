#!/bin/bash
set -e


input_source=""
output_file=""
function print_usage() {
    echo "Gstreamer video conversion usage:"
    echo ""
    echo "Options:"
    echo "  --help              Show this help"
    echo "  -i INPUT --input INPUT         Set the video source (required)"
    echo "  -o OUTPUT --output OUTPUT         Set the video output file (required)"
    exit 0
}

function parse_args() {
    while test $# -gt 0; do
        if [ "$1" == "--help" ] || [ "$1" == "-h" ]; then
            print_usage
            exit 0
        elif [ "$1" == "--input" ] || [ "$1" == "-i" ]; then
            input_source="$2"
            shift
        elif [ "$1" == "--output" ] || [ "$1" == "-o" ]; then
            output_file="$2"
            shift
        else
            echo "Received invalid argument: $1. See expected arguments below:"
            print_usage
            exit 1
        fi
        
        shift
    done
    if [ "$input_source" == "" ] || [ "$output_file" == "" ]; then
        echo "Input argument is required"
        print_usage
        exit 1
    fi
}

parse_args $@

apt-get -y --no-install-recommends install gstreamer1.0-plugins-ugly
PIPELINE="gst-launch-1.0 \
    filesrc location=$input_source name=src_0 ! decodebin ! queue ! \
    videoconvert ! x264enc ! h264parse config-interval=-1 ! \
    video/x-h264,alignment=nal,stream-format=byte-stream ! filesink location=$output_file"

echo $PIPELINE
echo "Running"
eval ${PIPELINE}
