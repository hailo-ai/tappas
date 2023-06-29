#!/bin/bash
set -e

function init_variables() {
    # Assure all check are ready before running the pipeline
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../scripts/misc/checks_before_run.sh --no-hailo

    decode_element="decodebin"
    num_of_src=4
    num_of_buffers=500
    additional_parameters=""
    sources=""
    check_vaapi=""
    print_gst_launch_only=false
    add_video_scale=false
    use_vaapi_scale=false
    converted_format=",format=I420"
    scale_element=""
    video_prefix_path=""
    video_sink_element=$([ "$XV_SUPPORTED" = "true" ] && echo "xvimagesink" || echo "ximagesink")
}

function print_usage() {
    echo "Run Decoding Display Pipeline - pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                              Show this help"
    echo "  --video-prefix-path                 Video prefix path"
    echo "  --show-fps                          Printing fps"
    echo "  --num-of-sources NUM                Setting number of sources to given input (Default is $num_of_src)"
    echo "  --use-vaapi                         Whether to use vaapi decodeing or not"
    echo "  --use-vaapi-scale                   Whether to use vaapi scaling or not (Defautl is CPU scale)"
    echo "  --no-display                        Whether to use display or not"
    echo "  --display-resolution RESOLUTION     Scale width and height of each stream in WxH mode (e.g. 640x480)"
    echo "  --format FORMAT                     Required format"
    echo "  --num-of-buffers NUM                Number of buffers for each stream (Default is $num_of_buffers)"
    echo "  --print-gst-launch                  Print the ready gst-launch command without running it"
    exit 0
}

function parse_args() {
    while test $# -gt 0; do
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage
        elif [ "$1" = "--print-gst-launch" ]; then
            print_gst_launch_only=true
        elif [ "$1" = "--show-fps" ]; then
            echo "Printing fps"
            additional_parameters="-v | grep hailo_display"
        elif [ "$1" = "--video-prefix-path" ]; then
            shift
            video_prefix_path=$1
        elif [ "$1" = "--use-vaapi" ]; then
            check_vaapi="--check-vaapi"
        elif [ "$1" = "--use-vaapi-scale" ]; then
            use_vaapi_scale=true
        elif [ "$1" = "--no-display" ]; then
            video_sink_element=fakesink
        elif [ "$1" = "--display-resolution" ]; then
            shift
            scale_width=$(echo $1 | awk -F 'x' '{print $1}')
            scale_height=$(echo $1 | awk -F 'x' '{print $2}')
            add_video_scale=true
            scale_element="videoscale qos=false ! video/x-raw, width=$scale_width, height=$scale_height, pixel-aspect-ratio=1/1 ! \
                           queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! "

        elif [ "$1" = "--format" ]; then
            shift
            converted_format_caps=",format=$1"
        elif [ "$1" = "--num-of-sources" ]; then
            shift
            echo "Setting number of sources to $1"
            num_of_src=$1
        elif [ "$1" = "--num-of-buffers" ]; then
            shift
            num_of_buffers=$1
        else
            echo "Received invalid argument: $1. See expected arguments below:"
            print_usage
            exit 1
        fi
        shift
    done
}

function post_parse_args() {
    if [ -z "${video_prefix_path}" ]; then
        echo "video prefix path must be specified"
    fi

    width_and_height=$(ffprobe -v error -select_streams v:0 -show_entries stream=width,height -of csv=s=x:p=0 "${video_prefix_path}0.mp4")
    width=$(echo $width_and_height | awk -F 'x' '{print $1}')
    height=$(echo $width_and_height | awk -F 'x' '{print $2}')

    # In case where there is no need to scale, set the scale as the width and height of the picture
    if [ "$add_video_scale" = false ]; then
        scale_width=$width
        scale_height=$height
    fi

    if [ -n "${check_vaapi}" ]; then

        # There are cases where we want to use the scale on CPU/GPU, support both of them
        if [ "$add_video_scale" = true ] && [ "$use_vaapi_scale" = true ]; then
            width=$scale_width
            height=$scale_height
            scale_element=""
        fi

        decode_element="qtdemux ! vaapidecodebin ! video/x-raw,width=$width,height=$width"

        if [ -n "${converted_format_caps}" ]; then
            decode_element+=$converted_format_caps
        fi

        source $script_dir/../../scripts/vaapi/set_env.sh
    else
        decode_element="decodebin ! video/x-raw, width=$width, height=$height"
    fi

}

function create_sources() {
    for ((n = 0; n < $num_of_src; n++)); do
        sources+="filesrc num-buffers=$num_of_buffers location=${video_prefix_path}$n.mp4 name=source_$n ! \
                $decode_element ! \
                queue name=hailo_preprocess_q_$n leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
                $scale_element \
                videoconvert qos=false ! video/x-raw$converted_format_caps ! queue name=hailo_predemux_q_$n leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
                demuxer.sink_$n "
    done
}

function main() {
    init_variables $@
    parse_args $@
    post_parse_args
    script_dir=$(dirname $(realpath "$0"))
    source $script_dir/../../scripts/misc/checks_before_run.sh $check_vaapi --no-hailo
    create_sources

    pipeline="gst-launch-1.0 \
        funnel name=demuxer ! \
        queue name=hailo_video_q_0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
        fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=false text-overlay=false $sources ${additional_parameters}"

    echo ${pipeline}
    if [ "$print_gst_launch_only" = true ]; then
        exit 0
    fi

    echo "Running Pipeline..."
    eval "${pipeline}"

}

main $@
