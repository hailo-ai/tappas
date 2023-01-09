#!/bin/bash
set -e

skip_hailort=false
target_platform="x86"
compile_num_cores=""

if [[ -z "$TAPPAS_WORKSPACE" ]]; then
  export TAPPAS_WORKSPACE=$(dirname "$(realpath "$0")")
  echo "No TAPPAS_WORKSPACE in environment found, using the default one $TAPPAS_WORKSPACE"
fi

readonly GST_HAILO_BUILD_MODE='release'
readonly VENV_NAME='hailo_tappas_venv'
readonly VENV_PATH="$(pwd)"

function print_usage() {
  echo "TAPPAS Install:"
  echo ""
  echo "Options:"
  echo "  --help                 Show this help"
  echo "  --skip-hailort         Skips installation of HailoRT Deb package"
  echo "  --target-platform      Target platform [x86, rpi(raspberry pi)], used for downloading only required media and hef files (Default is $target_platform)"
  echo "  --compile-num-of-cores Number of cpu cores to compile with (more cores makes the compilation process faster, but may cause 'out of swap memory' issue on weak machines)"
  exit 1
}

function parse_args() {
  while test $# -gt 0; do
    if [[ "$1" == "-h" || "$1" == "--help" ]]; then
      print_usage
    elif [ "$1" == "--skip-hailort" ]; then
      skip_hailort=true
    elif [ "$1" == "--target-platform" ]; then
        target_platform=$2
        if [ "$target_platform" == "rpi" ]; then
          compile_num_cores="--compile-num-of-cores 1"
        fi
        shift
    elif [ "$1" == "--compile-num-of-cores" ]; then
        compile_num_cores="--compile-num-of-cores $2"
        shift
    else
      echo "Unknown parameters, exiting"
      print_usage
    fi
    shift
  done
}

function python_venv_create_and_install() {
  # Create new venv or skip
  if [ ! -z $VIRTUAL_ENV ]; then
    echo "Installing into active virtualenv: $VIRTUAL_ENV"
  else
    # Delete old venv
    rm -rf ${VENV_PATH}/$VENV_NAME

    echo "Creating new virtualenv ($VENV_NAME) in ($VENV_PATH) and installing into it"
    python3 -m virtualenv $VENV_PATH/$VENV_NAME
    source ${VENV_PATH}/$VENV_NAME/bin/activate
  fi

  # Install pip packages & Call the downloader script
  pip3 install --upgrade pip setuptools
  pip3 install -r $TAPPAS_WORKSPACE/core/requirements/requirements.txt
  pip3 install -r $TAPPAS_WORKSPACE/core/requirements/gstreamer_requirements.txt
  pip3 install -r $TAPPAS_WORKSPACE/downloader/requirements.txt
  python3 $TAPPAS_WORKSPACE/downloader/main.py $target_platform
}

function clone_external() {
  # Clone required packages
  rm -rf ${TAPPAS_WORKSPACE}/sources
  mkdir -p ${TAPPAS_WORKSPACE}/sources
  pushd ${TAPPAS_WORKSPACE}/sources
  git clone --depth 1 --shallow-submodules -b 0.24.0 https://github.com/xtensor-stack/xtensor.git
  git clone --depth 1 --shallow-submodules -b 0.20.0 https://github.com/xtensor-stack/xtensor-blas.git
  git clone --depth 1 --shallow-submodules -b 0.7.3 https://github.com/xtensor-stack/xtl.git
  git clone --depth 1 --shallow-submodules -b v3.0.0 https://github.com/jarro2783/cxxopts.git
  git clone --depth 1 --shallow-submodules -b v1.1.0 https://github.com/Tencent/rapidjson.git
  git clone --depth 1 --shallow-submodules -b v2.9.0 https://github.com/pybind/pybind11.git
  mkdir -p ${TAPPAS_WORKSPACE}/core/open_source/xtensor_stack/base
  mkdir -p ${TAPPAS_WORKSPACE}/core/open_source/xtensor_stack/blas
  mkdir -p ${TAPPAS_WORKSPACE}/core/open_source/cxxopts
  mkdir -p ${TAPPAS_WORKSPACE}/core/open_source/rapidjson
  mkdir -p ${TAPPAS_WORKSPACE}/core/open_source/pybind11
  cp -r xtensor/include/. ${TAPPAS_WORKSPACE}/core/open_source/xtensor_stack/base
  cp -r xtensor-blas/include/. ${TAPPAS_WORKSPACE}/core/open_source/xtensor_stack/blas
  cp -r xtl/include/. ${TAPPAS_WORKSPACE}/core/open_source/xtensor_stack/base
  cp -r cxxopts/include/. ${TAPPAS_WORKSPACE}/core/open_source/cxxopts
  cp -r rapidjson/include/. ${TAPPAS_WORKSPACE}/core/open_source/rapidjson
  cp -r pybind11/include/. ${TAPPAS_WORKSPACE}/core/open_source/pybind11
  popd
  # Clone Catch2 required packages
  pushd ${TAPPAS_WORKSPACE}/sources
  git clone --depth 1 --shallow-submodules -b v2.13.7 https://github.com/catchorg/Catch2.git
  mkdir -p ${TAPPAS_WORKSPACE}/core/open_source/catch2
  cp -r Catch2/single_include/catch2/. ${TAPPAS_WORKSPACE}/core/open_source/catch2/
  popd
}

