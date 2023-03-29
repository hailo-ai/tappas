#!/bin/bash
set -e

function init_variables() {
    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../../../../scripts/misc/checks_before_run.sh

    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/face_recognition/resources"
    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/post_processes/"
    readonly APPS_LIBS_DIR="$TAPPAS_WORKSPACE/apps/h8/gstreamer/libs/apps/vms/"
    readonly CROPPER_SO="$POSTPROCESS_DIR/cropping_algorithms/libvms_croppers.so"
    
    # Face Alignment
    readonly FACE_ALIGN_SO="$APPS_LIBS_DIR/libvms_face_align.so"
    
    # Face Recognition
    readonly RECOGNITION_POST_SO="$POSTPROCESS_DIR/libface_recognition_post.so"
    readonly RECOGNITION_HEF_PATH="$RESOURCES_DIR/arcface_mobilefacenet_v1.hef"

    # Face Detection and Landmarking
    readonly DEFAULT_HEF_PATH="$RESOURCES_DIR/scrfd_10g.hef"
    readonly POSTPROCESS_SO="$POSTPROCESS_DIR/libscrfd_post.so"
    readonly FACE_JSON_CONFIG_PATH="$RESOURCES_DIR/configs/scrfd.json"
    readonly FUNCTION_NAME="scrfd_10g"

    hef_path=$DEFAULT_HEF_PATH
    input_source="$RESOURCES_DIR/face_recognition.mp4"
    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
    additional_parameters=""
    print_gst_launch_only=false
    vdevice_key=1
    function_name=$FUNCTION_NAME
    local_gallery_file="$RESOURCES_DIR/gallery/face_recognition_local_gallery.json"
}

function print_usage() {
    echo "Face recognition - pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                          Show this help"
    echo "  --show-fps                      Printing fps"
    echo "  -i INPUT --input INPUT          Set the input source (default $input_source)"
    echo "  --network NETWORK               Set network to use. choose from [scrfd_10g, scrfd_2.5g], default is scrfd_10g"
    echo "  --print-gst-launch              Print the ready gst-launch command without running it"
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
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage
            exit 0
        elif [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
        elif [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            additional_parameters="-v 2>&1 | grep hailo_display"
        elif [ "$1" = "--input" ] || [ "$1" == "-i" ]; then
            input_source="$2"
            shift
        elif [ $1 == "--network" ]; then
            if [ $2 == "scrfd_2.5g" ]; then
                hef_path="$RESOURCES_DIR/scrfd_2.5g.hef"
                function_name="scrfd_2_5g"
            elif [ $2 != "scrfd_10g" ]; then
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

function main() {
    init_variables $@
    parse_args $@

    # If the video provided is from a camera
    if [[ $input_source =~ "/dev/video" ]]; then
        source_element="v4l2src device=$input_source name=src_0 ! video/x-raw,format=YUY2,width=1920,height=1080,framerate=30/1 ! \
                        queue  max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                        videoflip video-direction=horiz"
    else
        source_element="filesrc location=$input_source name=src_0 ! decodebin"
    fi

    RECOGNITION_PIPELINE="hailocropper so-path=$CROPPER_SO function-name=face_recognition internal-offset=true name=cropper2 \
        hailoaggregator name=agg2 \
        cropper2. ! queue name=bypess2_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! agg2. \
        cropper2. ! queue name=pre_face_align_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailofilter so-path=$FACE_ALIGN_SO name=face_align_hailofilter use-gst-buffer=true qos=false ! \
        queue name=detector_pos_face_align_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailonet hef-path=$RECOGNITION_HEF_PATH scheduling-algorithm=1 vdevice-key=$vdevice_key ! \
        queue name=recognition_post_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailofilter so-path=$RECOGNITION_POST_SO name=face_recognition_hailofilter qos=false ! \
        queue name=recognition_pre_agg_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        agg2. agg2. "

    FACE_DETECTION_PIPELINE="hailonet hef-path=$hef_path scheduling-algorithm=1 vdevice-key=$vdevice_key ! \
        queue name=detector_post_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailofilter so-path=$POSTPROCESS_SO name=face_detection_hailofilter qos=false config-path=$FACE_JSON_CONFIG_PATH function_name=$function_name"

    FACE_TRACKER="hailotracker name=hailo_face_tracker class-id=-1 kalman-dist-thr=0.7 iou-thr=0.8 init-iou-thr=0.9 \
                    keep-new-frames=2 keep-tracked-frames=6 keep-lost-frames=8 keep-past-metadata=true qos=false"

    DETECTOR_PIPELINE="tee name=t hailomuxer name=hmux \
        t. ! \
            queue name=detector_bypass_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hmux. \
        t. ! \
            videoscale name=face_videoscale method=0 n-threads=2 add-borders=false qos=false ! \
            video/x-raw, pixel-aspect-ratio=1/1 ! \
            queue name=pre_face_detector_infer_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            $FACE_DETECTION_PIPELINE ! \
            queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hmux. \
        hmux. "

    pipeline="gst-launch-1.0 \
        $source_element ! \
        queue name=hailo_pre_convert_0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        videoconvert n-threads=2 qos=false ! \
        queue name=pre_detector_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        $DETECTOR_PIPELINE ! \
        queue name=pre_tracker_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        $FACE_TRACKER ! \
        queue name=hailo_post_tracker_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        $RECOGNITION_PIPELINE ! \
        queue name=hailo_pre_gallery_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailogallery gallery-file-path=$local_gallery_file \
        load-local-gallery=true similarity-thr=.4 gallery-queue-size=20 class-id=-1 ! \
        queue name=hailo_pre_draw2 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailooverlay name=hailo_overlay qos=false show-confidence=false local-gallery=true line-thickness=5 font-thickness=2 landmark-point-radius=8 ! \
        queue name=hailo_post_draw leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        videoconvert n-threads=4 qos=false name=display_videoconvert qos=false ! \
        queue name=hailo_display_q_0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=false text-overlay=false \
        ${additional_parameters}"

    echo ${pipeline}
    if [ "$print_gst_launch_only" = true ]; then
        exit 0
    fi

    echo "Running Pipeline..."
    eval "${pipeline}"

}

main $@
