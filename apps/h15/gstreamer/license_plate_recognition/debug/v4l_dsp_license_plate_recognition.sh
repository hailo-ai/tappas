#!/bin/bash
set -e

CURRENT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

function init_variables() {
    print_help_if_needed $@

    # Basic Directories
    readonly POSTPROCESS_DIR="/usr/lib/hailo-post-processes"
    readonly CROPPING_ALGORITHMS_DIR="$POSTPROCESS_DIR/cropping_algorithms"
    readonly RESOURCES_DIR="${CURRENT_DIR}/resources"
    readonly DEFAULT_LICENSE_PLATE_JSON_CONFIG_PATH="$RESOURCES_DIR/configs/yolov4_license_plate.json"
    readonly DEFAULT_VEHICLE_JSON_CONFIG_PATH="$RESOURCES_DIR/configs/yolov5_vehicle_detection.json"
    readonly DEBUG_SO="$POSTPROCESS_DIR/libdebug.so"
    readonly DEBUG_FUNCTION="generate_center_detection"
    # Default Video

    readonly DEFAULT_VIDEO_SOURCE="$RESOURCES_DIR/lpr.raw"

    # Vehicle Detection Macros
    readonly VEHICLE_DETECTION_HEF="$RESOURCES_DIR/yolov5m_vehicles.hef"
    readonly VEHICLE_DETECTION_POST_SO="$POSTPROCESS_DIR/libyolo_post.so"
    readonly VEHICLE_DETECTION_POST_FUNC="yolov5_vehicles_only"


    # License Plate Detection Macros
    readonly LICENSE_PLATE_DETECTION_HEF="$RESOURCES_DIR/tiny_yolov4_license_plates.hef"
    readonly LICENSE_PLATE_DETECTION_POST_SO="$POSTPROCESS_DIR/libyolo_post.so"
    readonly LICENSE_PLATE_DETECTION_POST_FUNC="tiny_yolov4_license_plates"

    # License Plate OCR Macros
    readonly LICENSE_PLATE_OCR_HEF="$RESOURCES_DIR/lprnet.hef"
    readonly LICENSE_PLATE_OCR_POST_SO="$POSTPROCESS_DIR/libocr_post.so"

    # Cropping Algorithm Macros
    readonly LICENSE_PLATE_CROP_SO="$CROPPING_ALGORITHMS_DIR/liblpr_croppers.so"
    readonly LICENSE_PLATE_DETECTION_CROP_FUNC="vehicles_without_ocr"
    readonly LICENSE_PLATE_OCR_CROP_FUNC="license_plate_quality_estimation"

    readonly WHOLE_BUFFER_CROP_SO="$CROPPING_ALGORITHMS_DIR/libwhole_buffer.so"

    # Pipeline Utilities
    readonly LPR_OVERLAY="$RESOURCES_DIR/liblpr_overlay.so"
    readonly LPR_OCR_SINK="$RESOURCES_DIR/liblpr_ocrsink.so"

    input_source=$DEFAULT_VIDEO_SOURCE

    print_gst_launch_only=false
    additonal_parameters=""
    stats_element=""
    debug_stats_export=""
    sync_pipeline=true
    device_id_prop=""
    tee_name="context_tee"
    internal_offset=false
    pipeline_1=""
    license_plate_json_config_path=$DEFAULT_LICENSE_PLATE_JSON_CONFIG_PATH
    car_json_config_path=$DEFAULT_VEHICLE_JSON_CONFIG_PATH 
}

function print_help_if_needed() {
    while test $# -gt 0; do
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage
        fi

        shift
    done
}

function print_usage() {
    echo "IMX8 LPR pipeline usage:"
    echo ""
    echo "Options:"
    echo "  -h --help                  Show this help"
    echo "  --show-fps                 Print fps"
    echo "  --print-gst-launch         Print the ready gst-launch command without running it"
    echo "  --print-device-stats       Print the power and temperature measured"
    exit 0
}

function parse_args() {
    while test $# -gt 0; do
        if [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
        elif [ "$1" = "--print-device-stats" ]; then
            hailo_bus_id=$(hailortcli scan | awk '{ print $NF }' | tail -n 1)
            device_id_prop="device_id=$hailo_bus_id"
            stats_element="hailodevicestats $device_id_prop"
            debug_stats_export="GST_DEBUG=hailodevicestats:5"
        elif [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            additonal_parameters="-v | grep -e hailo_display "
        else
            echo "Received invalid argument: $1. See expected arguments below:"
            print_usage
            exit 1
        fi

        shift
    done
}

init_variables $@
parse_args $@
source_element="v4l2src io-mode=mmap device=/dev/video0 name=src_0 ! video/x-raw, width=1920, height=1080, format=RGB"
internal_offset=true

function create_lp_detection_pipeline() {
    DUMP_IMAGE_TO_FILE_TEE="tee name=test_tee \
        test_tee. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        pngenc ! \
        multifilesink location=image.png \
        test_tee. "

    UDPSINK="videoscale ! video/x-raw, width=300, height=300 ! \
            queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            videoconvert ! video/x-raw, format=I420 ! \
            queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            rtpvrawpay ! 'application/x-rtp, media=(string)video, encoding-name=(string)RAW' ! udpsink host=10.0.0.2 port=5000 sync=false"

    UDPSINK2="videoconvert ! video/x-raw, format=I420 ! \
            queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            rtpvrawpay ! 'application/x-rtp, media=(string)video, encoding-name=(string)RAW' ! udpsink host=10.0.0.2 port=5002 sync=false"

    DUMP_IMAGE_TO_UDP_TEE="tee name=test_tee \
        test_tee. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        $UDPSINK \
        test_tee. "

    pipeline_1="queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                hailocropper pool-size=12 so-path=$WHOLE_BUFFER_CROP_SO function-name=create_crops internal-offset=$internal_offset name=cropper1 \
                hailoaggregator name=agg1 \
                cropper1. ! \
                    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                agg1. \
                cropper1. ! \
                    video/x-raw,format=RGB, width=640, height=640, pixel-aspect-ratio=1/1 ! \
                    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                agg1. \
                agg1. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0"

    pipeline_2="queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                hailocropper pool-size=12 so-path=$WHOLE_BUFFER_CROP_SO function-name=create_crops internal-offset=$internal_offset name=cropper2 \
                hailoaggregator name=agg2 \
                cropper2. ! \
                    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                agg2. \
                cropper2. ! \
                    video/x-raw,format=RGB, width=300, height=300, pixel-aspect-ratio=1/1 ! \
                    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                    $DUMP_IMAGE_TO_UDP_TEE ! \
                agg2. \
                agg2. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0"

    pipeline_3="queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                hailocropper pool-size=12 so-path=$WHOLE_BUFFER_CROP_SO function-name=create_crops internal-offset=$internal_offset name=cropper3 \
                hailoaggregator name=agg3 \
                cropper3. ! \
                    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                agg3. \
                cropper3. ! \
                    video/x-raw,format=RGB, width=416, height=416, pixel-aspect-ratio=1/1 ! \
                    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                agg3. \
                agg3. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0"
}
create_lp_detection_pipeline $@

FPS_DISP="fpsdisplaysink name=hailo_display sync=true video-sink=fakesink text-overlay=false"

PIPELINE="${debug_stats_export} gst-launch-1.0 ${stats_element} \
    $source_element ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    $pipeline_1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    $pipeline_2 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    $pipeline_3 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    $FPS_DISP ${additonal_parameters}"

echo "Running License Plate Recognition"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}