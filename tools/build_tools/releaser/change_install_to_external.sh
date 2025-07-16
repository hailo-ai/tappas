#!/bin/bash
set -e

readonly SCRIPT_DIR=$(dirname $(which $0))

rm -f $SCRIPT_DIR/../../../downloader/internal_main.py
sed -i 's#downloader/internal_main.py#downloader/main.py#g' $SCRIPT_DIR/../../../install.sh
sed -i 's#python3 downloader/internal_main.py#python3 downloader/main.py#g' $SCRIPT_DIR/../../../docker/Dockerfile.tappas

python3 $SCRIPT_DIR/change_install_to_external.py