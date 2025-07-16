#!/bin/bash
set -e

script_dir=$(dirname $(realpath "$0"))

function init_variables() {
    print_help_if_needed
    time=''
    output_path='out'
    command=''
    sampling_frequency=99
    additional_flags=''
}

function print_usage() {
    echo "Record perf data and convert it to flamegraph:"
    echo ""
    echo "Options:"
    echo "  --help                  Show this help"
    echo "  -t TIME --time TIME     Set Time to run"
    echo "  --output-path           Path of the output file name"
    echo "  --command               Measure only a specific command resources (recording the whole OS by default)"
    echo "  -F FREQ --freq FREQ     Sets the sampling frequency of the performance counter in Hertz"
    echo "                          which determines the frequency of samples taken to generate profiling data."
    echo "                          Higher frequencies can provide more detailed profiling information, but may also increase overhead and impact system performance"
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
        if [ "$1" = "--time" ] || [ "$1" == "-t" ]; then
            additional_flags="-- sleep $2"
            shift
        elif [ "$1" = "--output-path" ]; then
            output_path="$2"
            shift
        elif [ "$1" = "--command" ]; then            
            # Shift until there are no more arguments starting with a hyphen
            while [ $# -gt 1 ] && [[ "$2" != -* ]]; do
                command+=" $2"
                shift
            done
        elif [ "$1" = "--freq" ] || [ "$1" == "-F" ]; then
            sampling_frequency="$2"
            shift            
        else
            echo "Received invalid argument: $1. See expected arguments below:"
            print_usage
            exit 1
        fi

        shift
    done
}

function check_requirements() {
    if ! [ -x "$(command -v perf)" ]; then
        echo "perf is not installed, installing..."
        sudo apt install linux-tools-common linux-tools-generic linux-tools-$(uname -r)
    fi

    if [ ! -d "$script_dir/flamegraph" ]; then
        echo "Cloning flamegraph..."
        git clone https://github.com/brendangregg/FlameGraph.git $script_dir/flamegraph
    fi
}

function main() {
    init_variables
    check_requirements
    parse_args $@

    if [ -z "${time}" ] && [ -z "${command}" ]; then
        echo "Time or command is expected, exiting"
        exit 1
    fi

    record_tmp_dir=$script_dir/tappas_perf_record_$(date +%d_%m_%Y_%H_%M_%S)
    mkdir "$record_tmp_dir"
    pushd $record_tmp_dir

    # Flags explained:
    #   -a : All cpus
    #   -g : Enables call-graph
    #   -F : Profile at this frequency (samples per second)
    record_command="sudo perf record -F ${sampling_frequency} -a -g ${additional_flags} ${command}"
    echo "Running ${record_command}"
    eval "${record_command}"

    echo "Creating flamegraph..."
    sudo perf script > out.perf

    $script_dir/flamegraph/stackcollapse-perf.pl out.perf > $output_path.folded
    $script_dir/flamegraph/flamegraph.pl --colors=java $output_path.folded > $output_path.svg
    echo "Done! $(realpath $output_path.folded).folded created."
    echo "enter https://flamegraph.com/ or https://github.com/KDAB/hotspot"

    popd
}

main $@
