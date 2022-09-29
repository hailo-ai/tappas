#!/bin/bash
set -e

num_cores_to_use=$(($(nproc)/2))

function install_gst_shark() {
  # Build GstShark
  pushd ${TAPPAS_WORKSPACE}/sources/
  git clone --recurse-submodules -b tappas https://github.com/HailoRT-Automation/gst-shark.git
  pushd gst-shark/plugins
  meson build --prefix /usr/
  ninja -C build
  sudo env "PATH=$PATH" ninja -j $num_cores_to_use -C build install
  popd
  popd
}


# gst_shark id optional 
# We dont want to fail our install in case it fails
install_gst_shark || true
