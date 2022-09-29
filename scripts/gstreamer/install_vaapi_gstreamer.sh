#!/bin/bash
set -e

RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

function log_info() {
    echo -e "${BLUE}INFO:${NC} ${1}"
}

function log_error() {
    echo -e "${RED}ERROR:${NC} ${1}"
}

function log_warning() {
    echo -e "${YELLOW}WARN:${NC} ${1}"
}

function check_if_found_in_lspci() {
    log_info "Running lspci and searcing for a VGA device"

    if [[ $(lspci | grep VGA | wc -l) -eq 0 ]]; then
        log_error "No VGA device were found"
        exit 1
    fi

    log_info "Checking lspci done successfully"
}

function install_and_check_drivers() {
    log_info "Installing and checking intel graphic drivers"

    sudo apt-get install -y va-driver-all

    if [[ $(ls /usr/lib/x86_64-linux-gnu/dri | grep drv_video.so | wc -l) -eq 0 ]]; then
        log_error "No Intel graphic driver was found"
        exit 1
    fi

    log_info "Checking intel graphic drivers done successfully"
}

function check_dev_dri_found() {
    files_at_dri_dir=$(find /dev/dri/ -maxdepth 1 -not -type d)
    files_at_dri_dir_wc=$(find /dev/dri/ -maxdepth 1 -not -type d | wc -l)

    if [[ $files_at_dri_dir_wc -lt 2 ]]; then
        log_error "Expecting at least two files at /dev/dri (card and render)"
        exit 1
    fi

    user_groups=$(groups)

}

function check_vainfo() {
    log_info "Installing and checking vainfo"

    sudo apt-get install -y vainfo

    set +e
    vainfo
    vainfo_results=$?
    set -e

    if [[ $vainfo_results -ne 0 ]]; then
        log_error "vainfo failed, please make sure you exported LIBVA_DRIVER_NAME and LIBVA_DRIVERS_PATH"
        log_warning "current state: LIBVA_DRIVER_NAME=$LIBVA_DRIVER_NAME and LIBVA_DRIVERS_PATH=$LIBVA_DRIVERS_PATH"
        exit 1
    fi

    log_info "Checking vainfo done successfully"
    log_warning "Its recommended to reboot after this stage"
}

function install_gstreamer_vaapi() {
    log_info "Installing and checking gstreamer VAAPI"

    sudo apt-get install -y gstreamer1.0-vaapi

    log_info "Installed succesfuly, exporting GST_VAAPI_ALL_DRIVERS"
    export GST_VAAPI_ALL_DRIVERS=1

    set +e
    gst_inspect_vaapi_output=$(gst-inspect-1.0 vaapi)
    gst_inspect_vaapi_return_code=$?
    set -e

    if [[ $gst_inspect_vaapi_return_code -ne 0 ]]; then
        log_error "No GStreamer VAAPI elements were found"
        exit 1
    fi
}

function main() {
    if [[ -z "$LIBVA_DRIVER_NAME" ]]; then
        log_warning "LIBVA_DRIVER_NAME is not set, exiting"
        exit 1
    fi

    check_if_found_in_lspci
    install_and_check_drivers
    check_dev_dri_found
    check_vainfo
    install_gstreamer_vaapi
}

main
