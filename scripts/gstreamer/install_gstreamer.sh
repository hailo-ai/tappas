#!/bin/bash
set -e

# Extract GStramer version used
GSTREAMER_VERSION=$(gst-launch-1.0 --gst-version | awk '{print $NF}' | cut -d. -f1,2)
num_cores_to_use=$(($(nproc) - 1))

function install_plugins_good() {
  pushd ${TAPPAS_WORKSPACE}/sources/
  git clone --depth 1 -b ${GSTREAMER_VERSION} https://github.com/GStreamer/gst-plugins-good.git
  pushd gst-plugins-good

  # Patch rtsp-plugins-good
  git apply ${TAPPAS_WORKSPACE}/core/patches/rtsp/rtspsrc_stream_id_path.patch

  # Build plugins-good
  meson build --prefix /usr/
  ninja -j $num_cores_to_use -C build
  sudo env "PATH=$PATH" ninja -j $num_cores_to_use -C build install
  popd
  popd

}

function install_gst_instruments() {
  # Build debug plugins
  pushd ${TAPPAS_WORKSPACE}/sources/
  git clone --depth 1 -b 0.3.1 https://github.com/kirushyk/gst-instruments.git
  pushd gst-instruments
  meson build -Dui=disabled --prefix /usr/
  ninja -j $num_cores_to_use -C build
  sudo env "PATH=$PATH" ninja -j $num_cores_to_use -C build install
  popd
  popd

}

function install_gst_shark() {
  # Build GstShark
  pushd ${TAPPAS_WORKSPACE}/sources/
  git clone --depth 1 --shallow-submodules -b v0.7.2 https://github.com/RidgeRun/gst-shark/
  pushd gst-shark
  ./autogen.sh --prefix /usr/ --libdir /usr/lib/$(uname -p)-linux-gnu/
  sudo make install
  popd
  popd

}

install_plugins_good
install_gst_instruments
install_gst_shark || true  # We dont want to fail our install in cases where gst-shark fails
