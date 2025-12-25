#!/bin/bash

# The script updates pkg-config file with correct TAPPAS_WORKSPACE value
# and copies the file to ${TAPPAS_PKG_CONFIG_DIR}
# in order to make this file available for the pkg-config, the variable
# must be set:
# export PKG_CONFIG_PATH=${TAPPAS_PKG_CONFIG_DIR}
# It is added to $HOME/.hailo/tappas/tappas_env after tappas installation

set -e

readonly TAPPAS_PKG_CONFIG_FILE_TEMPLATE="hailo_tappas.pc_template"
readonly TAPPAS_CORE_PKG_CONFIG_FILE_TEMPLATE="hailo-tappas-core.pc_template"
TAPPAS_PKG_CONFIG_FILE_BY_MODE=$TAPPAS_PKG_CONFIG_FILE_TEMPLATE
ARCH=$(uname -m)
STATIC_OPENCV=false
LOG_PREFIX="[pkg_config_setup]"
function print_usage(){
cat << EOF

    Usage: $0 --tappas-workspace TAPPAS_WORKSPACE_PATH --tappas-version VERSION [--core-only]

EOF
  return 1
}

function parse_args(){
  while test $# -gt 0; do
    if [[ "$1" == "-h" || "$1" == "--help" ]]; then
      print_usage
    elif [ "$1" == "--tappas-workspace" ]; then
      TAPPAS_WORKSPACE=$2
      shift 2
    elif [ "$1" == "--tappas-version" ]; then
      TAPPAS_VERSION=$2
      shift 2
    elif [ "$1" == "--target-platform" ]; then
      ARCH=$2
      case "$ARCH" in
        x86|x86_64)
          ARCH="x86_64"
          ;;
        rpi5|rockchip|aarch64|rpi)
          ARCH="aarch64"
          ;;
        *)
          echo "Unknown target platform: $ARCH"
          print_usage
          ;;
      esac
      shift 2
    elif [ "$1" == "--core-only" ]; then
      TAPPAS_PKG_CONFIG_FILE_BY_MODE=$TAPPAS_CORE_PKG_CONFIG_FILE_TEMPLATE
      shift 2
    elif [ "$1" == "--static-opencv" ]; then
      if [ "$2" == "true" ]; then
        STATIC_OPENCV=true
      else
        STATIC_OPENCV=false
      fi
      shift 2
    else
      echo "Unknown parameters, exiting"
      print_usage
    fi
  done
}

function prepare_pkg_config_dir(){
  echo "${LOG_PREFIX} prepare pkg-config dir: ${TAPPAS_PKG_CONFIG_DIR}" >&2
  sudo rm -rf ${TAPPAS_PKG_CONFIG_DIR}/hailo-tappas-core.pc
  sudo mkdir -p ${TAPPAS_PKG_CONFIG_DIR}
}

function update_pc_file(){
  TAPPAS_PKG_CONFIG_FILE=${TAPPAS_PKG_CONFIG_FILE_BY_MODE%_template}
  echo "${LOG_PREFIX} update pc file: template=pkg_config/${TAPPAS_PKG_CONFIG_FILE_BY_MODE} output=pkg_config/${TAPPAS_PKG_CONFIG_FILE}" >&2
  sed "s|tappas_workspace=|tappas_workspace=${TAPPAS_WORKSPACE}|; \
        s|arch=|arch=${ARCH}|; \
        s|Version:|Version: ${TAPPAS_VERSION}|" \
        pkg_config/${TAPPAS_PKG_CONFIG_FILE_BY_MODE} > pkg_config/${TAPPAS_PKG_CONFIG_FILE}
  if [ "$STATIC_OPENCV" == true ]; then
    echo "${LOG_PREFIX} static opencv enabled: removing opencv4 from Requires" >&2
    sed -i "s|Requires: opencv4 gstreamer-1.0|Requires: gstreamer-1.0|" pkg_config/${TAPPAS_PKG_CONFIG_FILE}
  fi
}

function copy_pc_file(){
  echo "${LOG_PREFIX} copy pc file: pkg_config/${TAPPAS_PKG_CONFIG_FILE} -> ${TAPPAS_PKG_CONFIG_DIR}" >&2
  sudo cp pkg_config/${TAPPAS_PKG_CONFIG_FILE} ${TAPPAS_PKG_CONFIG_DIR}
}

function main(){
  TAPPAS_INSTALLATION_DIR=/usr/lib/${ARCH}-linux-gnu
  TAPPAS_PKG_CONFIG_DIR=${TAPPAS_INSTALLATION_DIR}/pkgconfig
  echo "${LOG_PREFIX} ARCH=${ARCH}" >&2
  echo "${LOG_PREFIX} TAPPAS_INSTALLATION_DIR=${TAPPAS_INSTALLATION_DIR}" >&2
  echo "${LOG_PREFIX} TAPPAS_PKG_CONFIG_DIR=${TAPPAS_PKG_CONFIG_DIR}" >&2
  prepare_pkg_config_dir
  pushd $(dirname $0)
  update_pc_file
  copy_pc_file
  echo "${LOG_PREFIX} content of ${TAPPAS_PKG_CONFIG_DIR}/${TAPPAS_PKG_CONFIG_FILE}:" >&2
  cat ${TAPPAS_PKG_CONFIG_DIR}/${TAPPAS_PKG_CONFIG_FILE}  
  popd
  echo "${LOG_PREFIX} done" >&2
}

parse_args $@
main $@
