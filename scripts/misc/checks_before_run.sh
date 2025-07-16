#!/bin/bash

RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

check_python_packages=false
export_only=false
check_vaapi=false
no_hailo=false

INSTALLATION_DIR=/usr
TAPPAS_LIB_PATH=${INSTALLATION_DIR}/lib/$(uname -m)-linux-gnu
TAPPAS_GST_PLUGIN_PATH=${TAPPAS_LIB_PATH}/gstreamer-1.0

function check__print_usage() {
    echo "Checks before app run:"
    echo ""
    echo "Options:"
    echo "  --help                   Show this help"
    echo "  --check-python-packages  Check if python packages exists"
    echo "  --export-only            Handle setting TAPPAS_WORKSPACE only mode"
    echo "  --check-vaapi            Include checks for VAAPI pipelines"
    echo "  --no-hailo               Exclude checks for Hailo device"
    exit 1
}

function check__parse_args() {
    # Unknown arguemnt could be argument of our parent
    # https://unix.stackexchange.com/questions/541878/prevent-bashscript-argument-being-transferred-to-a-child-sourced-script
    while test $# -gt 0; do
        if [[ "$1" == "-h" || "$1" == "--help" ]]; then
            check__print_usage
        elif [ "$1" == "--check-python-packages" ]; then
            check_python_packages=true
        elif [ "$1" == "--export-only" ]; then
            export_only=true
        elif [ "$1" == "--check-vaapi" ]; then
            check_vaapi=true
        elif [ "$1" == "--no-hailo" ]; then
            no_hailo=true
        fi
        shift
    done
}


function log_error() {
    echo -e "${RED}ERROR:${NC} ${1}"
}

function log_warning() {
    echo -e "${YELLOW}WARN:${NC} ${1}"
}

function export_workspaces() {
    if [[ -z "$TAPPAS_WORKSPACE" ]]; then
        SCRIPT_DIR=$(dirname $(realpath $0))
        export TAPPAS_WORKSPACE=$(readlink -f $SCRIPT_DIR/../../../../../)
    fi
}

function validate_hailo_device_connected() {
    devices_count=`lspci -d 1e60: | wc -l`

    if (( $devices_count == 0 )); then
        log_error "No Hailo-8 devices found. Please connect the device and try again"
        return 1
    fi
}

function check_if_python_package_found() {
    # The two methods might be suitable, one for packages we install through pip and
    # The other for packages we manually add through `pth files`.
    set +e
    python3 -c "import pkg_resources; pkg_resources.require('$1')" &> /dev/null
    package_import_return_code_method_pkg=$?
    set -e

    if [[ package_import_return_code_method_pkg -eq 0 ]]; then
        return 0
    fi

    set +e
    python3 -c "import $1" &> /dev/null
    package_import_return_code_method_direct=$?
    set -e

    if [[ $package_import_return_code_method_direct -eq 0 ]]; then        
        return 0
    fi

    log_warning "Pip package '${1}' is missing, Perhaps the virtualenv is not activated?"
    return 1
}

function check_python_packages_found() {
    cat $TAPPAS_WORKSPACE/core/requirements/gstreamer_requirements.txt | while read line 
    do
        check_if_python_package_found $line
    done

    check_if_python_package_found "gsthailo"    
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

function add_vaapi_fakedriver() {
    export LIBVA_DRIVER_NAME=fakedriver
}

function check_vaapi_driver() {
    if [[ -z "$LIBVA_DRIVER_NAME" ]]; then
        log_warning "LIBVA_DRIVER_NAME is not set, sourcing VA-API 'set_env' file"
        . $TAPPAS_WORKSPACE/scripts/vaapi/set_env.sh
    fi
}

function _export_ld_library_path(){
    ! [[ -n $LD_LIBRARY_PATH && $LD_LIBRARY_PATH =~ ${TAPPAS_LIB_PATH} ]] && \
        export LD_LIBRARY_PATH="${TAPPAS_LIB_PATH}:${LD_LIBRARY_PATH}"
    return 0
}

function _export_gst_plugin_path(){
    ! [[ -n $GST_PLUGIN_PATH && $GST_PLUGIN_PATH =~ ${TAPPAS_GST_PLUGIN_PATH} ]] && \
        export GST_PLUGIN_PATH="${TAPPAS_GST_PLUGIN_PATH}:${GST_PLUGIN_PATH}"
    return 0
}

function export_env_path_vars(){
    _export_ld_library_path
    _export_gst_plugin_path
}

function main() {
    functions_to_run=( export_workspaces export_xv_image_is_supported export_env_path_vars)

    if [ "$no_hailo" = false ]; then
        functions_to_run+=( validate_hailo_device_connected )
    fi

    if [ "$check_python_packages" = true ]; then
        functions_to_run+=( check_python_packages_found )
    fi

    if [ "$check_vaapi" = true ]; then
        functions_to_run+=( check_vaapi_driver )
    else
        functions_to_run+=( add_vaapi_fakedriver )
    fi

    if [ "$export_only" = true ]; then
        functions_to_run=( export_workspaces )
    fi

    for func_name in "${functions_to_run[@]}"
    do  
        $func_name
        return_code=$?

        if [ $return_code -ne 0 ]; then
            return $return_code
        fi
    done
}

check__parse_args $@
main
