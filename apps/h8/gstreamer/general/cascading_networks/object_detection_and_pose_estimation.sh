#!/bin/bash
set -e

function init_variables() {
    readonly DEFAULT_MAX_CAMERA_WIDTH="1920"
    readonly DEFAULT_MAX_CAMERA_HEGIHT="1080"
    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/cascading_networks/resources"
    readonly DEFAULT_DETECTION_VIDEO_SOURCE="$RESOURCES_DIR/instance_segmentation.mp4"

    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh

    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes"
    readonly CROPING_ALGORITHMS_DIR="$POSTPROCESS_DIR/cropping_algorithms"

    readonly DEFAULT_DETECTION_HEF_PATH="$RESOURCES_DIR/yolov5m_wo_spp_60p.hef"
    readonly DEFAULT_POST_ESTIMATION_HEF_PATH="$RESOURCES_DIR/mspn_regnetx_800mf.hef"

    readonly DEFAULT_DETECTION_POSTPROCESS_SO="$POSTPROCESS_DIR/libyolo_hailortpp_post.so"
    readonly DEFAULT_LANDMARKS_POSTPROCESS_SO="$POSTPROCESS_DIR/libmspn_post.so"

    readonly DEFAULT_CROP_SO="$CROPING_ALGORITHMS_DIR/libmspn.so"
    readonly DEFAULT_JSON_CONFIG_PATH="$RESOURCES_DIR/configs/mspn.json"

    readonly DEFAULT_VDEVICE_KEY="1"
    readonly DEFAULT_NETWORK_NAME="yolov5"

    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
    video_sink="fpsdisplaysink video-sink=$video_sink_element text-overlay=false"
    detection_hef_path=$DEFAULT_DETECTION_HEF_PATH
    pose_estimation_hef_path=$DEFAULT_POST_ESTIMATION_HEF_PATH
    detection_postprocess_so=$DEFAULT_DETECTION_POSTPROCESS_SO
    landmarks_postprocess_so=$DEFAULT_LANDMARKS_POSTPROCESS_SO
    input_source=$DEFAULT_DETECTION_VIDEO_SOURCE
    crop_so=$DEFAULT_CROP_SO
    json_config_path=$DEFAULT_JSON_CONFIG_PATH
    network_name=$DEFAULT_NETWORK_NAME

    internal_offset=false
    print_gst_launch_only=false
    additional_parameters=""
    max_camera_width=$DEFAULT_MAX_CAMERA_WIDTH
    max_camera_height=$DEFAULT_MAX_CAMERA_HEGIHT
}

function print_usage() {
    echo "Sanity hailo pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                  Show this help"
    echo "  -i INPUT --input INPUT                  Set the input source (default $DEFAULT_DETECTION_VIDEO_SOURCE)"
    echo "  --show-fps                              Print fps"
    echo "  --max-camera-resolution WIDTHxHEIGHT    The maximun input resolution from camera as an input (default ${DEFAULT_MAX_CAMERA_WIDTH}x${DEFAULT_MAX_CAMERA_HEGIHT})"
    echo "  --print-gst-launch      Print the ready gst-launch command without running it"
    echo "  --tcp-address              Used for TAPPAS GUI, switchs the sink to TCP client"
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
        elif [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            additional_parameters="-v | grep hailo_display"
        elif [ "$1" = "--input" ] || [ "$1" == "-i" ]; then
            input_source="$2"
            shift
        elif [ "$1" = "--max-camera-resolution" ]; then
            shift
            max_camera_width=$(echo $1 | awk -F'x' '{print $1}')
            max_camera_height=$(echo $1 | awk -F'x' '{print $2}')
            echo "The maximum resolution to use for camera as an input is $1"
        elif [ "$1" = "--tcp-address" ]; then
            tcp_host=$(echo $2 | awk -F':' '{print $1}')
            tcp_port=$(echo $2 | awk -F':' '{print $2}')
            video_sink="queue name=queue_before_sink leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                        videoscale ! video/x-raw,width=836,height=546,format=RGB ! \
                        queue max-size-bytes=0 max-size-time=0 ! tcpclientsink host=$tcp_host port=$tcp_port" 

            shift
        else
            echo "Received invalid argument: $1. See expected arguments below:"
            print_usage
            exit 1
        fi

        shift
    done
}

function find_max_resolution() {
    resolutions=($(ffmpeg -f video4linux2 -list_formats all -i $input_source 2>&1 | grep JPEG | grep -oE '\b[0-9]+x[0-9]+\b' | tr '\n' ' '))
    max_resolution="0x0"

    for res in "${resolutions[@]}"
    do
        if [[ "$res" =~ ^([0-9]+)x([0-9]+)$ ]]; then
            if (( ${BASH_REMATCH[1]} <= $max_camera_width && ${BASH_REMATCH[2]} <= $max_camera_height )); then
              if (( ${BASH_REMATCH[1]} >= ${max_resolution%x*} || ${BASH_REMATCH[2]} >= ${max_resolution#*x} )); then
                max_resolution="$res"
              fi
            fi
        fi
    done

    width=$(echo $max_resolution | awk -F'x' '{print $1}')
    height=$(echo $max_resolution | awk -F'x' '{print $2}')
}

init_variables $@
parse_args $@

# If the video provided is from a camera
if [[ $input_source =~ "/dev/video" ]]; then
    find_max_resolution

    source_element="v4l2src device=$input_source name=src_0 ! image/jpeg, width=$width, height=$height !  \
                    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
                    decodebin ! \
                    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
                    videoflip video-direction=horiz ! videoconvert qos=false"
    internal_offset=false
else
    source_element="filesrc location=$input_source name=src_0 ! decodebin ! videoconvert qos=false"
    internal_offset=true
fi

OBJECT_DETECTION_PIPELINE="videoscale qos=false ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$detection_hef_path scheduling-algorithm=1 scheduler-threshold=5 \
    scheduler-timeout-ms=100 vdevice-key=$DEFAULT_VDEVICE_KEY ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailofilter name=detection so-path=$detection_postprocess_so qos=false function-name=$network_name ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0"

LANDMARKS_PIPELINE="videoscale qos=false ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$pose_estimation_hef_path scheduling-algorithm=1 scheduler-threshold=5 \
    scheduler-timeout-ms=100 vdevice-key=$DEFAULT_VDEVICE_KEY ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailofilter name=pose-estimation so-path=$landmarks_postprocess_so config-path=$json_config_path \
    qos=false ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0"

PIPELINE="gst-launch-1.0 $source_element ! tee name=t hailomuxer name=hmux \
    t. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! hmux. \
    t. ! $OBJECT_DETECTION_PIPELINE ! hmux. \
    hmux. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailocropper so-path=$crop_so function-name=create_crops_only_person internal-offset=$internal_offset name=cropper \
    hailoaggregator name=agg \
    cropper. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! agg. \
    cropper. ! $LANDMARKS_PIPELINE ! agg. \
    agg. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay qos=false ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! videoconvert ! \
    $video_sink name=hailo_display sync=false ${additional_parameters}"

echo "Running $network_name"
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}
