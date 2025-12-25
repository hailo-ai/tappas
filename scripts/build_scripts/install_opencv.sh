#!/bin/bash
set -e

BUILD_FROM_SOURCE=false

function print_usage() {
    echo "Install OpenCV:"
    echo ""
    echo "Options:"
    echo "  --help    Show this help"
    echo "  --build   Build OpenCV from source (static libraries with IPP disabled)"
    echo ""
    echo "Without --build flag, installs OpenCV from apt packages."
    exit 1
}

function parse_args() {
    while test $# -gt 0; do
        if [[ "$1" == "-h" || "$1" == "--help" ]]; then
            print_usage
        elif [ "$1" == "--build" ]; then
            BUILD_FROM_SOURCE=true
        else
            echo "Unknown parameter: $1"
            print_usage
        fi
        shift
    done
}

function prepare_opencv() {
    rm -f 4.5.2.zip
    wget https://github.com/opencv/opencv/archive/4.5.2.zip
    unzip 4.5.2.zip
    rm 4.5.2.zip
    pushd opencv-4.5.2
    mkdir build
    pushd build
}

function compile() {
    cmake -DOPENCV_GENERATE_PKGCONFIG=ON \
        -DBUILD_LIST=core,imgproc,imgcodecs,calib3d, \
        -DCMAKE_BUILD_TYPE=RELEASE \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
        -DCMAKE_C_FLAGS="-ffunction-sections -fdata-sections" \
        -DCMAKE_CXX_FLAGS="-ffunction-sections -fdata-sections" \
        -DWITH_IPP=OFF \
        -DWITH_ITT=OFF \
        -DBUILD_ZLIB=ON -DZLIB_FOUND=FALSE \
        -DBUILD_JPEG=ON -DJPEG_FOUND=FALSE \
        -DBUILD_TIFF=ON -DTIFF_FOUND=FALSE \
        -DBUILD_PNG=ON -DPNG_FOUND=FALSE \
        -DBUILD_JASPER=ON -DJASPER_FOUND=FALSE \
        -DWITH_PROTOBUF=OFF -DWITH_QUIRC=OFF \
        -DWITH_WEBP=OFF -DWITH_OPENJPEG=OFF \
        -DWITH_GSTREAMER=OFF -DWITH_GTK=OFF \
        -DWITH_TBB=OFF -DWITH_OPENGL=OFF \
        -DOPENCV_DNN_OPENCL=OFF -DBUILD_opencv_python2=OFF \
        -DWITH_TEGRA=OFF -DENABLE_NEON=OFF \
        -DWITH_OPENEXR=ON -DBUILD_OPENEXR=ON \
        -DINSTALL_C_EXAMPLES=OFF \
        -DINSTALL_PYTHON_EXAMPLES=OFF \
        -DOPENCV_INSTALL_PATH=/usr/local ..

    num_cores_to_use=$(($(nproc) / 2))
    make -j$num_cores_to_use
    sudo make install
    sudo ldconfig
}

function clean_environment() {
    popd
    popd
    rm -rf "opencv-4.5.2"
}

function main_from_source() {
    sudo apt-get install -y unzip
    mkdir -p sources
    pushd sources
    prepare_opencv
    compile
    clean_environment
    popd
}

function main() {
    sudo apt-get install -y libopencv-dev python3-opencv
}

parse_args "$@"

if [ "$BUILD_FROM_SOURCE" = true ]; then
    main_from_source
else
    main
fi
