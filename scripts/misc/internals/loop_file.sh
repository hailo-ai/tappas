#!/bin/bash

set -e

# Check that the input file and number of copies have been specified
if [ $# -ne 2 ]; then
    echo "Usage: $0 [input_file] [number of copies]"
    exit 1
fi

input_file=$1
n=$2

# Create a new output file by appending "_looped" to the input file name
output_file="${input_file%.*}_looped.${input_file##*.}"

# Use ffmpeg to loop the input file $n times
ffmpeg -i "$input_file" -filter_complex "loop=$n:size=99" "$output_file"

echo "Looped video saved to $output_file"