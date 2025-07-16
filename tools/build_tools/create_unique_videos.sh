#!/bin/bash
set -e

STEPS=8
SOURCE_VIDEO=""

function print_usage() {
    echo "Create unique videos:"
    echo ""
    echo "Options:"
    echo "  --help                  Show this help"
    echo "  --input                 Path to the input file"
    echo "  --num-of-output-videos  The amount of videos to produce (default = $STEPS)"
    exit 1
}

function parse_args() {
    while test $# -gt 0; do
        if [[ "$1" == "-h" || "$1" == "--help" ]]; then
            print_usage
        elif [ "$1" == "--input" ]; then
            SOURCE_VIDEO=$2
            shift
        elif [ "$1" == "--num-of-output-videos" ]; then
            STEPS=$2
            shift
        else
            echo "Unknown parameters, exiting"
            print_usage
        fi
        shift
    done

    if [[ -f "$SOURCE_VIDEO" ]] ; then
        echo "Input file is required, exiting"
        exit 1
    fi
}

function main() {
    for ((n = 0; n < $STEPS; n++)); do
        # The range of the hue is [-1..1]
        # therefore, we would start from the min [-1] and jump by [2 / num_of_steps] each time
        hue_value=$(python3 -c "print(-1 + (( 2 / $STEPS) * $n))")
        gst-launch-1.0 filesrc location=$SOURCE_VIDEO ! queue ! decodebin ! queue ! videobalance hue=$hue_value !  queue ! x264enc ! mp4mux ! queue ! filesink location="output_${n}.mp4" sync=false

    done
}

parse_args "$@"
main

