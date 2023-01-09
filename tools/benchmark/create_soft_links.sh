output_prefix='video'
num_of_copies=''
input=''

function print_usage() {
    echo "Create soft links usage:"
    echo ""
    echo "Options:"
    echo "  --help                          Show this help"
    echo "  --num-of-copies NUM      Num of copies to make"
    echo "  --input STR              Input video"
    echo "  --output-prefix STR      Prefix of the output video"
    exit 0
}

function parse_args() {
    while test $# -gt 0; do
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage
        elif [ "$1" = "--num-of-copies" ]; then
            shift
            num_of_copies=$1
        elif [ "$1" = "--input" ]; then
            shift
            input=$1
        elif [ "$1" = "--output-prefix" ]; then
            shift
            output_prefix=$1
        else
            echo "Received invalid argument: $1. See expected arguments below:"
            print_usage
            exit 1
        fi
        shift
    done
}

function main() {
    parse_args $@
    rm -f ${output_prefix}*.mp4

    for ((n = 0; n < $num_of_copies; n++)); do
        ln -s "${input}" "${output_prefix}${n}.mp4"
    done
}

main $@
