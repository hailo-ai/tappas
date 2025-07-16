#!/bin/bash

readonly VENV_NAME=hailo_tappas_release_env
readonly SCRIPT_PATH=$(dirname `which $0`)


virtualenv -p python3.8 $VENV_NAME
source $VENV_NAME/bin/activate
pip3 install pyyaml==6.0 pydantic==2.0.2

$SCRIPT_PATH/create_release.py $@
exit $?
