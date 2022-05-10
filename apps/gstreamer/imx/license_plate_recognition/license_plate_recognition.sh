#!/bin/bash
set -e

function init_variables() {
    readonly APPS_DIR="/home/root/apps"
    readonly RESOURCES_DIR="$APPS_DIR/license_plate_recognition/resources"
    readonly APP_EXECUTABLE="$RESOURCES_DIR/license_plate_recognition"
}

function print_usage() {
    echo "ARM Detection pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help              Show this help"
    exit 0
}

function parse_args() {
    while test $# -gt 0; do
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage
            exit 0
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

    "${APP_EXECUTABLE}"
}

main $@