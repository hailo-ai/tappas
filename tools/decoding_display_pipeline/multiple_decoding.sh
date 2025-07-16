#!/bin/bash
set -e

BASE_FOLDER=/tmp/profile
rm -rf $BASE_FOLDER

# Exports for the graphic tracer
export GST_DEBUG_FILE=$TAPPAS_WORKSPACE/run.log
export HAILO_PROFILE_LOCATION=/tmp/profile
export GST_DEBUG_DUMP_DOT_DIR=/tmp/
export GST_DEBUG="GST_TRACER:7"
export GST_TRACERS="graphic"

usecases_json="usecases.json"

function pre_test() {
    mkdir -p $HAILO_PROFILE_LOCATION
    echo $HAILO_PROFILE_LOCATION |  awk -F'/' '{print $NF}'
}

function post_test() {
    direcotry_name=$(echo $HAILO_PROFILE_LOCATION |  awk -F'/' '{print $NF}')
    latest_file="$(ls -Art "${HAILO_PROFILE_LOCATION}/graphic/" | tail -n 1)"

    mv "${HAILO_PROFILE_LOCATION}/graphic/$latest_file" "$HAILO_PROFILE_LOCATION/graphic/p_$direcotry_name.dot"
    cp "$HAILO_PROFILE_LOCATION/graphic/p_$direcotry_name.dot" "$HAILO_PROFILE_LOCATION/../"
    rm -f "$HAILO_PROFILE_LOCATION/graphic/pipe*"

    echo "Done."
    echo "---"
}

function run_tests_json() {
    tests_file_path=$1

    jq -c '.[]' $tests_file_path | while read i; do
        name=$(echo $i | jq -r '.name')
        flags=$(echo $i | jq -r '.flags')

        export HAILO_PROFILE_LOCATION=$BASE_FOLDER/$name
        pre_test
        eval "./decoding_display_pipeline.sh $flags | tee $HAILO_PROFILE_LOCATION/../p_$name.results"
        post_test

    done
}

function main() {

    if ! [ -x "$(command -v jq)" ]; then
        echo "jq is not installed, installing..."
        sudo apt install jq
    fi

    ubuntu_version=$(lsb_release -r | awk '{print $2}' | awk -F'.' '{print $1}')
    if [ $ubuntu_version -eq 20 ]; then
        usecases_json="usecases_nv12_only.json"
    fi

    run_tests_json $usecases_json

    pushd "$BASE_FOLDER"
    for f in ./*.dot; do dot $f -T png > $f.png; done
    
    for f in ./*.results; do 
        echo "$f" >> "concacted_results"
        cat "$f" >> "concacted_results"
        echo "----" >> "concacted_results"       
    done
    popd
}

main
