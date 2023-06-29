#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@

    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh

    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes/"
    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/detection/resources"
    readonly CROPPING_ALGORITHMS_DIR="$POSTPROCESS_DIR/cropping_algorithms"

    readonly DEFAULT_POSTPROCESS_SO="$POSTPROCESS_DIR/libyolo_hailortpp_post.so"
    readonly DEFAULT_NETWORK_NAME="yolov5"
    readonly DEFAULT_BATCH_SIZE="1"
    readonly DEFAULT_VIDEO_SOURCE="$RESOURCES_DIR/detection.mp4"
    readonly DEFAULT_HEF_PATH="$RESOURCES_DIR/yolov5m_wo_spp_60p.hef"

    # Cropping Algorithm Macros
    readonly WHOLE_BUFFER_CROP_SO="$CROPPING_ALGORITHMS_DIR/libwhole_buffer.so"

    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
    postprocess_so=$DEFAULT_POSTPROCESS_SO
    network_name=$DEFAULT_NETWORK_NAME
    input_source=$DEFAULT_VIDEO_SOURCE
    batch_size=$DEFAULT_BATCH_SIZE
    hef_path=$DEFAULT_HEF_PATH
    crop_so=$WHOLE_BUFFER_CROP_SO
    json_config_path="null"

    print_gst_launch_only=false
    additional_parameters=""
    stats_element=""
    debug_stats_export=""
    sync_pipeline=false
    device_id_prop=""
    internal_offset=false
    cropping_period=1
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
    echo "Detection pipeline usage:"
    echo ""
    echo "Options:"
    echo "  -h --help                  Show this help"
    echo "  --inference-period INT     How frequently to perform inference, default is every 1 buffer (must be >= 1)."
    echo "                             NOTE: It is recommended to adjust the hailotracker element properties as suits your inference period."
    echo "  --network NETWORK          Set network to use. choose from [yolov3, yolov4, yolov5, mobilenet_ssd, nanodet], default is yolov5"
    echo "  -i INPUT --input INPUT     Set the input source (default $input_source)"
    echo "  --show-fps                 Print fps"
    echo "  --print-gst-launch         Print the ready gst-launch command without running it"
    echo "  --print-device-stats       Print the power and temperature measured"
    exit 0
}

function parse_args() {
    while test $# -gt 0; do
        if [ $1 == "--network" ]; then
            if [ $2 == "yolov4" ]; then
                network_name="yolov4"
                hef_path="$RESOURCES_DIR/yolov4_leaky.hef"
                batch_size="4"
                json_config_path="$RESOURCES_DIR/configs/yolov4.json"
                postprocess_so="$POSTPROCESS_DIR/libyolo_post.so"
            elif [ $2 == "yolov3" ]; then
                network_name="yolov3"
                hef_path="$RESOURCES_DIR/yolov3.hef"
                batch_size="4"
                json_config_path="$RESOURCES_DIR/configs/yolov3.json"
                postprocess_so="$POSTPROCESS_DIR/libyolo_post.so"
            elif [ $2 == "mobilenet_ssd" ]; then
                network_name="mobilenet_ssd"
                hef_path="$RESOURCES_DIR/ssd_mobilenet_v1.hef"
                postprocess_so="$POSTPROCESS_DIR/libmobilenet_ssd_post.so"
                json_config_path="null"
            elif [ $2 == "nanodet" ]; then
                network_name="nanodet_repvgg"
                hef_path="$RESOURCES_DIR/nanodet_repvgg.hef"
                postprocess_so="$POSTPROCESS_DIR/libnanodet_post.so"
                json_config_path="null"
            elif [ $2 != "yolov5" ]; then
                echo "Received invalid network: $2. See expected arguments below:"
                print_usage
                exit 1
            fi
            shift
        elif [ "$1" == "--inference-period" ]; then
            if [ $2 -lt 1 ]; then
                echo "--inference-period received invalid number: $2. See expected arguments below:"
                print_usage
                exit 1
            else
                cropping_period="$2"
            fi
            shift
        elif [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
        elif [ "$1" = "--print-device-stats" ]; then
            hailo_bus_id=$(hailortcli scan | awk '{ print $NF }' | tail -n 1)
            device_id_prop="device_id=$hailo_bus_id"
            stats_element="hailodevicestats $device_id_prop"
            debug_stats_export="GST_DEBUG=hailodevicestats:5"
        elif [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            additional_parameters="-v | grep -e hailo_display -e hailodevicestats"
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

init_variables $@
parse_args $@

# If the video provided is from a camera
if [[ $input_source =~ "/dev/video" ]]; then
    source_element="v4l2src device=$input_source name=src_0 ! videoflip video-direction=horiz"
    internal_offset=false
else
    source_element="filesrc location=$input_source name=src_0 ! decodebin"
    internal_offset=true
fi

PIPELINE="${debug_stats_export} gst-launch-1.0 ${stats_element} \
    $source_element ! \
    videoscale ! videoconvert ! video/x-raw,pixel-aspect-ratio=1/1,format=RGB ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailocropper so-path=$crop_so function-name=create_crops internal-offset=$internal_offset cropping-period=$cropping_period name=cropper1 \
    hailoaggregator name=agg1 \
    cropper1. ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    agg1. \
    cropper1. ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailonet hef-path=$hef_path $device_id_prop batch-size=$batch_size ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailofilter function-name=$network_name so-path=$postprocess_so config-path=$json_config_path qos=false ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    agg1. \
    agg1. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailotracker name=hailo_tracker keep-new-frames=5 keep-tracked-frames=5 keep-lost-frames=5 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! \
    videoconvert ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=$sync_pipeline text-overlay=false ${additional_parameters}"

echo "Running $network_name"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
