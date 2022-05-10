#!/bin/bash
# Re-Compiles libgsthailo and then copy
set -e

BUILD_TYPE=Release


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

pushd $TAPPAS_WORKSPACE/hailort/sources/hailort/
rm -rf build

cmake -H. -Bbuild -DHAILO_BUILD_GSTREAMER=1 -DCMAKE_BUILD_TYPE=$BUILD_TYPE
cmake --build build

sudo cp -a $TAPPAS_WORKSPACE/hailort/sources/hailort/build/hailort/libhailort/bindings/gstreamer/libgsthailo.so /usr/lib/$(uname -p)-linux-gnu/gstreamer-1.0
rm -rf $TAPPAS_WORKSPACE/hailort/sources/hailort/build/hailort/libhailort/src/libhailort.so*
popd
