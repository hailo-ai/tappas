#!/bin/bash
set -e

CURRENT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

function init_variables() {
    print_help_if_needed $@

    # Basic Directories
    readonly POSTPROCESS_DIR="/usr/lib/hailo-post-processes"
    readonly CROPPING_ALGORITHMS_DIR="$POSTPROCESS_DIR/cropping_algorithms"
    readonly RESOURCES_DIR="${CURRENT_DIR}/resources"

    # Default Video
    readonly DEFAULT_VIDEO_SOURCE="/dev/video0"

    readonly FOUR_K_BITRATE=25000000
    readonly HD_BITRATE=6000000
    readonly SD_BITRATE=3000000

    readonly DEFAULT_MAX_BUFFER_SIZE=5
    readonly DEFAULT_FORMAT="NV12"
    readonly DEFAULT_BITRATE=25000000
    readonly DEFAULT_FRONTEND_CONFIG_FILE_PATH="$RESOURCES_DIR/configs/frontend_config.json"

    input_source=$DEFAULT_VIDEO_SOURCE

    json_config_path_4k="$RESOURCES_DIR/configs/osd_4k.json"
    json_config_path_fhd="$RESOURCES_DIR/configs/osd_fhd.json"
    json_config_path_hd="$RESOURCES_DIR/configs/osd_hd.json" 
    json_config_path_sd="$RESOURCES_DIR/configs/osd_sd.json"
    
    frontend_config_file_path="$DEFAULT_FRONTEND_CONFIG_FILE_PATH"

    encoding_hrd="hrd=false"

    # Limit the encoding bitrate to 10Mbps to support weak host.
    # Comment this out if you encounter a large latency in the host side
    # Tune the value down to reach the desired latency (will decrease the video quality).
    # ----------------------------------------------
    # bitrate=10000000
    # encoding_hrd="hrd=true hrd-cpb-size=$bitrate"
    # ----------------------------------------------

    max_buffers_size=$DEFAULT_MAX_BUFFER_SIZE

    print_gst_launch_only=false
    additonal_parameters=""
    video_format=$DEFAULT_FORMAT
    sync_pipeline=false
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
    echo "Basic security camera streaming pipeline usage:"
    echo ""
    echo "Options:"
    echo "  -h --help                  Show this help"
    echo "  --show-fps                 Print fps"
    echo "  --print-gst-launch         Print the ready gst-launch command without running it"
    echo "  -i --input-source          Set the input source (default $DEFAULT_VIDEO_SOURCE)"
    echo "  --vision-config-file-path  Set the frontend config file path (default $DEFAULT_FRONTEND_CONFIG_FILE_PATH)"
    exit 0
}

function parse_args() {
    while test $# -gt 0; do
        if [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
        elif [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            additonal_parameters="-v | grep -e hailo_display"
        elif [ "$1" = "-i" ] || [ "$1" = "--input-source" ]; then
            input_source="$2"
            shift
        elif [ "$1" = "--vision-config-file-path" ]; then
            frontend_config_file_path="$2"
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

function create_pipeline() {

    FPS_DISP="fpsdisplaysink text-overlay=false sync=$sync_pipeline video-sink=fakesink"

    UDP_SINK="queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
              rtph264pay ! 'application/x-rtp, media=(string)video, encoding-name=(string)H264' ! \
              udpsink host=10.0.0.2 sync=$sync_pipeline"

    FOUR_K_TO_ENCODER_BRANCH="queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
                            hailoosd config-file-path=$json_config_path_4k ! \
                            queue leaky=downstream max-size-buffers=2 max-size-bytes=0 max-size-time=0 ! \
                            hailoh264enc bitrate=$FOUR_K_BITRATE hrd=false ! \
                            video/x-h264 ! \
                            tee name=fourk_enc_tee \
                            fourk_enc_tee. ! \
                                $UDP_SINK port=5000 \
                            fourk_enc_tee. ! \
                                queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
                                $FPS_DISP name=hailo_display_4k_enc "

    FOUR_K_BRANCH="queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
                   $FPS_DISP name=hailo_display_4k "

    # In the FHD branch we do not output the stream into Encoder, due to hardware limitation.
    FHD_BRANCH="queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
                $FPS_DISP name=hailo_display_fhd "

    HD_BRANCH="queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
               hailoosd config-file-path=$json_config_path_hd ! \
               queue leaky=downstream max-size-buffers=2 max-size-bytes=0 max-size-time=0 ! \
               hailoh264enc bitrate=$HD_BITRATE $encoding_hrd ! \
               video/x-h264 ! \
                tee name=hd_tee \
                hd_tee. ! \
                    $UDP_SINK port=5002 \
                hd_tee. ! \
                    queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
                    $FPS_DISP name=hailo_display_hd_enc "

    SD_BRANCH="queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
               hailoosd config-file-path=$json_config_path_sd ! \
               queue leaky=downstream max-size-buffers=2 max-size-bytes=0 max-size-time=0 ! \
               hailoh264enc bitrate=$SD_BITRATE $encoding_hrd ! \
               video/x-h264 ! \
                tee name=sd_tee \
                sd_tee. ! \
                    $UDP_SINK port=5004 \
                sd_tee. ! \
                    queue leaky=no max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
                    $FPS_DISP name=hailo_display_sd_enc "

}

create_pipeline $@

PIPELINE="${debug_stats_export} gst-launch-1.0 \
    v4l2src io-mode=mmap device=$input_source name=src_0 ! video/x-raw, width=3840, height=2160, framerate=30/1, format=$video_format ! \
    queue leaky=downstream max-size-buffers=$max_buffers_size max-size-bytes=0 max-size-time=0 ! \
    hailofrontend config-file-path=$frontend_config_file_path name=preproc \
    preproc. ! $FOUR_K_TO_ENCODER_BRANCH \
    preproc. ! $FOUR_K_BRANCH \
    preproc. ! $FHD_BRANCH \
    preproc. ! $HD_BRANCH \
    preproc. ! $SD_BRANCH \
    ${additonal_parameters} "

echo "Running Pipeline..."
echo ${PIPELINE}

if [ "$print_gst_launch_only" = true ]; then
    exit 0
fi

eval ${PIPELINE}