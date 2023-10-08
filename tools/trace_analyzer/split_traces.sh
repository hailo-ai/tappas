#!/bin/bash
set -e

split_traces_dir=$1
mkdir -p $split_traces_dir

#read value of environment variable GST_TRACERS and split it into an array of tracers, then loop through the array and create a file for each tracer
IFS=';' read -ra tracers <<< "$GST_TRACERS"

for trace in "${tracers[@]}"
do
    if [[ $trace == "graphic" ]]; then
        continue
    fi
    
    cat $GST_DEBUG_FILE | awk '!/gsttracer.c|GST_PIPELINE|gsttracerrecord.c/' | grep $trace | tr -s ' ' > $split_traces_dir/$trace.log
done
