#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh

    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes"
    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/tiling/resources"

    readonly DEFAULT_DETECTION_POSTPROCESS_SO="$POSTPROCESS_DIR/libmobilenet_ssd_post.so"
    readonly DEFAULT_DETECTION_VIDEO_SOURCE="$RESOURCES_DIR/river_tiber.mp4"
    readonly DEFAULT_HEF_PATH="$RESOURCES_DIR/ssd_mobilenet_v1_visdrone.hef"

    hef_path=$DEFAULT_HEF_PATH
    detection_postprocess_so=$DEFAULT_DETECTION_POSTPROCESS_SO
    input_source=$DEFAULT_DETECTION_VIDEO_SOURCE

    postprocess_func_name="mobilenet_ssd_visdrone"
    internal_offset=true
    sync_pipeline=false
    print_gst_launch_only=false
    additional_parameters=""

    # Tiling default parameters
    tiles_along_x_axis=4
    tiles_along_y_axis=3
    overlap_x_axis=0.08
    overlap_y_axis=0.08
    iou_threshold=0.3

    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
}

function print_usage() {
    echo "Tiling pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                         Show this help"
    echo "  -i INPUT --input INPUT         Set the input source (default $input_source)"
    echo "  --tiles-x-axis TILES-X         Set number of tiles along x axis (columns) (default is $tiles_along_x_axis)"
    echo "  --tiles-y-axis TILES-Y         Set number of tiles along y axis (rows) (default is $tiles_along_y_axis)"
    echo "  --overlap-x-axis OVERLAP-X     Set overlap in percentage between tiles along x axis (columns) (default is $overlap_x_axis)"
    echo "  --overlap-y-axis OVERLAP-Y     Set overlap in percentage between tiles along y axis (rows) (default is $overlap_y_axis)"
    echo "  --iou-threshold IOU-THRESHOLD  Set iou threshold for NMS (default is $iou_threshold)"
    echo "  --show-fps                     Print fps"
    echo "  --print-gst-launch             Print the ready gst-launch command without running it"
    echo "  --sync-pipeline                Set pipeline to sync to video file timing. works only on video file source (default is false)"
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
        if [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
        elif [ "$1" = "--tiles-x-axis" ]; then
            tiles_along_x_axis="$2"
            shift
        elif [ "$1" = "--tiles-y-axis" ]; then
            tiles_along_y_axis="$2"
            shift
        elif [ "$1" = "--overlap-x-axis" ]; then
            overlap_x_axis="$2"
            shift
        elif [ "$1" = "--overlap-y-axis" ]; then
            overlap_y_axis="$2"
            shift
        elif [ "$1" = "--iou-threshold" ]; then
            iou_threshold="$2"
            shift
        elif [ "$1" = "--sync-pipeline" ]; then
            echo "Setting videosink sync parameter to true"
            sync_pipeline=true
        elif [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            additional_parameters="-v | grep hailo_display"
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
    source_element="v4l2src device=$input_source name=src_0  ! videoflip video-direction=horiz ! videoconvert qos=false"
    internal_offset=false
    sync_pipeline=false
else
    source_element="filesrc location=$input_source name=src_0 ! decodebin ! videoconvert qos=false"
fi

# Detection section
DETECTION_PIPELINE="\
        queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
        hailonet hef-path=$hef_path ! \
        queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
        hailofilter function-name=$postprocess_func_name so-path=$detection_postprocess_so qos=false ! \
        queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0"

TILE_CROPPER_ELEMENT="hailotilecropper internal-offset=$internal_offset name=cropper \
            tiles-along-x-axis=$tiles_along_x_axis tiles-along-y-axis=$tiles_along_y_axis overlap-x-axis=$overlap_x_axis overlap-y-axis=$overlap_y_axis"

PIPELINE="gst-launch-1.0 $source_element ! \
    video/x-raw,pixel-aspect-ratio=1/1,format=RGB ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    $TILE_CROPPER_ELEMENT hailotileaggregator flatten-detections=true iou-threshold=$iou_threshold name=agg \
    cropper. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! agg. \
    cropper. ! $DETECTION_PIPELINE ! agg. \
    agg. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay qos=false ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! videoconvert qos=false ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=$sync_pipeline text-overlay=false $additional_parameters"

echo "Running $network_name"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
