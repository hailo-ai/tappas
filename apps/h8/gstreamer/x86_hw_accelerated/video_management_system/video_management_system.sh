#!/bin/bash
set -e

readonly VMS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/x86_hw_accelerated/video_management_system"

DEFAULT_NUMBER_OF_SOURCES=8
MAX_NUMBER_OF_SOURCES=8

# Use the video_management_system_base.sh configurations
source "$VMS_DIR/video_management_system_base.sh"


function main() {
    init_variables $@
    parse_args $@
    init_variables_based_on_video_format
    validate_args

    create_hailo_device_configurations

    create_pipeline_structure

    echo ${pipeline}
    if [ "$print_gst_launch_only" = true ]; then
        exit 0
    fi

    echo "Running Pipeline..."
    eval "${pipeline}"
}

main $@