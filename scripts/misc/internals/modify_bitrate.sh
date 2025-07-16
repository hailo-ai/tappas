#!/bin/bash

set -e

# Check if FFmpeg is installed
command -v ffmpeg >/dev/null 2>&1 || { echo >&2 "FFmpeg is required but it's not installed. please run 'sudo apt install ffmpeg'"; exit 1; }

# Set default values
DEFAULT_BITRATE=2874933
DEFAULT_FRAMERATE=25

bitrate=$DEFAULT_BITRATE
framerate=$DEFAULT_FRAMERATE
file_path=""
file_output=""

function print_usage(){
    echo "Usage: modify_bitrate.sh [options]"
    echo "Options:"
    echo "  --file-path: Sets the path of the input file (mandatory)"
    echo "  --file-output: Sets the path of the output file (mandatory)"
    echo "  --bitrate: Sets the bitrate of the output file (default: $DEFAULT_BITRATE)"
    echo "  --framerate: Sets the framerate of the output file (default: $DEFAULT_FRAMERATE)"
}

# Parse command line arguments
function parse_args() {
    while test $# -gt 0; do
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage
            exit 0
        elif [ "$1" = "--bitrate" ]; then
            shift
            echo "Setting bitrate $1"
            bitrate=$1
        elif [ "$1" = "--file-path" ]; then
            shift
            echo "Setting file path $1"
            file_path=$1
        elif [ "$1" = "--file-output" ]; then
            shift
            echo "Setting file output $1"
            file_output=$1
        elif [ "$1" = "--framerate" ]; then
            shift
            echo "Setting framerate $1"
            framerate=$1
        else
            echo "Received invalid argument: $1. See expected arguments below:"
            print_usage
            exit 1
        fi
        shift
    done
}

# Check if mandatory arguments are set
function check_args() {
    if [ -z "$file_path" ]; then
        echo "Please specify the path of the input file"
        print_usage
        exit 1
    fi

    if [ -z "$file_output" ]; then
        echo "Please specify the path of the output file"
        print_usage
        exit 1
    fi
}

parse_args $@
check_args

echo "Running ffmpeg -i $file_path -c:v libx264 -b:v $bitrate -r $framerate $file_output"

# Run FFmpeg pipeline
ffmpeg -i $file_path -c:v libx264 -b:v $bitrate -r $framerate $file_output