function install_hailo() {
  if [ "$skip_hailort" = false ]; then
    sudo dpkg -i ${TAPPAS_WORKSPACE}/hailort/hailort_*_$(dpkg --print-architecture).deb
  fi

  if [ "$target_platform" != "x86" ]; then
    echo "Skipping run_app tool on non x86 target platform..."
  else
    pip3 install -e ${TAPPAS_WORKSPACE}/tools/run_app
    mkdir -p ${TAPPAS_WORKSPACE}/scripts/bash_completion.d
    activate-global-python-argcomplete --dest=${TAPPAS_WORKSPACE}/scripts/bash_completion.d

    if ! grep -Fxq ". $TAPPAS_WORKSPACE/scripts/bash_completion.d/python-argcomplete" ~/.bashrc; then
      echo ". $TAPPAS_WORKSPACE/scripts/bash_completion.d/python-argcomplete" >> ~/.bashrc
    fi
  fi

  # Add GstHailo to the known paths
  # Note, extracting the user site should support both within and without a venv
  # https://stackoverflow.com/a/46071447/5708016
  USER_SITE_DIR=$(python3 -c 'import sysconfig; print(sysconfig.get_paths()["purelib"])')
  mkdir -p $USER_SITE_DIR
  echo "$TAPPAS_WORKSPACE/core/hailo/python/" > "$USER_SITE_DIR/gsthailo.pth"

  $TAPPAS_WORKSPACE/scripts/gstreamer/install_gstreamer.sh

  libgsthailo_version=$(ldd /usr/lib/$(uname -m)-linux-gnu/gstreamer-1.0/libgsthailo.so | grep -o 'libhailort.*' | awk '{print $1}')
  libgsthailo_ver_num=${libgsthailo_version#*libhailort.so.}

  libhailort_version=$(ls /usr/lib/libhailort.so -l)
  libhailort_version_num=${libhailort_version#*libhailort.so.}

  if [ $libgsthailo_ver_num == $libhailort_version_num ]; then
    echo "libgsthailo version was already compiled - will skip compilation"
    ${TAPPAS_WORKSPACE}/scripts/gstreamer/install_hailo_gstreamer.sh --build-mode $GST_HAILO_BUILD_MODE --target-platform $target_platform $compile_num_cores

  else
    echo "found newer version of libgsthailo"
    ${TAPPAS_WORKSPACE}/scripts/gstreamer/install_hailo_gstreamer.sh --build-mode $GST_HAILO_BUILD_MODE --target-platform $target_platform --compile-libgsthailo $compile_num_cores
    
  fi
}

function check_systems_requirements(){
    ./check_system_requirements.sh
    if [ "$?" != "0"  ]; then
        exit 1
    fi
}

function verify_that_hailort_found_if_needed() {
    if [ "$target_platform" != "x86" ]; then
        hailort_sources_dir="$TAPPAS_WORKSPACE/hailort/sources"

        if [ ! -d "$hailort_sources_dir" ]; then 
            echo "HailoRT sources directory not found ($hailort_sources_dir), Please follow our manual installation guide"
            exit 1
        fi
    fi
}

function main() {
  check_systems_requirements
  verify_that_hailort_found_if_needed
  python_venv_create_and_install
  clone_external
  install_hailo
}

parse_args "$@"
main "$@"
