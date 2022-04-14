#!/bin/bash

RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

function log_error() {
    echo -e "${RED}ERROR:${NC} ${1}"
}

function log_warning() {
    echo -e "${YELLOW}WARN:${NC} ${1}"
}

function export_workspaces() {
    if [[ -z "$TAPPAS_WORKSPACE" ]]; then
        SCRIPT_DIR=$(dirname $(realpath $0))
        export TAPPAS_WORKSPACE=$(readlink -f $SCRIPT_DIR/../../../../)
    fi
}

function validate_hailo_device_connected() {
    devices_count=`lspci -d 1e60: | wc -l`

    if (( $devices_count == 0 )); then
        log_error "No Hailo-8 devices found. Please connect the device and try again"
        return 1
    fi
}

function validate_hailort_version() {
    if ! [ -x "$(command -v hailortcli)" ]; then
        log_error "hailortcli was not found"
        return 1
    fi

    if [ ! -f "$TAPPAS_WORKSPACE/.config" ]; then
        log_warning "Can't validate if hailort version is supported"
        return 0
    fi 

    hailort_version=$(hailortcli fw-control identify | grep -a "Firmware Version" | grep -aoP "(\d+.\d+.\d+)")
    hailort_expected_version=$(cat $TAPPAS_WORKSPACE/.config | awk -F'=' '{print $2}')

    if [ "$hailort_version" != "$hailort_expected_version" ]; then
        log_warning "HailoRT version is $hailort_version, expected to be $hailort_expected_version"
        return 1
    fi
}

function export_xv_image_is_supported() {
    # Check if an adapter that are accessible through the X-Video extension is found
    if xvinfo | grep -q 'no adaptors present'; then
        echo "No XV adaptors found, using ximagesink instead"
        export XV_SUPPORTED=false
    else
        export XV_SUPPORTED=true
    fi

}

function main() {
    functions_to_run=( export_workspaces validate_hailo_device_connected validate_hailort_version export_xv_image_is_supported )

    for func_name in "${functions_to_run[@]}"
    do  
        $func_name
        return_code=$?

        if [ $return_code -ne 0 ]; then
            return $return_code
        fi
    done
}

main