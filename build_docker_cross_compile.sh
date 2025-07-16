#!/bin/bash
# Build docker must be in the parent directory. Otherwise copying the whole directory tree is not allowed
set -e

readonly platform_dir="release"
readonly dockerfile_dir="docker"
readonly dockerfile_tappas_combined="Dockerfile.tappas_combined"

tag=latest
flags=""
gst_hailo_build_mode="release"
platform=""
target_platform="x86"
version=""
docker_builder_name=""

function print_usage() {
    echo "Run Hailo Docker:"
    echo ""
    echo "Options:"
    echo "  --help                   Show this help"
    echo "  --no-cache               Build Docker without cache"
    echo "  --tag                    The selected tag, default is - $tag"
    echo "  --platform               Build Docker for the pass platform"
    echo "  --target-platform        Target platform [x86, arm, rpi(raspberry pi)], used for downloading only required media and hef files (Default is $target_platform)"
    echo "  --version                The release version used for naming the docker tarball"
    exit 1
}

function parse_args() {
    while test $# -gt 0; do
        if [[ "$1" == "-h" || "$1" == "--help" ]]; then
            print_usage
        elif [ "$1" == "--no-cache" ]; then
            flags="--no-cache"
        elif [ "$1" == "--platform" ]; then
            platform=$2
            shift
        elif [ "$1" == "--target-platform" ]; then
            target_platform=$2
            shift
        elif [ "$1" == "--version" ]; then
            version=$2
            shift
        elif [ "$1" == "--tag" ]; then
            tag=$2
            shift
        else
            echo "Unknown parameters, exiting"
            print_usage
        fi
        shift
    done
}

parse_args "$@"
sudo docker run --rm --privileged multiarch/qemu-user-static --reset -p yes --credential yes
docker_builder_name= docker buildx create --driver docker-container --use
docker buildx build --platform=$platform $flags -f ${dockerfile_dir}/${dockerfile_tappas_combined} -t hailo_tappas:"$tag" \
    -o type=docker,dest=./hailo-docker-tappas-v${version}.tar \
    --build-arg skip_headers_install="$skip_headers_install" \
    --build-arg target_platform="$target_platform" \
    --build-arg gst_hailo_build_mode="$gst_hailo_build_mode" .
docker buildx stop ${docker_builder_name}
docker buildx rm ${docker_builder_name}
