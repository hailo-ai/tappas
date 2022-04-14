#!/bin/bash
set -e

BUILD_DIR=${TAPPAS_WORKSPACE}/core/hailo/gstreamer
POSTPROCESS_DIR=${TAPPAS_WORKSPACE}/apps/gstreamer/x86/libs
CROPPING_ALGORITHMS_DIR=${POSTPROCESS_DIR}/cropping_algorithms
BUILD_MODE=release
SKIP_HAILORT=false
COMPILE_LIBGSTHAILO=false
PYTHON_VERSION=3.6

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
        elif [ "$1" == "--skip-hailort" ]; then
            SKIP_HAILORT=true
        elif [ "$1" == "--compile-libgsthailo" ]; then
            COMPILE_LIBGSTHAILO=true
        else
            echo "Unknown parameters, exiting"
            print_usage
        fi
        shift
    done
}

function handle_libgsthailo() {
    # Compile from sources if the user selects to or the .deb has not placed it already there
    if [ "$COMPILE_LIBGSTHAILO" = true ] || [ ! -f /usr/lib/$(uname -p)-linux-gnu/gstreamer-1.0/libgsthailo.so ]; then
        ${TAPPAS_WORKSPACE}/scripts/gstreamer/compile_libgsthailo.sh --build-type $BUILD_MODE
    fi
}

function main() {
    if [ "$SKIP_HAILORT" = false ]; then
        handle_libgsthailo
    fi

    # Handle the build of gsthailotools
    pushd "$BUILD_DIR"
    CC=gcc-9 CXX=g++-9 meson build.$BUILD_MODE --buildtype $BUILD_MODE \
                            -Dlibargs="-I/usr/include/hailo/,-I/usr/include/gstreamer-1.0/gst/hailo/" \
                             -Dinclude_python=true -Dpython_version=$PYTHON_VERSION

    # Occupy all the cores could sometimes freeze the PC
    if [[ $(nproc) -le 4 ]]; then
	    num_cores_to_use=$(($(nproc)/2))
    else 
    	num_cores_to_use=$(($(nproc) - 1))
    fi

    if [[ -f "build.$BUILD_MODE/.ninja_log" ]]; then
        # Solve permission bug
        chown $(id -u):$(id -g) build.$BUILD_MODE/.ninja_log
    fi

    ninja -j $num_cores_to_use -C build.$BUILD_MODE
    sudo env "PATH=$PATH" ninja install -C build.$BUILD_MODE
    sudo cp -a build.$BUILD_MODE/plugins/libgsthailometa.so /usr/lib/$(uname -p)-linux-gnu/
    sudo cp -a build.$BUILD_MODE/plugins/libgsthailotools.so /usr/lib/$(uname -p)-linux-gnu/gstreamer-1.0/
    sudo cp -a build.$BUILD_MODE/plugins/python/libgsthailopython.so /usr/lib/$(uname -p)-linux-gnu/gstreamer-1.0/

    mkdir -p $POSTPROCESS_DIR
    cp build.$BUILD_MODE/libs/*.so $POSTPROCESS_DIR
    mkdir -p $CROPPING_ALGORITHMS_DIR
    cp build.$BUILD_MODE/libs/croppers/*.so $CROPPING_ALGORITHMS_DIR

    site_packages=$(pip show pip | grep Location | cut -d: -f2 | tr -d ' ')
    cp build.$BUILD_MODE/plugins/pyhailotracker.*.so "${site_packages}"
    cp build.$BUILD_MODE/plugins/python/libhailo.so ${site_packages}/hailo.so
    popd
}

parse_args "$@"
main
