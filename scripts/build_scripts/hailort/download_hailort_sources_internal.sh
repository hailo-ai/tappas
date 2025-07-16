#!/bin/bash
# Download the hailort sources internal (from Freenas)
set -e

SKIP_SOURCES="false"
HAILORT_RELEASE_FREENAS_PATH=''
HAILORT_FORMAL_RELEASE_VERSION=''

function print_usage() {
    echo "Install Hailo GStreamer:"
    echo ""
    echo "Options:"
    echo "  --help                           Show this help"
    echo "  --hailort-release-path           Freenas HailoRT release path (F.E releases/platform_releases/2022-07-05_18-43-04)"
    echo "  --hailort-formal-release-version HailoRT formal release verison (F.E 4.8.0)"
    echo "  --skip-sources                   Skip download sources"
    exit 1
}

function parse_args() {
    while test $# -gt 0; do
        if [[ "$1" == "-h" || "$1" == "--help" ]]; then
            print_usage
        elif [ "$1" == "--hailort-release-path" ]; then
            HAILORT_RELEASE_FREENAS_PATH=$2
            shift
        elif [ "$1" == "--hailort-formal-release-version" ]; then
            HAILORT_FORMAL_RELEASE_VERSION=$2
            shift
        elif [ "$1" == "--skip-sources" ]; then
            SKIP_SOURCES="true"
        else
            echo "Unknown parameters, exiting"
            print_usage
        fi
        shift
    done
}

function non_formal_artifacts() {
    rsync -arv --progress hailo@192.168.12.21:${HAILORT_RELEASE_FREENAS_PATH}/release/hailort_*_$(dpkg --print-architecture).deb hailort/
    rsync -arv --progress hailo@192.168.12.21:${HAILORT_RELEASE_FREENAS_PATH}/release/hailort-pcie-driver_*_all.deb hailort/

    if [[ $SKIP_SOURCES == "false" ]]; then
        rsync -arv --progress hailo@192.168.12.21:${HAILORT_RELEASE_FREENAS_PATH}/sources/hailort/* hailort/sources
    fi
}

function formal_artifacts() {
    release_path="~/releases/platform_formal_releases/${HAILORT_FORMAL_RELEASE_VERSION}/Hailort/Linux/Installer"
    rsync -arv --progress hailo@192.168.12.21:${release_path}/hailort_*_$(dpkg --print-architecture).deb hailort/
    rsync -arv --progress hailo@192.168.12.21:${release_path}/hailort-pcie-driver_*_all.deb hailort/

    if [[ $SKIP_SOURCES == "false" ]]; then
        git clone https://github.com/hailo-ai/hailort.git -b "v${HAILORT_FORMAL_RELEASE_VERSION}" --depth 1 hailort/sources
    fi
}

parse_args "$@"

mkdir -p hailort

if [[ ! -z "$HAILORT_RELEASE_FREENAS_PATH" ]]; then
    non_formal_artifacts
elif [[ ! -z "$HAILORT_FORMAL_RELEASE_VERSION" ]]; then
    formal_artifacts
fi



