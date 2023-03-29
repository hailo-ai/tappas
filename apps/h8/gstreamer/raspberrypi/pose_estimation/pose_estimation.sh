#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh

    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes/"
    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/raspberrypi/pose_estimation/resources"
    readonly DEFAULT_POSTPROCESS_SO="$POSTPROCESS_DIR/libcenterpose_post.so"
    readonly DEFAULT_NETWORK_NAME="centerpose"
    readonly DEFAULT_VIDEO_SOURCE="$TAPPAS_WORKSPACE/apps/h8/gstreamer/raspberrypi/detection/resources/detection.mp4"
    readonly DEFAULT_HEF_PATH="$RESOURCES_DIR/centerpose_regnetx_1.6gf_fpn.hef"

    postprocess_so=$DEFAULT_POSTPROCESS_SO
    network_name=$DEFAULT_NETWORK_NAME
    input_source=$DEFAULT_VIDEO_SOURCE
    hef_path=$DEFAULT_HEF_PATH
    network_name=$DEFAULT_NETWORK_NAME
    sync_pipeline=false

    print_gst_launch_only=false
    additional_parameters=""

    camera_framerate="35/1"
    camera_input_format="RGB"
    camera_input_width=640
    camera_input_height=640
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

function print_usage() {
    echo "Pose Estimation pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                  Show this help"
    echo "  -i INPUT --input INPUT  Set the video source - Could be path to video file or a video device path"
    echo "  --network NETWORK       Set network to use. choose from [centerpose, centerpose_416], default is centerpose"
    echo "  --show-fps              Printing fps"
    echo "  --print-gst-launch      Print the ready gst-launch command without running it"
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
            additional_parameters="-v | grep hailo_display"
        elif [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
        elif [ "$1" = "--input" ] || [ "$1" = "-i" ]; then
            input_source="$2"
            shift
        elif [ $1 == "--network" ]; then
            if [ $2 == "centerpose_416" ]; then
                network_name="centerpose_416"
                hef_path="$RESOURCES_DIR/centerpose_repvgg_a0.hef"
                camera_input_width=416
                camera_input_height=416
            elif [ $2 != "centerpose" ]; then
                echo "Received invalid network: $2. See expected arguments below:"
                print_usage
                exit 1
            fi
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
    validate_rpi_platform_camera_support
    source_element="v4l2src device=$input_source name=src_0 !  video/x-raw, format=$camera_input_format, width=$camera_input_width, height=$camera_input_height, framerate=$camera_framerate, pixel-aspect-ratio=1/1 ! \
                    queue max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
                    videoflip video-direction=horiz"
else
    source_element="filesrc location=$input_source name=src_0 ! qtdemux ! h264parse ! avdec_h264 ! \
                    queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
                    videoscale n-threads=3 ! video/x-raw, pixel-aspect-ratio=1/1 ! \
                    queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
                    videoconvert n-threads=2"
fi

PIPELINE="gst-launch-1.0 \
    $source_element ! \
    queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path ! \
    queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$postprocess_so qos=false function-name=$network_name ! \
    queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay qos=false ! \
    queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
    videoconvert n-threads=2 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display sync=$sync_pipeline text-overlay=false ${additional_parameters}"

echo ${PIPELINE}
if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi
eval ${PIPELINE}
