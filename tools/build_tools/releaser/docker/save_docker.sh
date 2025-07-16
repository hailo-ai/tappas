#!/bin/bash
set -e


VERSION="v3.14"
TAPPAS_WORKSPACE="$(dirname "$0")/../../../.."

function print_usage() {
    echo "Run Hailo Docker:"
    echo ""
    echo "Options:"
    echo "  --help      Show this help"
    echo "  --version   The selected version, default is - $tag"
    exit 1
}

function parse_args() {
    while test $# -gt 0; do
        if [[ "$1" == "-h" || "$1" == "--help" ]]; then
            print_usage
        elif [ "$1" == "--tag" ]; then
            VERSION=$2
            shift
        else
            echo "Unknown parameters, exiting"
            print_usage
        fi
        shift
    done
}

parse_args "$@"

# NOTE: Repalce to build_docker before the release
$TAPPAS_WORKSPACE/build_docker_internal.sh --tag $VERSION

docker save -o hailo-docker-tappas-${VERSION}.tar.gz hailo_tappas:${VERSION}
