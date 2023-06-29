#!/bin/bash
set -e

function init_variables() {
    if [[ -z "$TAPPAS_WORKSPACE" ]]; then
        BENCHMARK_DIR=$(dirname $(realpath $0))
        export TAPPAS_WORKSPACE=$(readlink -f $BENCHMARK_DIR/../../)
    fi

    min_num_of_src=1
    max_num_of_src=4
    converted_format=""
    num_of_buffers=500
    use_vaapi=''
    use_vaapi_scale=''
    no_display='--no-display'
    display_resolution=''
    video_prefix_path=""
}

function print_usage() {
    echo "Decoding Display Pipeline - pipeline usage:"
    echo ""
    echo "Options:"
    echo "  --help                              Show this help"
    echo " "
    echo "  --video-prefix-path PATH            Video prefix path"
    echo " "
    echo "  --min-num-of-sources NUM            Setting number of sources to given input (Default is $min_num_of_src)"
    echo "  --max-num-of-sources NUM            Setting number of sources to given input (Default is $max_num_of_src)"
    echo "  --num-of-buffers NUM                Number of buffers for each stream (Default is $num_of_buffers)"
    echo " "
    echo "  --use-vaapi                         Whther to use vaapi decodeing or not (Default is no vaapi)"
    echo "  --use-vaapi-scale                   Whether to use vaapi scaling or not (Defautl is CPU scale)"
    echo "  --use-display                       Whther to use display or not (Default is no display)"
    echo " "
    echo "  --format FORMAT                     Required format"
    echo "  --display-resolution RESOLUTION     Scale width and height of each stream in WxH mode (e.g. 640x480)"
    exit 0
}

function parse_args() {
    while test $# -gt 0; do
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage
        elif [ "$1" = "--use-vaapi" ]; then
            use_vaapi="--use-vaapi"
        elif [ "$1" = "--use-vaapi-scale" ]; then
            use_vaapi_scale="--use-vaapi-scale"
        elif [ "$1" = "--format" ]; then
            shift
            converted_format="--format $1"
        elif [ "$1" = "--video-prefix-path" ]; then
            shift
            video_prefix_path=$1
        elif [ "$1" = "--display-resolution" ]; then
            shift
            display_resolution="--display-resolution $1"
        elif [ "$1" = "--use-display" ]; then
            no_display=''
        elif [ "$1" = "--min-num-of-sources" ]; then
            shift
            min_num_of_src=$1
        elif [ "$1" = "--max-num-of-sources" ]; then
            shift
            max_num_of_src=$1
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

function check_invalid_combinations() {
    if [ "$no_display" = '' ] && [ "$converted_format" = "--format RGBA" ]; then
        echo "Received invalid configuration. Cannot use display with RGBA format."
        exit 2
    fi
}

function main() {
    init_variables $@
    parse_args $@
    check_invalid_combinations

    for ((n = $min_num_of_src; n <= $max_num_of_src; n++)); do
        avg_fps=$($TAPPAS_WORKSPACE/tools/decoding_display_pipeline/run_decoding_display_pipeline.sh --video-prefix-path \
                  $video_prefix_path $no_display $display_resolution $use_vaapi $use_vaapi_scale \
                  --num-of-sources $n --num-of-buffers $num_of_buffers $converted_format --show-fps \
                  | grep -oP 'average: (\d+.\d+)' | awk '{print $2}' | awk 'BEGIN{s=0;}{s+=$1;}END{print s/NR;}')
        echo "Num of sources: ${n}, Average FPS: ${avg_fps}"
    done
}

main $@
