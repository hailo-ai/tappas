#!/bin/bash
# This shell script calls make in given driver directory using yocto's toolchain environment
# that contains the required cross compiler and attributes (kernel directory) for a specific platform.
set -e

cmake=false
build_dir=""
build_type="Debug"
env_setup_path=""
make_target=""
compile_from=""
hailort_include_dir=""
hailort_lib=""

function print_usage() {
    echo "usage:"
    echo ""
    echo "Options:"
    echo "  --help               Show this help"
    echo "  --cmake              Run cmake command instead of make"
    echo "  --env-setup-path     Setting toolchain environment setup path"
    echo "  --build-directory    Setting build directory"
    echo "  --build-type         Setting build type"
    echo " --make-target         Setting make target"
    echo " --hailort-include-dir Setting hailort include dir"
    echo " --hailort-lib         Setting hailortlib"
    echo " --compile-from        Setting directory to run the compilation from"
}

function parse_args() {
    while test $# -gt 0; do
        if [ "$1" = "--help" ] || [ "$1" == "-h" ]; then
            print_usage; exit 0;
        elif [ "$1" = "--cmake" ]; then
            echo "Run cmake command instead of make"
            cmake=true
        elif [ "$1" = "--env-setup-path" ]; then
            shift
            echo "Setting toolchain environment setup path to $1"
            env_setup_path=$1
        elif [ "$1" = "--build-directory" ]; then
            shift
            echo "Setting build directory to $1"
            build_dir=$1
        elif [ "$1" = "--build-type" ]; then
            shift
            echo "Setting build type to $1"
            build_type=$1
        elif [ "$1" = "--hailort-include-dir" ]; then
            shift
            echo "Setting hailort include dir to $1"
            hailort_include_dir=$1
        elif [ "$1" = "--hailort-lib" ]; then
            shift
            echo "Setting hailort lib to $1"
            hailort_lib=$1
        elif [ "$1" = "--make-target" ]; then
            shift
            echo "Setting make target to $1"
            make_target=$1
        elif [ "$1" = "--compile-from" ]; then
            shift
            echo "Setting directory to run the compilation from $1"
            compile_from=$1
        else
            echo "Received invalid argument: $1. See expected arguments below:"
            print_usage
            exit 1
        fi
        shift
    done
}

function main(){
    parse_args $@

    # clear global ld library paths to prevent compilation errors
    unset LD_LIBRARY_PATH

    # enter the driver directory
    cd $compile_from

    # source the toolchain environment
    source $env_setup_path
    export LDFLAGS=
    
    echo " using cross compiler: ! $(which $CC) "
    # invoke make all KERNEL_DIR=<kernel_dir>
    compiler=$(echo $CC | cut -d' ' -f1)
    cxx_compiler=$(echo $CXX | cut -d' ' -f1)

    if [ "$cmake" = true ]; then
        cmake . -B"$build_dir" -DCMAKE_SYSROOT="$PKG_CONFIG_SYSROOT_DIR" -DCMAKE_LINKER="$LD" \
          -DCMAKE_C_COMPILER="$compiler" -DCMAKE_CXX_COMPILER="$cxx_compiler" -DCMAKE_BUILD_TYPE="$build_type" \
          -DHAILORT_INCLUDE_DIR="$hailort_include_dir" -DHAILORT_LIB="$hailort_lib" -DCMAKE_LIBRARY_ARCHITECTURE="aarch64-linux-gnu"
    else
        make $make_target CC="$compiler" CXX="$cxx_compiler" LD="$LD" STRIP="$STRIP" \
         LD_LIBRARY_PATH="$LD_LIBRARY_PATH" --directory "$build_dir" --no-print-directory
    fi

}

main $@

