#!/bin/bash
set -e

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
        -DBUILD_LIST=core,imgproc,imgcodecs,calib3d,features2d,flann \
        -DCMAKE_BUILD_TYPE=RELEASE \
        -DWITH_PROTOBUF=OFF -DWITH_QUIRC=OFF \
        -DWITH_WEBP=OFF -DWITH_OPENJPEG=OFF \
        -DWITH_GSTREAMER=OFF -DWITH_GTK=OFF \
        -DOPENCV_DNN_OPENCL=OFF -DBUILD_opencv_python2=OFF \
        -DINSTALL_C_EXAMPLES=ON \
        -DINSTALL_PYTHON_EXAMPLES=ON \
        -DCMAKE_INSTALL_PREFIX=/usr/local ..

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

    pushd $TAPPAS_WORKSPACE/sources
    prepare_opencv
    compile
    clean_environment
    popd
}

function main() {
    sudo apt-get install -y unzip
    sudo apt-get install -y libopencv-dev python3-opencv
}

main