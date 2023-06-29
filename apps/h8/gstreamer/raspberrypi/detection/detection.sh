#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh

    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes"
    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/raspberrypi/detection/resources"

    readonly DEFAULT_POSTPROCESS_SO="$POSTPROCESS_DIR/libyolo_hailortpp_post.so"
    readonly DEFAULT_NETWORK_NAME="yolov5"
    readonly DEFAULT_BATCH_SIZE="1"
    readonly DEFAULT_VIDEO_SOURCE="$RESOURCES_DIR/detection.mp4"
    readonly DEFAULT_HEF_PATH="$RESOURCES_DIR/yolov5m_wo_spp_60p.hef"

    postprocess_so=$DEFAULT_POSTPROCESS_SO
    network_name=$DEFAULT_NETWORK_NAME
    input_source=$DEFAULT_VIDEO_SOURCE
    batch_size=$DEFAULT_BATCH_SIZE
    hef_path=$DEFAULT_HEF_PATH

    print_gst_launch_only=false
    additional_parameters=""
    stats_element=""
    debug_stats_export=""
    sync_pipeline=false
    device_id_prop=""

    camera_framerate="40/1"
    camera_input_width=640
    camera_input_height=640
    camera_input_format="RGB"
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
    echo "  --network NETWORK          Set network to use. choose from [yolov5, mobilenet_ssd], default is yolov5"
    echo "  -i INPUT --input INPUT     Set the input source (default $input_source)"
    echo "  --show-fps                 Print fps"
    echo "  --print-gst-launch         Print the ready gst-launch command without running it"
    echo "  --print-device-stats       Print the power and temperature measured"
    exit 0
}

function parse_args() {
    while test $# -gt 0; do
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage
            exit 0
        elif [ $1 == "--network" ]; then
            if [ $2 == "mobilenet_ssd" ]; then
                network_name="mobilenet_ssd"
                hef_path="$RESOURCES_DIR/ssd_mobilenet_v1.hef"
                postprocess_so="$POSTPROCESS_DIR/libmobilenet_ssd_post.so"
            elif [ $2 != "yolov5" ]; then
                echo "Received invalid network: $2. See expected arguments below:"
                print_usage
                exit 1
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

function validate_rpi_platform_camera_support() {
    os_id=$(cat /etc/os-release | grep ^ID=)
    if [ "$os_id" != "ID=debian" ]; then
        echo "Camera device supported only in Debian release of Raspberry PI"
        exit 1
    fi

    identify_v4l=$(gst-launch-1.0 v4l2src device=$input_source num-buffers=1 ! fakesink 2>&1 | grep 'Cannot identify device' | wc -l)
    if [ "$identify_v4l" -eq 1 ]; then
        echo "Cannot identify v4l device '$input_source'"
        exit 1
    fi

    num_of_cam_devices_sup=$(gst-launch-1.0 --gst-debug=v4l2src:5 v4l2src device=$input_source num-buffers=1 ! fakesink 2>&1 | sed -une '/caps of src/ s/[:;] /\n/gp' | grep $camera_input_format | wc -l)
    if [ "$num_of_cam_devices_sup" -eq 0 ]; then
        echo "v4l device '$input_source' is not supporting requested input format '$camera_input_format'"
        exit 1
    fi
}

init_variables $@
parse_args $@

# If the video provided is from a camera
if [[ $input_source =~ "/dev/video" ]]; then
    validate_rpi_platform_camera_support
    source_element="v4l2src device=$input_source name=src_0 !  video/x-raw, format=$camera_input_format, width=$camera_input_width, height=$camera_input_height, framerate=$camera_framerate, pixel-aspect-ratio=1/1 ! \
                    queue max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
                    videoflip video-direction=horiz ! \
                    queue max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
                    videoscale n-threads=2"
else
    source_element="filesrc location=$input_source name=src_0 ! qtdemux ! h264parse ! avdec_h264 max_threads=2 ! \
                    queue max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
                    videoscale n-threads=2 ! \
                    queue max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
                    videoconvert n-threads=3"
fi

PIPELINE="${debug_stats_export} gst-launch-1.0 ${stats_element} \
    $source_element ! \
    queue max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path $device_id_prop batch-size=$batch_size ! \
    queue max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
    hailofilter function-name=$network_name so-path=$postprocess_so qos=false ! \
    queue max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! \
    queue max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
    videoconvert n-threads=3 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display sync=$sync_pipeline text-overlay=false ${additional_parameters}"

echo "Running $network_name"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
