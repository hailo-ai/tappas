#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../scripts/misc/checks_before_run.sh

    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/gstreamer/x86/network_switch/resources"
    readonly APP_EXECUTABLE="$RESOURCES_DIR/detection_and_depth_estimation_networks_switch"
    readonly DEFAULT_VIDEO="$RESOURCES_DIR/instance_segmentation.mp4"

    input_source=$DEFAULT_VIDEO
    show_fps=""
}

function print_usage() {
    echo "Multistream Detection hailo - pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                   Show this help"
    echo "  -i INPUT --input INPUT   Set the video source (default $DEFAULT_VIDEO)"
    echo "  --show-fps               Printing fps"
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
        if [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            show_fps="--show-fps"
        elif [ "$1" = "--input" ] || [ "$1" == "-i" ]; then
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


function main() {
    init_variables $@
    parse_args $@

    "${APP_EXECUTABLE}" "${show_fps}" -i "${input_source}"
}

main $@