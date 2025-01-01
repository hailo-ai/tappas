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
    readonly DEFAULT_ENCODER_CONFIG_PATH="$RESOURCES_DIR/configs/encoder_config.json"

    # Default Video
    readonly DEFAULT_VIDEO_SOURCE="$RESOURCES_DIR/lpr_nv12.raw"

    # Vehicle Detection Macros
    readonly VEHICLE_DETECTION_HEF="$RESOURCES_DIR/yolov5m_vehicles_nv12.hef"
    readonly VEHICLE_DETECTION_POST_SO="$POSTPROCESS_DIR/libyolo_hailortpp_post.so"
    readonly VEHICLE_DETECTION_POST_FUNC="yolov5m_vehicles_nv12"

    # License Plate Detection Macros
    readonly LICENSE_PLATE_DETECTION_HEF="$RESOURCES_DIR/tiny_yolov4_license_plates_nv12.hef"
    readonly LICENSE_PLATE_DETECTION_POST_SO="$POSTPROCESS_DIR/libyolo_post.so"
    readonly LICENSE_PLATE_DETECTION_POST_FUNC="tiny_yolov4_license_plates"

    # License Plate OCR Macros
    readonly LICENSE_PLATE_OCR_HEF="$RESOURCES_DIR/lprnet_nv12.hef"
    readonly LICENSE_PLATE_OCR_POST_SO="$POSTPROCESS_DIR/libocr_post.so"
    readonly LICENSE_PLATE_OCR_POST_FUNC="lprnet_nv12"

    # Cropping Algorithm Macros
    readonly LICENSE_PLATE_CROP_SO="$CROPPING_ALGORITHMS_DIR/liblpr_croppers.so"
    readonly LICENSE_PLATE_DETECTION_CROP_FUNC="vehicles_without_ocr"
    readonly LICENSE_PLATE_OCR_CROP_FUNC="license_plate_quality_estimation"

    # Pipeline Utilities
    readonly LPR_OVERLAY="$RESOURCES_DIR/liblpr_overlay.so"
    readonly LPR_OCR_SINK="$RESOURCES_DIR/liblpr_ocrsink.so"

    readonly DEFAULT_UDP_PORT=5000
    readonly DEFAULT_UDP_HOST_IP="10.0.0.2"

    input_source=$DEFAULT_VIDEO_SOURCE
    udp_port=$DEFAULT_UDP_PORT
    udp_host_ip=$DEFAULT_UDP_HOST_IP

    encoder_config_path=$DEFAULT_ENCODER_CONFIG_PATH
    print_gst_launch_only=false
    additional_parameters=""
    stats_element=""
    debug_stats_export=""
    sync_pipeline=true
    device_id_prop=""
    tee_name="context_tee"
    internal_offset=false
    pipeline_1=""
    license_plate_json_config_path=$DEFAULT_LICENSE_PLATE_JSON_CONFIG_PATH
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
    echo "Hailo15 LPR pipeline usage:"
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
            additional_parameters="-v | grep -e hailo_display"
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
source_element="filesrc location=$input_source name=src_0 ! rawvideoparse format=nv12 width=1920 height=1080"
internal_offset=true

function create_lp_detection_pipeline() {
    pipeline_1="queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                hailocropper use-dsp=false so-path=$LICENSE_PLATE_CROP_SO function-name=$LICENSE_PLATE_DETECTION_CROP_FUNC internal-offset=$internal_offset drop-uncropped-buffers=true name=cropper1 \
                hailoaggregator name=agg1 \
                cropper1. ! \
                    queue leaky=no max-size-buffers=50 max-size-bytes=0 max-size-time=0 ! \
                agg1. \
                cropper1. ! \
                    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                    hailonet hef-path=$LICENSE_PLATE_DETECTION_HEF vdevice-group-id=1 scheduling-algorithm=1 ! \
                    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                    hailofilter so-path=$LICENSE_PLATE_DETECTION_POST_SO config-path=$license_plate_json_config_path function-name=$LICENSE_PLATE_DETECTION_POST_FUNC qos=false ! \
                    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                agg1. \
                agg1. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                hailocropper use-dsp=false so-path=$LICENSE_PLATE_CROP_SO function-name=$LICENSE_PLATE_OCR_CROP_FUNC internal-offset=$internal_offset drop-uncropped-buffers=true name=cropper2 \
                hailoaggregator name=agg2 \
                cropper2. ! \
                    queue leaky=no max-size-buffers=50 max-size-bytes=0 max-size-time=0 ! \
                agg2. \
                cropper2. ! \
                    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
	                hailonet hef-path=$LICENSE_PLATE_OCR_HEF vdevice-group-id=1 scheduling-algorithm=1 ! \
                    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                    hailofilter so-path=$LICENSE_PLATE_OCR_POST_SO function-name=$LICENSE_PLATE_OCR_POST_FUNC qos=false ! \
                    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                agg2. \
                agg2. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0"
}

create_lp_detection_pipeline $@

UDP_SINK="udpsink host=$udp_host_ip port=$udp_port"

PIPELINE="${debug_stats_export} gst-launch-1.0 ${stats_element} \
    $source_element ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet batch-size=4 hef-path=$VEHICLE_DETECTION_HEF vdevice-group-id=1 scheduling-algorithm=1 nms-iou-threshold=0.45 nms-score-threshold=0.3 nms-max-proposals-per-class=200 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$VEHICLE_DETECTION_POST_SO function-name=$VEHICLE_DETECTION_POST_FUNC qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailotracker name=hailo_tracker keep-past-metadata=true kalman-dist-thr=.5 iou-thr=.6 keep-tracked-frames=2 keep-lost-frames=2 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    tee name=$tee_name \
    $tee_name. ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay line-thickness=5 font-thickness=3 qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter use-gst-buffer=true so-path=$LPR_OVERLAY qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailoupload pool-size=16 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    video/x-raw, framerate=25/1 ! \
    hailoencodebin config-file-path=$encoder_config_path ! h264parse config-interval=-1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    tee name=udp_tee \
    udp_tee. ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    rtph264pay ! 'application/x-rtp, media=(string)video, encoding-name=(string)H264' ! \
    fpsdisplaysink fps-update-interval=2000 video-sink='$UDP_SINK' name=udp_sink sync=$sync_pipeline text-overlay=false \
    udp_tee. ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    fpsdisplaysink fps-update-interval=2000 video-sink=fakesink name=hailo_display sync=$sync_pipeline text-overlay=false \
    $tee_name. ! \
    $pipeline_1 ! \
    hailofilter use-gst-buffer=true so-path=$LPR_OCR_SINK qos=false ! \
    fakesink sync=false async=false  ${additional_parameters}"

echo "Running License Plate Recognition"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
