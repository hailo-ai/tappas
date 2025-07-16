#!/bin/bash
set -e

function print_usage() {
    echo "Analyze GStreamer log file:"
    echo ""
    echo "Options:"
    echo "  --help                  Show this help"
    echo "  -i INPUT --input INPUT  Set the camera source (default $input_source)"
    echo "  --show-fps              Print fps"
    echo '  --bitrate-element-filter Filter the bitrate element to parse (default udpsrc)'
    echo "  --gst-log-file          Set the gst log file (default $gst_log_file)"
    echo '  --latency-element-filter Filter the latency element to parse (default sink)'
    echo "  --print-gst-launch      Print the ready gst-launch command without running it"
    exit 0
}

bitrate_element_filter="udpsrc"
latency_element_filter="sink"
gst_log_file="/tmp/gst.log"
tracers="bitrate;framerate;interlatency"

function parse_args() {
    while test $# -gt 0; do
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage
            exit 0
        elif [ "$1" = "--time-to-run" ] || [ "$1" == "-t" ]; then
            time_to_run=$2
            shift
        elif [ "$1" = "--udp-port" ] || [ "$1" == "-p" ]; then
            udp_port=$2
            shift
        elif [ "$1" = "--bitrate-element-filter" ]; then
            bitrate_element_filter=$2
            shift
        elif [ "$1" = "--latency-element-filter" ]; then
            latency_element_filter=$2
            shift
        elif [ "$1" = "--gst-log-file" ]; then
            gst_log_file=$2
            shift
        elif [ "$1" = "--tracers" ]; then
            tracers=$2
            shift
        else
            echo "Received invalid argument: $1. See expected arguments below:"
            print_usage
            exit 1
        fi

        shift
    done
}

function parse_framerate_results(){
    rm -f framerate_sorted.txt
    cat $gst_log_file | grep framerate | awk '{print $NF}' |  awk -F '(uint))' '{print $2}' | awk -F ';' '{print $1}' | sort -n > framerate_sorted.txt
    lowest_framerate=$(awk '$0==($0+0)' framerate_sorted.txt | head -n 1)
    echo "lowest framerate: $lowest_framerate"
    echo "slowest element:"
    cat $gst_log_file | grep framerate | grep "fps=(uint)$lowest_framerate"
}

function parse_bitrate_results() {
    echo ""
    echo "Bitrate over time:"
    cat $gst_log_file | grep $bitrate_element_filter | grep bitrate

    rm -f bitrate.txt
    cat $gst_log_file | grep $bitrate_element_filter | grep bitrate | awk '{print $NF}' | awk -F '(guint64))' '{print $2}' | awk -F ';' '{print $1}' | sort -n > bitrate.txt

    echo ""
    echo "Lowest bitrate:"
    head -n 1 bitrate.txt

    num_of_lines=0
    num_of_lines=$(cat bitrate.txt | wc -l)
    sum=0
    while IFS= read -r line; do
        sum=$(echo "$sum + $line" | bc)
    done < bitrate.txt
    echo ""
    echo "Average bitrate:"
    bc -l <<< "$sum / $num_of_lines"
}

function parse_latency_results() {
    rm -f latency.txt
    cat $gst_log_file | grep $latency_element_filter | grep src | grep latency | awk '{print $NF}' | awk -F '0:00:0' '{print $2}' | awk -F ';' '{print $1}' > latency.txt
    # cut the file with sed until the number of lines in the file are less than 1000
    num_of_lines=0
    num_of_lines=$(cat latency.txt | wc -l)
    while [ $num_of_lines -gt 2000 ]; do
        sed -i '0~2d' latency.txt
        num_of_lines=$(cat latency.txt | wc -l)
    done

    sum=0
    while IFS= read -r line; do
        sum=$(echo "$sum + $line" | bc)
    done < latency.txt

    echo ""
    echo "num of lines: $num_of_lines"
    echo "Average latency in ms:"
    bc -l <<< "($sum / $num_of_lines) * 1000"

    echo ""
    echo "peak latency in ms:"
    peak_latency=$(cat latency.txt | sort -n | tail -1)
    echo $peak_latency| awk '{print $1 * 1000}'
    cat $gst_log_file | grep $latency_element_filter | grep src | grep latency  | grep $peak_latency

    echo ""
    cout_over_latency=$(cat latency.txt | awk '{if ($1 > 0.6) print $1}' | wc -l)
    echo "got latency over 600ms $cout_over_latency times out of $num_of_lines results"
}

parse_args $@

if [[ $tracers == *"bitrate"* ]]; then
    parse_bitrate_results
fi
if [[ $tracers == *"interlatency"* ]]; then
    parse_latency_results
fi
if [[ $tracers == *"framerate"* ]]; then
    parse_framerate_results
fi