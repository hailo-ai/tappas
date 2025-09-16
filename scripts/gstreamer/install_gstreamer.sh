#!/bin/bash
set -e

ARCH_DIR=""
INSTALLATION_DIR=/usr

# Extract GStramer version used
GSTREAMER_VERSION=$(gst-launch-1.0 --gst-version | awk '{print $NF}' | cut -d. -f1,2)
num_cores_to_use=$(($(nproc)/2))
cross_compile_command=""

function install_plugins_good() {
  if [[ ! $GSTREAMER_VERSION == @(1.14|1.16) ]]; then 
  	return 0
  fi
	
  pushd ${TAPPAS_WORKSPACE}/sources/
  git clone --depth 1 -b ${GSTREAMER_VERSION} https://github.com/GStreamer/gst-plugins-good.git
  pushd gst-plugins-good

  # Patch rtsp-plugins-good
  git apply ${TAPPAS_WORKSPACE}/core/patches/rtsp/rtspsrc_stream_id_path.patch

  # Build plugins-good
  meson build ${cross_compile_command} -Dlibdir=/usr/lib/${ARCH_DIR} --prefix ${INSTALLATION_DIR}
  ninja -v -j $num_cores_to_use -C build
  sudo env "PATH=$PATH" ninja -j $num_cores_to_use -C build install
  popd
  popd

}

function install_gst_instruments() {
  # Build debug plugins
  pushd ${TAPPAS_WORKSPACE}/sources/
  git clone --depth 1 -b 0.3.1 https://github.com/kirushyk/gst-instruments.git
  pushd gst-instruments
  meson build ${cross_compile_command} -Dui=disabled -Dlibdir=/usr/lib/${ARCH_DIR} --prefix ${INSTALLATION_DIR}
  ninja -v -j $num_cores_to_use -C build
  sudo env "PATH=$PATH" ninja -j $num_cores_to_use -C build install
  popd
  popd

}

function parse_args() {
  while test $# -gt 0; do
    if [ "$1" = "--target-platform" ]; then
      case $2 in
        x86|x86_64)
          ARCH_DIR="x86_64-linux-gnu"
          ;;
        rpi5|rockchip|aarch64|rpi)
          ARCH_DIR="aarch64-linux-gnu"
          ;;
        imx8)
          ARCH_DIR=""
          ;;
        *)
          echo "Unknown target platform: $ARCH"
          print_usage
          ;;
      esac
      shift
    elif [ "$1" = "--cross-file" ]; then
      cross_compile_command="$1 $2"
      shift
    else
      echo "Unknown parameters, exiting"
      exit 1
    fi
    shift
  done
}

parse_args "$@"


install_plugins_good

# gst_instruments is optional 
# We dont want to fail our install in case it fails
install_gst_instruments || true
