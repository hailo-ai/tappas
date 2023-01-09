#!/bin/bash
set -e

function check_ubuntu22_version() {
    ubuntu_version=$(lsb_release -r | awk '{print $2}' | awk -F'.' '{print $1}')
    if [ $ubuntu_version != 22 ]; then
        echo "ERROR: The Video Managment Systems (VMS) pipeline is currntely supported for Ubuntu 22.04 OS (Your system is Ubuntu $ubuntu_version)"
        exit 1
    fi
}

function init_variables() {
    person_attribute_streams="sink_0,sink_3,sink_6,sink_7"
    face_attribute_streams="sink_1,sink_4"
    face_recognition_streams="sink_2,sink_5"

    print_help_if_needed $@
    script_dir=$(dirname $(realpath "$0"))
    check_ubuntu22_version
    source $script_dir/../../../../scripts/misc/checks_before_run.sh --check-vaapi

    # Standard Dirs
    readonly RESOURCES_DIR="$TAPPAS_WORKSPACE/apps/gstreamer/x86_hw_accelerated/video_management_system/resources"
    readonly POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/gstreamer/libs/post_processes/"
    readonly VMS_POSTPROCESS_DIR="$TAPPAS_WORKSPACE/apps/gstreamer/libs/apps/vms/"
    readonly CROPING_ALGORITHMS_DIR="$POSTPROCESS_DIR/cropping_algorithms"
    readonly VMS_CROP_SO="$CROPING_ALGORITHMS_DIR/libvms_croppers.so"
    readonly WHOLE_BUFFER_CROP_SO="$CROPING_ALGORITHMS_DIR/libwhole_buffer.so"
    readonly DEFAULT_VDEVICE_KEY="1"

    # Person Detection
    readonly PERSON_DETECTION_HEF_PATH="$RESOURCES_DIR/yolov5s_personface_rgba.hef"
    readonly PERSON_DETECTION_POSTPROCESS_SO="$POSTPROCESS_DIR/libyolo_post.so"
    readonly PERSON_JSON_CONFIG_PATH="$RESOURCES_DIR/configs/yolov5_personface.json"
    readonly PERSON_DETECTION_FUNCTION_NAME="yolov5_no_faces"

    # Person Attributes
    readonly PERSON_ATTR_HEF_PATH="$RESOURCES_DIR/person_attr_resnet_v1_18_rgbx.hef"
    readonly PERSON_ATTR_POSTPROCESS_SO="$VMS_POSTPROCESS_DIR/libperson_attributes_post.so"
    readonly PERSON_ATTR_FUNCTION_NAME="person_attributes_rgba"
    readonly PERSON_ATTR_CROP_FUNC="person_attributes"

    # Face Detection and Landmarking
    readonly FACE_DETECTION_HEF_PATH="$RESOURCES_DIR/scrfd_2.5g.hef"
    readonly FACE_DETECTION_POSTPROCESS_SO="$POSTPROCESS_DIR/libface_detection_post.so"
    readonly FACE_JSON_CONFIG_PATH="$RESOURCES_DIR/configs/scrfd.json"
    readonly FACE_DETECTION_FUNCTION_NAME="scrfd"

    # Face Attributes
    readonly FACE_ATTR_HEF_PATH="$RESOURCES_DIR/face_attr_resnet_v1_18_rgbx.hef"
    readonly FACE_ATTR_POSTPROCESS_SO="$VMS_POSTPROCESS_DIR/libface_attributes_post.so"
    readonly FACE_ATTR_FUNCTION_NAME="face_attributes_rgba"
    readonly FACE_ATTR_CROP_FUNC="face_attributes"

    # Face Recognition
    readonly FACE_ALIGN_SO="$VMS_POSTPROCESS_DIR/libvms_face_align.so"
    readonly FACE_RECOGNITION_HEF_PATH="$RESOURCES_DIR/arcface_mobilefacenet_rgbx.hef"
    readonly FACE_RECOGNITION_POST_SO="$VMS_POSTPROCESS_DIR/libface_recognition_post.so"
    readonly FACE_RECOGNITION_FUNCTION_NAME="arcface_rgba"
    readonly GALLERY_CONFIG="$RESOURCES_DIR/gallery/face_recognition_local_gallery.json"

    num_of_src=8
    additonal_parameters=""
    sources=""

    decode_element="qtdemux ! \
                    vaapidecodebin ! video/x-raw, format=RGBA, width=1920, height=1080"
    create_compositor_table

    print_gst_launch_only=false
    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")

    hailo_bus_id=$(hailortcli scan | awk '{ print $NF }' | tail -n 1)
    device_id_prop="device_id=$hailo_bus_id"
    stats_element="hailodevicestats $device_id_prop"
}

