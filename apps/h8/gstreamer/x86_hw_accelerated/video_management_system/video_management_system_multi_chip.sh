#!/bin/bash
set -e

readonly VMS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/x86_hw_accelerated/video_management_system"

DEFAULT_NUMBER_OF_SOURCES=16
MAX_NUMBER_OF_SOURCES=32

# Use the video_management_system_base.sh configurations
source "$VMS_DIR/video_management_system_base.sh"

function configure_default_streams() {
    person_attribute_streams="sink_0,sink_3,sink_6,sink_7,sink_8,sink_9,sink_12,sink_13,sink_16,sink_17,sink_20,sink_21,sink_24,sink_25,sink_28,sink_29,sink_30,sink_31"
    face_attribute_streams="sink_1,sink_4,sink_10,sink_14,sink_18,sink_22,sink_26"
    face_recognition_streams="sink_2,sink_5,sink_11,sink_15,sink_19,sink_23,sink_27"
}

function create_hailo_device_configurations() {
    FACE_DETECTION_HAILO_DEVICE_CONF="scheduling-algorithm=1 vdevice-key=2 device-count=2 batch-size=8"
    PERSON_DETECTION_HAILO_DEVICE_CONF="scheduling-algorithm=1 vdevice-key=2 device-count=2 batch-size=8"
    FACE_RECOGNITION_HAILO_DEVICE_CONF="scheduling-algorithm=1 vdevice-key=1 device-count=1"
    FACE_ATTR_HAILO_DEVICE_CONF="scheduling-algorithm=1 vdevice-key=1 device-count=1"
    PERSON_ATTR_HAILO_DEVICE_CONF="scheduling-algorithm=1 vdevice-key=1 device-count=1"
}

function create_compositor_table(){
    # create a compositor table of 6 rows and 6 columns of frame size 300X300
    comp_row0="sink_0::xpos=0 sink_0::ypos=0 sink_1::xpos=300 sink_1::ypos=0 sink_2::xpos=600 sink_2::ypos=0 sink_3::xpos=900 sink_3::ypos=0 sink_4::xpos=1200 sink_4::ypos=0 sink_5::xpos=1500 sink_5::ypos=0"
    comp_row1="sink_6::xpos=0 sink_6::ypos=300 sink_7::xpos=300 sink_7::ypos=300 sink_8::xpos=600 sink_8::ypos=300 sink_9::xpos=900 sink_9::ypos=300 sink_10::xpos=1200 sink_10::ypos=300 sink_11::xpos=1500 sink_11::ypos=300"
    comp_row2="sink_12::xpos=0 sink_12::ypos=600 sink_13::xpos=300 sink_13::ypos=600 sink_14::xpos=600 sink_14::ypos=600 sink_15::xpos=900 sink_15::ypos=600 sink_16::xpos=1200 sink_16::ypos=600 sink_17::xpos=1500 sink_17::ypos=600"
    comp_row3="sink_18::xpos=0 sink_18::ypos=900 sink_19::xpos=300 sink_19::ypos=900 sink_20::xpos=600 sink_20::ypos=900 sink_21::xpos=900 sink_21::ypos=900 sink_22::xpos=1200 sink_22::ypos=900 sink_23::xpos=1500 sink_23::ypos=900"
    comp_row4="sink_24::xpos=0 sink_24::ypos=1200 sink_25::xpos=300 sink_25::ypos=1200 sink_26::xpos=600 sink_26::ypos=1200 sink_27::xpos=900 sink_27::ypos=1200 sink_28::xpos=1200 sink_28::ypos=1200 sink_29::xpos=1500 sink_29::ypos=1200"
    comp_row5="sink_30::xpos=0 sink_30::ypos=1500 sink_31::xpos=300 sink_31::ypos=1500 sink_32::xpos=600 sink_32::ypos=1500 sink_33::xpos=900 sink_33::ypos=1500 sink_34::xpos=1200 sink_34::ypos=1500 sink_35::xpos=1500 sink_35::ypos=1500"
    compositor_locations="$comp_row0 $comp_row1 $comp_row2 $comp_row3 $comp_row4 $comp_row5"
    display_width=300
    display_height=300
}

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
    eval "HAILO_ENABLE_MULTI_DEVICE_SCHEDULER=1 ${pipeline}"
}

main $@