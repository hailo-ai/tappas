#!/bin/bash
set -e

cross_compile_command=""
script_dir=$(dirname $(realpath "$0"))
source $script_dir/../misc/checks_before_run.sh --export-only

BUILD_DIR="$TAPPAS_WORKSPACE/core/hailo"
BUILD_MODE=release
SKIP_HAILORT=false
COMPILE_LIBGSTHAILO=false
INCLUDE_UNIT_TESTS=true
PYTHON_VERSION=$(python3 --version | awk '{print $2}' | awk -F'.' '{print $1"."$2}')
TARGET="all"
TARGET_PLATFORM="x86"
GCC_VERSION="12"
INSTALLATION_DIR=/usr

# Occupy all the cores could sometimes freeze the PC
num_cores_to_use=$(($(nproc)/2))

function print_usage() {
    echo "Install Hailo GStreamer:"
    echo ""
    echo "Options:"
    echo "  --help                 Show this help"
    echo "  --build-dir            Path to the GStreamer build dir"
    echo "  --build-mode           Build mode, debug/release, default is $BUILD_MODE"
    echo "  --skip-hailort         Skip compiling HailoRT"
    echo "  --python-version       Python version"
    echo "  --compile-libgsthailo  Compile libgsthailo instead of copying it from the release"
    echo "  --skip-unit-tests      Skip compilation of unit tests"
    echo "  --target               Tappas build target [all, plugins, libs, apps] (default = all)"
    echo "  --target-platform      Target platform, used for installing only required media and hef files [x86, arm, hailo15] (Default is $TARGET_PLATFORM)"
    echo "  --gcc-version          GCC version to use (Default is $GCC_VERSION)"
    echo "  --compile-num-of-cores Number of cpu cores to compile with (more cores makes the compilation process faster, but may cause 'out of swap memory' issue on weak machines)"
    exit 1
}

function parse_args() {
    while test $# -gt 0; do
        if [[ "$1" == "-h" || "$1" == "--help" ]]; then
            print_usage
        elif [ "$1" == "--build-dir" ]; then
            BUILD_DIR=$2
            shift
        elif [ "$1" == "--build-mode" ]; then
            BUILD_MODE=$2
            shift
        elif [ "$1" == "--python-version" ]; then
            PYTHON_VERSION=$2
            shift
        elif [ "$1" == "--target" ]; then
            TARGET=$2
            shift
        elif [ "$1" == "--target-platform" ]; then
            TARGET_PLATFORM=$2
            shift
        elif [ "$1" == "--gcc-version" ]; then
            GCC_VERSION=$2
            shift
        elif [ "$1" == "--skip-hailort" ]; then
            SKIP_HAILORT=true
        elif [ "$1" == "--skip-unit-tests" ]; then
            INCLUDE_UNIT_TESTS=false
        elif [ "$1" == "--compile-num-of-cores" ]; then
            num_cores_to_use=$2
            shift
        elif [ "$1" == "--compile-libgsthailo" ]; then
            COMPILE_LIBGSTHAILO=true
        elif [ "$1" = "--cross-file" ]; then
            cross_compile_command="$1 $2"
            shift
        else
            echo "Unknown parameters, exiting"
            print_usage
        fi
        shift
    done
}

function main() {
    # Handle the build of gsthailotools
    pushd "$BUILD_DIR"

    reconfigure_flag=
    if [ -d "./build.$BUILD_MODE" ]
    then
        reconfigure_flag=--reconfigure
    fi


    echo "Compiling Hailo Gstreamer target $TARGET, with $num_cores_to_use cpu cores, build type $BUILD_MODE $reconfigure_flag"

    echo "CC=gcc-$GCC_VERSION CXX=g++-$GCC_VERSION meson build.$BUILD_MODE $reconfigure_flag --prefix '${INSTALLATION_DIR}' --buildtype $BUILD_MODE \
                            ${cross_compile_command} \
                            -Dtarget=$TARGET \
                            -Dtarget_platform=$TARGET_PLATFORM \
                            -Dlibargs='-I/usr/include/hailo/,-I/usr/include/gstreamer-1.0/gst/hailo/' \
                             -Dinclude_python=true -Dpython_version=$PYTHON_VERSION"

    CC=gcc-$GCC_VERSION CXX=g++-$GCC_VERSION meson build.$BUILD_MODE $reconfigure_flag --prefix "${INSTALLATION_DIR}" --buildtype $BUILD_MODE \
                            ${cross_compile_command} \
                            -Dtarget=$TARGET \
                            -Dtarget_platform=$TARGET_PLATFORM \
                            -Dlibargs="-I/usr/include/hailo/,-I/usr/include/gstreamer-1.0/gst/hailo/" \
                             -Dinclude_python=true -Dpython_version=$PYTHON_VERSION

    if [[ -f "build.$BUILD_MODE/.ninja_log" ]]; then
        # Solve permission bug
        chown $(id -u):$(id -g) build.$BUILD_MODE/.ninja_log
    fi

    ninja -j $num_cores_to_use -C build.$BUILD_MODE
    sudo env "PATH=$PATH" ninja install -C build.$BUILD_MODE

    popd
}

parse_args "$@"
main
