#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh

    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes"
    readonly RESOURCES_PATH="$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/multinetworks_parallel/resources"
    readonly DEFAULT_VIDEO_SOURCE="$RESOURCES_PATH/detection.mp4"
    readonly DEFAULT_HEF_PATH="$RESOURCES_PATH/joined_yolov5m_wo_spp_60p_no_alls_centerpose_repvgg_a0_no_alls_center_nms_joint_nms.hef"
    readonly DEFAULT_JSON_CONFIG_PATH="$RESOURCES_PATH/configs/yolov5.json"

    video_source=$DEFAULT_VIDEO_SOURCE
    hef_path=$DEFAULT_HEF_PATH
    pose_estimation_post_so="$POSTPROCESS_DIR/libcenterpose_post.so"
    detection_post_so="$POSTPROCESS_DIR/libyolo_post.so"
    batch_size=4
    vdevice_key=1
    json_config_path=$DEFAULT_JSON_CONFIG_PATH

    pose_estimation_net_name="joined_yolov5m_wo_spp_60p_no_alls_centerpose_repvgg_a0_no_alls_center_nms_joint_nms/center_nms_centerpose_repvgg_a0_no_alls_joint_nms"
    detection_net_name="joined_yolov5m_wo_spp_60p_no_alls_centerpose_repvgg_a0_no_alls_center_nms_joint_nms/yolov5m_wo_spp_60p_no_alls"

    print_gst_launch_only=false
    additional_parameters=""
    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
}

function print_usage() {
    echo "Detection and Depth estimation pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                  Show this help"
    echo "  -i INPUT --input INPUT  Set the video source (default $video_source)"
    echo "  --show-fps              Print fps"
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
        elif [ "$1" = "--input" ] || [ "$1" == "-i" ]; then
            video_source="$2"
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
if [[ $video_source =~ "/dev/video" ]]; then
    source_element="v4l2src device=$video_source name=src_0 ! videoflip video-direction=horiz"
    sync_pipeline=false
else
    source_element="filesrc location=$video_source name=src_0 ! decodebin"
    sync_pipeline=true
fi

PIPELINE="gst-launch-1.0 \
    $source_element ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert !
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    tee name=t ! \
    queue ! videoscale ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path is-active=true scheduling-algorithm=0 batch-size=$batch_size vdevice-key=$vdevice_key net-name=$pose_estimation_net_name ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$pose_estimation_post_so function-name=centerpose_merged qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay qos=false ! videoconvert ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display_pose sync=false text-overlay=false \
    t. ! \
    videoscale ! queue ! \
    hailonet hef-path=$hef_path is-active=true scheduling-algorithm=0 batch-size=$batch_size vdevice-key=$vdevice_key net-name=$detection_net_name ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$detection_post_so config-path=$json_config_path function-name=yolov5 qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay qos=false ! videoconvert ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=false text-overlay=false ${additional_parameters} "

echo "Running"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
