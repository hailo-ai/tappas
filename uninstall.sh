#!/bin/bash

# Hailo Tappas uninstall script
# requires `sudo` privileges

set -e

#############################################
readonly INSTALLATION_DIR=/usr
readonly HAILO_USER_DIR=${HOME}/.hailo/tappas
readonly TAPPAS_BASH_ENV=${HAILO_USER_DIR}/tappas_env
readonly VENV_NAME=hailo_tappas_venv
readonly GSTREAMER_CACHE=${HOME}/.cache/gstreamer-1.0
readonly TAPPAS_VERSION=$(grep -a1 project core/hailo/meson.build | grep version | cut -d':' -f2 | tr -d "', ")

# indication that older version of Tappas (pre-3.24) exists on the host
readonly LIB_DIR=/usr/lib/$(uname -m)-linux-gnu
readonly OLD_TAPPAS_INSTALLATION=${LIB_DIR}/gstreamer-1.0/libgsthailotools.so
readonly OLD_TAPPAS_LIBS=(
    "libgsthailometa.so"
    "libhailo_tracker.so"
    "libhailo_cv_singleton.so"
    "libhailo_gst_image.so"
    "libgsthailotools.so"
    "libgsthailopython.so"
    "libgstsharktracers.so"
)
readonly OLD_TAPPAS_FILES=(
    "${LIB_DIR}/pkgconfig/gsthailometa.pc"
    "/usr/include/gsthailometa"
)
#############################################

function removed_msg(){
    echo "Hailo Tappas was removed successfully."
    echo "To unset Tappas environment variables, please relogin or reboot the PC."
    exit 0
}

function not_removed_msg(){
    echo "Hailo Tappas was not removed."
    exit 1
}

function check_tappas_installed(){
    rm -rf ${GSTREAMER_CACHE} && gst-inspect-1.0 hailotools &> /dev/null
}

function remove_so(){
    sudo rm -f ${LIB_DIR}/libhailo_*
    sudo rm -f ${LIB_DIR}/libgsthailometa.*
    sudo rm -rf ${LIB_DIR}/hailo/tappas
    sudo rm -f ${LIB_DIR}/libgstintercept*
    sudo rm -f ${LIB_DIR}/gstreamer-1.0/libgsthailotracers*
    sudo rm -f ${LIB_DIR}/gstreamer-1.0/libgsthailotools*
    sudo rm -f ${LIB_DIR}/gstreamer-1.0/libgstinstruments*
}

function remove_misc(){
    sudo rm -rf ${INSTALLATION_DIR}/include/hailo/tappas
    sudo rm -rf ${INSTALLATION_DIR}share/doc/hailo-tappas-core-${TAPPAS_VERSION}
    sudo rm -f ${LIB_DIR}/pkgconfig/hailo-tappas-core.pc
    sudo rm -f ${LIB_DIR}/pkgconfig/gsthailometa.pc
    sudo rm -f /etc/ld.so.preload.d/hailo_so.conf
}

function clean_tappas_user_env(){
    rm -rf ${TAPPAS_BASH_ENV}
}

function remove_tappas_venv(){
    pushd $(dirname $0) > /dev/null
    rm -rf ${VENV_NAME}
}

function remove_old_tappas_files(){
    # removing tappas file from system paths
    # relevant for Tappas pre-v3.24
    for file in ${OLD_TAPPAS_FILES[@]}
    do
        sudo rm -rf $file
    done

    for lib in ${OLD_TAPPAS_LIBS[@]}
    do
        sudo find ${LIB_DIR} -name "${lib}*" -delete
    done
}

function main(){
    if [ -f ${OLD_TAPPAS_INSTALLATION} ]; then
        echo "Warning: Older version of Hailo Tappas found - removing..."
        remove_old_tappas_files
    fi

    remove_so
    remove_misc
    remove_tappas_venv
    clean_tappas_user_env
    check_tappas_installed && return_code=$?
    if [[ $return_code == 0 ]]; then
        not_removed_msg
    else
        removed_msg
    fi
}

main
