#!/bin/bash

# Hailo Tappas uninstall script
# requires `sudo` privileges

set -e

#############################################
readonly GSTREAMER_DIR=core/hailo/gstreamer
readonly SHARK_DIR=sources/gst-shark/plugins
readonly PIP_REQUIREMENTS=core/requirements/requirements.txt
VENV_NAME=hailo_tappas_venv
#############################################

function not_installed_msg(){
    echo "Hailo Tappas does not appear to be installed..."
}

function check_tappas_installed(){
    rm -rf ~/.cache/gstreamer-1.0/registry.x86_64.bin && gst-inspect-1.0 hailotools &> /dev/null
}

function uninstall_gst(){
    pushd $GSTREAMER_DIR
    sudo env "PATH=$PATH" ninja uninstall -C build.release
    popd
}

function uninstall_shark(){
    if [[ -d $SHARK_DIR ]]; then
        pushd $SHARK_DIR
        sudo env "PATH=$PATH" ninja uninstall -C build
        popd
    fi
}

function create_venv(){
    if [[ ! -d $VENV_NAME ]]; then
        VENV_NAME=tmp_venv
        python3 -m virtualenv $VENV_NAME
    fi
}

function activate_venv(){
    source "$VENV_NAME/bin/activate"
}

function remove_venv(){
    rm -rf $VENV_NAME
}

function install_env_requirements(){
    pip install -r $PIP_REQUIREMENTS
}

function uninstall_so(){
    create_venv
    activate_venv
    install_env_requirements
    uninstall_gst
    uninstall_shark
    deactivate
    remove_venv
}

function remove_bash_autocompletion(){
    if [[ ! -z $HOME && -f $HOME/.bashrc ]]; then
        local tappas_bash_comp_line=$(grep -n tappas $HOME/.bashrc | grep bash_completion | cut -d':' -f1)
        sed -i "${tappas_bash_comp_line}d" $HOME/.bashrc
    fi
}

function main(){
    check_tappas_installed && return_code=$?
    if [[ $return_code == 0 ]]; then
        uninstall_so
        remove_bash_autocompletion
    else
        not_installed_msg
    fi
}

main
