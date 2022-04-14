#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../scripts/misc/checks_before_run.sh

    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/gstreamer/x86/license_plate_recognition/resources"
    readonly APP_EXECUTABLE="$RESOURCES_DIR/license_plate_recognition"
    readonly DEFAULT_VIDEO="$RESOURCES_DIR/lpr_ayalon_loop.mp4"

    input_source=$DEFAULT_VIDEO
    show_fps=""
    required_gstreamer_version="1.14"
}

function print_usage() {
    echo "Multistream Detection hailo - pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                   Show this help"
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
        else
            echo "Received invalid argument: $1. See expected arguments below:"
            print_usage
            exit 1
        fi
        shift
    done
}

function check_gst_version() {
    echo "Checking for supported GStreamer version, found:"
    cmd_output=$(gst-inspect-1.0 --gst-version | awk '{print $NF}' | awk -F'.' '{print $1"."$2}' 2>&1)
    echo ${cmd_output}
    if [[ "$cmd_output" != "$required_gstreamer_version" ]]; then
        echo "GStreamer version does not support this application, version required: ${required_gstreamer_version}"
        exit 1
    fi
}

function main() {
    init_variables $@
    parse_args $@

    rm -f $TAPPAS_WORKSPACE/lp_*.jpg

    check_gst_version

    "${APP_EXECUTABLE}" "${show_fps}" -i "${input_source}"
}

main $@