function create_compositor_table(){
    # create a compositor table of 4 rows and 4 columns of frame size 640X640
    comp_row0="sink_0::xpos=0 sink_0::ypos=0 sink_1::xpos=640 sink_1::ypos=0 sink_2::xpos=1280 sink_2::ypos=0 sink_3::xpos=1920 sink_3::ypos=0"
    comp_row1="sink_4::xpos=0 sink_4::ypos=640 sink_5::xpos=640 sink_5::ypos=640 sink_6::xpos=1280 sink_6::ypos=640 sink_7::xpos=1920 sink_7::ypos=640"
    comp_row2="sink_8::xpos=0 sink_8::ypos=1280 sink_9::xpos=640 sink_9::ypos=1280 sink_10::xpos=1280 sink_10::ypos=1280 sink_11::xpos=1920 sink_11::ypos=1280"
    comp_row3="sink_12::xpos=0 sink_12::ypos=1920 sink_13::xpos=640 sink_13::ypos=1920 sink_14::xpos=1280 sink_14::ypos=1920 sink_15::xpos=1920 sink_15::ypos=1920"
    compositor_locations="$comp_row0 $comp_row1 $comp_row2 $comp_row3"
}

function print_usage() {
    echo "Video Managment Systems (VMS) pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                          Show this help"
    echo "  --show-fps                      Printing fps"
    echo "  --face-attr-streams             List of streames to perform face attributes on (default is '$face_attribute_streams')"
    echo "  --person-attr-streams           List of streames to perform person attributes on (default is '$person_attribute_streams')"
    echo "  --face-recognition-streams      List of streames to perform face recognition on (default is '$face_recognition_streams')"
    echo "  --num-of-sources NUM            Setting number of sources to given input (default value is 8, value between 2-8)"
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

function validate_args() {
    start_index=0
    for ((n = $start_index; n < $num_of_src; n++)); do
        # Count the number of times this stream appears in the strem configuration lists
        sink_count=$(echo "$person_attribute_streams $face_attribute_streams $face_recognition_streams" | grep -o "sink_$n" | wc -l)
        if [ "$sink_count" -gt 1 ]; then
            echo "Parameter Error: sink_$n appears more than once in the streams"
            exit 1
        fi
    done
}

function parse_args() {
    while test $# -gt 0; do
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage
            exit 0
        elif [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
        elif [ "$1" = "--num-of-sources" ]; then
            if [ "$2" -lt 2 ] || [ "$2" -gt 8 ]; then
                echo "Invalid argument received: num-of-sources must be between 2-8"
                exit 1
            fi
            shift
            echo "Setting number of sources to $1"
            num_of_src=$1
        elif [ "$1" = "--person-attr-streams" ]; then
            shift
            echo "Setting person attributes streams to $1"
            person_attribute_streams=$1
        elif [ "$1" = "--face-attr-streams" ]; then
            shift
            echo "Setting face attributes streams to $1"
            face_attribute_streams=$1
        elif [ "$1" = "--face-recognition-streams" ]; then
            shift
            echo "Setting face recognition streams to $1"
            face_recognition_streams=$1
        elif [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            additonal_parameters="-v | grep hailo_display:"
        else
            echo "Received invalid argument: $1. See expected arguments below:"
            print_usage
            exit 1
        fi
        shift
    done
}

function create_sources() {
    start_index=0
    identity=""
    for ((n = $start_index; n < $num_of_src; n++)); do
        hailooverlay_element="hailooverlay qos=false show-confidence=false font-thickness=2"

        if [[ $face_recognition_streams == *"sink_$n"* ]]; then
            # Add face recognition local gallery support to the overlay
            hailooverlay_element="$hailooverlay_element local-gallery=true line-thickness=5 landmark-point-radius=6"
        else
            # Add face blur to the overlay
            hailooverlay_element="$hailooverlay_element face-blur=true"
        fi

        sources+="filesrc location=$RESOURCES_DIR/vms_video$n.mp4 name=source_$n ! \
                $decode_element ! \
                queue name=pre_roundrobin_q_$n leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                roundrobin.sink_$n disp_router.src_$n ! \
                queue name=comp_overlay_$n leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                $hailooverlay_element ! \
                queue name=pre_comp_videoconvert_queue_$n leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                videoconvert name=pre_comp_videoconvert_$n qos=false ! \
                queue name=comp_q_$n leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                comp.sink_$n "

        streamrouter_disp_element+=" src_$n::input-streams=\"<sink_$n>\""
    done
}

function main() {
    init_variables $@
    parse_args $@
    internal_offset=true

    streamrouter_disp_element="hailostreamrouter name=disp_router"
    streamrouter_input_streams="src_0::input-streams=\"<$person_attribute_streams>\" \
                                src_1::input-streams=\"<$face_attribute_streams>\" \
                                src_2::input-streams=\"<$face_recognition_streams>\""

    validate_args
    create_sources

    PERSON_TRACKER="\
        hailotracker name=hailo_person_tracker class-id=1 kalman-dist-thr=0.7 iou-thr=0.8 init-iou-thr=0.9 \
        keep-new-frames=2 keep-tracked-frames=2 keep-lost-frames=2 keep-past-metadata=true qos=false"

    FACE_TRACKER="\
        hailotracker name=hailo_face_tracker kalman-dist-thr=0.7 iou-thr=0.8 init-iou-thr=0.9 \
        keep-new-frames=2 keep-tracked-frames=8 keep-lost-frames=8 keep-past-metadata=true qos=false"

    FACE_ATTR_INFER_POST="\
        queue name=face_attr_pre_infer_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailonet hef-path=$FACE_ATTR_HEF_PATH scheduling-algorithm=1 vdevice-key=$DEFAULT_VDEVICE_KEY ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailofilter use-gst-buffer=true function-name=$FACE_ATTR_FUNCTION_NAME so-path=$FACE_ATTR_POSTPROCESS_SO qos=false ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0"

    PERSON_ATTR_INFER_POST="\
        queue name=person_attr_pre_infer_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailonet hef-path=$PERSON_ATTR_HEF_PATH scheduling-algorithm=1 vdevice-key=$DEFAULT_VDEVICE_KEY ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailofilter use-gst-buffer=true function-name=$PERSON_ATTR_FUNCTION_NAME so-path=$PERSON_ATTR_POSTPROCESS_SO qos=false ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0"

    FACE_RECOGNITION_INFER_POST="\
        queue name=pre_face_align_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailofilter so-path=$FACE_ALIGN_SO use-gst-buffer=true qos=false ! \
        queue name=pre_recognition_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailonet hef-path=$FACE_RECOGNITION_HEF_PATH scheduling-algorithm=1 vdevice-key=$DEFAULT_VDEVICE_KEY ! \
        queue name=recognition_post_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailofilter use-gst-buffer=true function-name=$FACE_RECOGNITION_FUNCTION_NAME so-path=$FACE_RECOGNITION_POST_SO qos=false ! \
        queue name=recognition_pre_agg_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0"

    PERSON_DETECTION_PIPELINE="\
        video/x-raw,format=RGBA ! \
        hailocropper filter-streams=\"<$person_attribute_streams>\" so-path=$WHOLE_BUFFER_CROP_SO function-name=create_crops internal-offset=$internal_offset name=person_detect_cropper \
        hailoaggregator name=person_detect_agg \
        person_detect_cropper. ! \
            queue name=person_detect_bypass_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        person_detect_agg. \
        person_detect_cropper. ! \
            video/x-raw,format=RGBA, width=1920, height=1080 ! \
            queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            videoscale name=person_videoscale method=0 n-threads=2 add-borders=false qos=false ! \
            queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            hailonet scheduling-algorithm=1 hef-path=$PERSON_DETECTION_HEF_PATH vdevice-key=$DEFAULT_VDEVICE_KEY ! \
            queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            hailofilter name=person_detector_hailofilter so-path=$PERSON_DETECTION_POSTPROCESS_SO config-path=$PERSON_JSON_CONFIG_PATH function-name=$PERSON_DETECTION_FUNCTION_NAME qos=false ! \
            queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            video/x-raw,format=RGBA ! \
        person_detect_agg. \
        person_detect_agg. ! video/x-raw,format=RGBA"

    FACE_DETECTION_PIPELINE="\
        video/x-raw,format=RGBA ! \
        hailocropper filter-streams=\"<$face_attribute_streams, $face_recognition_streams>\" so-path=$WHOLE_BUFFER_CROP_SO function-name=create_crops internal-offset=$internal_offset name=face_detect_cropper \
        hailoaggregator name=face_detect_agg \
        face_detect_cropper. ! \
            queue name=face_detect_bypass_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        face_detect_agg. \
        face_detect_cropper. ! \
            video/x-raw,format=RGBA, width=1920, height=1080 ! \
            queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            videoscale name=face_videoscale method=0 n-threads=2 add-borders=false qos=false ! \
            queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            videoconvert ! video/x-raw,format=RGB ! \
            queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            hailonet scheduling-algorithm=1 hef-path=$FACE_DETECTION_HEF_PATH vdevice-key=$DEFAULT_VDEVICE_KEY ! \
            queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            hailofilter name=face_detector_hailofilter so-path=$FACE_DETECTION_POSTPROCESS_SO config-path=$FACE_JSON_CONFIG_PATH function-name=$FACE_DETECTION_FUNCTION_NAME qos=false ! \
            videoconvert ! video/x-raw,format=RGBA ! \
            queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        face_detect_agg. \
        face_detect_agg. ! video/x-raw,format=RGBA"

    PERSON_ATTR_PIPELINE="router.src_0 ! \
            queue name=pre_person_cropper_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            hailocropper so-path=$VMS_CROP_SO function-name=$PERSON_ATTR_CROP_FUNC internal-offset=$internal_offset name=person_attr_cropper \
            hailoaggregator name=person_attr_agg \
            person_attr_cropper. ! \
                queue name=person_attr_bypass_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            person_attr_agg. \
            person_attr_cropper. ! \
                $PERSON_ATTR_INFER_POST ! \
            person_attr_agg. \
            person_attr_agg. ! \
                queue name=person_attr_fakesink_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
                fakesink name=person_hailo_display sync=false "

    FACE_ATTR_PIPELINE="router.src_1 ! \
        queue name=pre_face_cropper_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailocropper so-path=$VMS_CROP_SO function-name=$FACE_ATTR_CROP_FUNC internal-offset=$internal_offset name=face_attr_cropper \
        hailoaggregator name=face_attr_agg \
        face_attr_cropper. ! \
            queue name=face_attr_bypass_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        face_attr_agg. \
        face_attr_cropper. ! \
            $FACE_ATTR_INFER_POST ! \
        face_attr_agg. \
        face_attr_agg. ! \
            queue name=face_attr_fakesink_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            fakesink name=face_attr_hailo_display sync=false "
    
    FACE_RECOGNITION_PIPELINE="router.src_2 ! \
        queue name=pre_face_rec_cropper_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailocropper so-path=$VMS_CROP_SO function-name=face_recognition internal-offset=$internal_offset name=face_rec_cropper \
        hailoaggregator name=face_rec_agg \
        face_rec_cropper. ! \
            queue name=face_rec_bypass_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        face_rec_agg. \
        face_rec_cropper. ! \
            $FACE_RECOGNITION_INFER_POST ! \
        face_rec_agg. \
        face_rec_agg. ! \
            queue name=face_rec_fakesink_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            fakesink name=face_rec_hailo_display sync=false "

    DISPLAY_PIPELINE_BRANCH="\
        queue name=pre_overlay_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        videoscale name=comp_videoscale_$n method=0 n-threads=6 add-borders=false qos=false ! \
        video/x-raw, pixel-aspect-ratio=1/1, width=640, height=640 ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        $streamrouter_disp_element "

    DETECTOR_PIPELINE="\
        queue name=pre_face_detector_infer_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        $FACE_DETECTION_PIPELINE ! \
        queue name=pre_face_tracker_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        $FACE_TRACKER ! \
        queue name=post_face_tracker_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        $PERSON_DETECTION_PIPELINE ! \
        queue name=pre_person_tracker_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        $PERSON_TRACKER ! \
        queue name=post_person_tracker_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        hailogallery gallery-file-path=$GALLERY_CONFIG \
        load-local-gallery=true similarity-thr=.4 gallery-queue-size=30 class-id=-1 ! \
        queue name=pst_gallery_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        tee name=disp_t \
        disp_t. ! \
            $DISPLAY_PIPELINE_BRANCH \
        disp_t."

    pipeline_comp_displaysink="\
            compositor background=1 name=comp start-time-selection=0 $compositor_locations ! \
            queue name=hailo_video_q_0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=false text-overlay=false"

    pipeline="gst-launch-1.0 \
            $sources \
            hailoroundrobin name=roundrobin funnel-mode=false ! \
            queue leaky=no name=pre_detector_pipe_q max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            $DETECTOR_PIPELINE ! \
            queue name=post_detector_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
            hailostreamrouter name=router $streamrouter_input_streams \
            $pipeline_comp_displaysink \
                $FACE_ATTR_PIPELINE \
                $PERSON_ATTR_PIPELINE \
                $FACE_RECOGNITION_PIPELINE \
            ${additonal_parameters}"

    echo ${pipeline}
    if [ "$print_gst_launch_only" = true ]; then
        exit 0
    fi

    echo "Running Pipeline..."
    eval "${pipeline}"

}

main $@
