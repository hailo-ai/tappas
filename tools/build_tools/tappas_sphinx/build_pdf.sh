#!/bin/bash
set -e

CURRENT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

readonly VENV_NAME="${CURRENT_DIR}/tappas_venv"
readonly STABLE_PIP="pip==20.3.3"
readonly ENV_RQUIREMETNS_FILE="${CURRENT_DIR}/env_requirements.txt"
readonly INSTALL_LOG="${CURRENT_DIR}/env_setup.log"
readonly INSTALL_DOC_PY="${CURRENT_DIR}/run_tappas_doc.py"

PY_VERSION="python3"
echo $PY_VERSION


function prepare_log_file(){    
    if [ -f $INSTALL_LOG ]
        then
           > $INSTALL_LOG
    fi
}

function handle_venv(){
    if [ ! -z $VIRTUAL_ENV ]
    then
        echo "Running inside activated virtual environment $VIRTUAL_ENV"
        install_env_requirements
    elif [ -d $VENV_NAME ]
    then
        echo "Using existing virtual environment <$VENV_NAME> "
        activate_venv
    else
        echo "<$VENV_NAME> doesn't exist, will create it"
        create_venv
        activate_venv
        install_env_requirements
    fi
}

function create_venv(){
    $PY_VERSION -m virtualenv $VENV_NAME >> $INSTALL_LOG 2>&1
}

function activate_venv(){
    echo "Activating virtualenv <$VENV_NAME>"
    source "$VENV_NAME/bin/activate"
}

function run_python_installer(){
    $INSTALL_DOC_PY
}

function install_env_requirements(){
    echo "Installing env requirements from $ENV_RQUIREMETNS_FILE"
    pip install -r $ENV_RQUIREMETNS_FILE >> $INSTALL_LOG 2>&1
}

function main(){
    echo "build_pdf.sh script executed"
    prepare_log_file
    handle_venv
    run_python_installer 
}

main ""