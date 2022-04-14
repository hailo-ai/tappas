#!/bin/bash
# Add HailoRT to venv
set -e

CURRENT=$(cat /etc/sudoers | grep secure_path | awk '{print $2}' | awk -F'=' '{print $2}' |  cut -c2-)
NEW=\"${WORKSPACE}/${PLATFORM_VENV}/bin:$CURRENT
sed -i "s@secure_path=.*@secure_path=${NEW}@g" /etc/sudoers
