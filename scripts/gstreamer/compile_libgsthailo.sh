#!/bin/bash
# Re-Compiles libgsthailo and then copy
set -e

BUILD_TYPE=Release
INSTALLATION_DIR=/usr


function print_usage() {
    echo "Compile libgsthailo:"
    echo ""
    echo "Options:"
    echo "  --help        Show this help"
    echo "  --build-type  Build mode --> Release/Debug"
    exit 1
}

function parse_args() {
    while test $# -gt 0; do
        if [[ "$1" == "-h" || "$1" == "--help" ]]; then
            print_usage
        elif [ "$1" == "--build-type" ]; then
            BUILD_TYPE=$2
            BUILD_TYPE=${BUILD_TYPE^}
            shift
        else
            echo "Unknown parameters, exiting"
            print_usage
        fi
        shift
    done
}

parse_args "$@"

if [[ ! -d $TAPPAS_WORKSPACE/hailort/sources ]]; then
    echo "Can't compile libgsthailo, sources not found, exiting"
    exit 1
fi

pushd $TAPPAS_WORKSPACE/hailort/sources/hailort/libhailort/bindings/gstreamer
rm -rf build

cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=$BUILD_TYPE
cmake --build build

sudo cp -a $TAPPAS_WORKSPACE/hailort/sources/hailort/libhailort/bindings/gstreamer/build/libgsthailo.so ${INSTALLATION_DIR}/lib/$(uname -m)-linux-gnu/gstreamer-1.0
rm -rf $TAPPAS_WORKSPACE/hailort/sources/hailort/libhailort/bindings/gstreamer/build/libgsthailo.so*
popd
