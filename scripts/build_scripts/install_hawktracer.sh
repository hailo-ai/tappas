#!/bin/bash
set -e 

pushd $TAPPAS_WORKSPACE/sources

git clone https://github.com/amzn/hawktracer.git
pushd hawktracer

mkdir build
pushd build
cmake ..
cmake --build . 
sudo cmake --build . --target install
sudo ldconfig

popd
popd
popd 

echo "Please visit the hello-world tutorial"
echo "https://www.hawktracer.org/doc/stable/tutorial_hello_world.html"
