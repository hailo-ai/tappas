#!/bin/bash
set -e

skip_hailort=false
PYTHON_VERSION=3.6

function print_usage() {
  echo "TAPPAS Install:"
  echo ""
  echo "Options:"
  echo "  --help                 Show this help"
  echo "  --skip-hailort         Path to the GStreamer build dir"
  exit 1
}

function parse_args() {
  while test $# -gt 0; do
    if [[ "$1" == "-h" || "$1" == "--help" ]]; then
      print_usage
    elif [ "$1" == "--skip-hailort" ]; then
      skip_hailort=true
    else
      echo "Unknown parameters, exiting"
      print_usage
    fi
    shift
  done
}

function install_apt_packages() {
  sudo apt-get update &&
    $TAPPAS_WORKSPACE/scripts/build_scripts/install_pkg_file.sh $TAPPAS_WORKSPACE/core/requirements/basic_plugins.pkg

  lsb_release=$(lsb_release -r | awk '{print $2}')

  # Ubuntu20 support
  if [ $lsb_release = "20.04" ]; then
    PYTHON_VERSION=3.8
    sudo add-apt-repository -y ppa:kisak/kisak-mesa &&
      sudo apt-get dist-upgrade -y
  fi

  # Install apt packages
  sudo add-apt-repository -y ppa:oibaf/graphics-drivers &&
    sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test &&
    sudo apt-get update &&
    $TAPPAS_WORKSPACE/scripts/build_scripts/install_pkg_file.sh $TAPPAS_WORKSPACE/core/requirements/gstreamer_plugins.pkg
}

function python_venv_create_and_install() {
  VENV_NAME='hailo_tappas_venv'
  VENV_PATH="$(pwd)"

  # Install python specific version if not found
  if ! [ -x "$(command -v python$PYTHON_VERSION)" ]; then
    echo "Python$PYTHON_VERSION not found. Installing"
    sudo add-apt-repository -y ppa:deadsnakes/ppa
    sudo apt update
    sudo apt-get -y --no-install-recommends install python$PYTHON_VERSION
  fi

  # Create new venv or skip
  if [ ! -z $VIRTUAL_ENV ]; then
    echo "Installing into active virtualenv: $VIRTUAL_ENV"
  else
    # Delete old venv
    rm -rf ${VENV_PATH}/$VENV_NAME

    echo "Creating new virtualenv ($VENV_NAME) in ($VENV_PATH) and installing into it"
    virtualenv -p python$PYTHON_VERSION $VENV_PATH/$VENV_NAME
    source ${VENV_PATH}/$VENV_NAME/bin/activate
  fi

  # Install pip packages & Call the downloader script
  pip3 install --upgrade pip setuptools
  pip3 install -r $TAPPAS_WORKSPACE/core/requirements/requirements.txt
  pip3 install -r $TAPPAS_WORKSPACE/core/requirements/gstreamer_requirements.txt
  pip3 install -r $TAPPAS_WORKSPACE/downloader/requirements.txt
  python3 $TAPPAS_WORKSPACE/downloader/main.py
}

function clone_external_sources() {
  # Clone required packages
  rm -rf ${TAPPAS_WORKSPACE}/sources
  mkdir -p ${TAPPAS_WORKSPACE}/sources
  pushd ${TAPPAS_WORKSPACE}/sources
  git clone --depth 1 --shallow-submodules -b 0.24.0 https://github.com/xtensor-stack/xtensor.git
  git clone --depth 1 --shallow-submodules -b 0.20.0 https://github.com/xtensor-stack/xtensor-blas.git
  git clone --depth 1 --shallow-submodules -b 0.7.3 https://github.com/xtensor-stack/xtl.git
  git clone --depth 1 --shallow-submodules -b v3.0.0 https://github.com/jarro2783/cxxopts.git
  git clone --depth 1 --shallow-submodules -b v2.9.0 https://github.com/pybind/pybind11.git
  mkdir -p ${TAPPAS_WORKSPACE}/core/open_source/xtensor_stack/base
  mkdir -p ${TAPPAS_WORKSPACE}/core/open_source/xtensor_stack/blas
  mkdir -p ${TAPPAS_WORKSPACE}/core/open_source/cxxopts
  mkdir -p ${TAPPAS_WORKSPACE}/core/open_source/pybind11
  cp -r xtensor/include/. ${TAPPAS_WORKSPACE}/core/open_source/xtensor_stack/base
  cp -r xtensor-blas/include/. ${TAPPAS_WORKSPACE}/core/open_source/xtensor_stack/blas
  cp -r xtl/include/. ${TAPPAS_WORKSPACE}/core/open_source/xtensor_stack/base
  cp -r cxxopts/include/. ${TAPPAS_WORKSPACE}/core/open_source/cxxopts
  cp -r pybind11/include/. ${TAPPAS_WORKSPACE}/core/open_source/pybind11
  popd

  }

function install_gstreamer() {
  # Clear any existing GStreamer cache
  rm -rf ~/.cache/gstreamer-1.0/

  # Copy the patches and then compile and install gstreamer plugins that requires patch or lack of apt install
  $TAPPAS_WORKSPACE/scripts/gstreamer/install_gstreamer.sh

  # Copy and append core features
  ls -l ${TAPPAS_WORKSPACE}/core/open_source/opencv/lib/*.so.* | grep -v ^l | awk '{print $NF}' | sudo xargs chmod 644
  sudo cp -a ${TAPPAS_WORKSPACE}/core/open_source/opencv/lib/* /usr/lib/$(uname -p)-linux-gnu
  sudo cp -a ${TAPPAS_WORKSPACE}/core/open_source/opencv/include/* /usr/include/$(uname -p)-linux-gnu
  mkdir ${TAPPAS_WORKSPACE}/tmp
  cp ${TAPPAS_WORKSPACE}/core/open_source/opencv/opencv4.pc ${TAPPAS_WORKSPACE}/tmp/opencv4.pc
  sed -i "s/<ARCH>/$(uname -p)/g" ${TAPPAS_WORKSPACE}/tmp/opencv4.pc
  sudo cp -a ${TAPPAS_WORKSPACE}/tmp/opencv4.pc /usr/lib/$(uname -p)-linux-gnu/pkgconfig
  # Compile and install Hailo Gstreamer
  GSTREAMER_BUILD_DIR=${TAPPAS_WORKSPACE}/core/hailo/gstreamer
  GST_HAILO_BUILD_MODE=release
  ${TAPPAS_WORKSPACE}/scripts/gstreamer/install_hailo_gstreamer.sh --build-dir $GSTREAMER_BUILD_DIR --build-mode $GST_HAILO_BUILD_MODE --python-version $PYTHON_VERSION
}

function install_hailo() {
  if [ "$skip_hailort" = false ]; then
    sudo dpkg -i ${TAPPAS_WORKSPACE}/hailort/hailort_*_$(dpkg --print-architecture).deb
  fi
}

function main() {

  if [[ -z "$TAPPAS_WORKSPACE" ]]; then
    export TAPPAS_WORKSPACE=$(dirname "$(realpath "$0")")
    echo "No TAPPAS_WORKSPACE in environment found, using the default one $TAPPAS_WORKSPACE"
  fi

  install_apt_packages
  python_venv_create_and_install
  clone_external_sources
  install_hailo
  install_gstreamer
}

parse_args "$@"
main "$@"
