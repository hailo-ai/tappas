#!/bin/bash

# The script updates pkg-config file with correct TAPPAS_WORKSPACE value
# and copies the file to ${TAPPAS_PKG_CONFIG_DIR}
# in order to make this file available for the pkg-config, the variable
# must be set:
# export PKG_CONFIG_PATH=${TAPPAS_PKG_CONFIG_DIR}
# It is added to $HOME/.hailo/tappas/tappas_env after tappas installation

set -e

readonly TAPPAS_INSTALLATION_DIR=/opt/hailo/tappas
readonly TAPPAS_PKG_CONFIG_DIR=${TAPPAS_INSTALLATION_DIR}/pkgconfig
readonly TAPPAS_PKG_CONFIG_FILE_TEMPLATE="hailo_tappas.pc_template"
readonly TAPPAS_CORE_PKG_CONFIG_FILE_TEMPLATE="hailo-tappas-core.pc_template"
TAPPAS_PKG_CONFIG_FILE_BY_MODE=$TAPPAS_PKG_CONFIG_FILE_TEMPLATE
readonly ARCH=$(uname -m)

function print_usage(){
cat << EOF

    Usage: $0 --tappas-workspace TAPPAS_WORKSPACE_PATH --tappas-version VERSION [--core-only]

EOF
  return 1
}

function parse_args(){
  if [[ $# -lt 4 || $# -gt 6 ]]; then
      print_usage
  fi
  while test $# -gt 0; do
    if [[ "$1" == "-h" || "$1" == "--help" ]]; then
      print_usage
    elif [ "$1" == "--tappas-workspace" ]; then
      TAPPAS_WORKSPACE=$2
      shift 2
    elif [ "$1" == "--tappas-version" ]; then
      TAPPAS_VERSION=$2
      shift 2
    elif [ "$1" == "--core-only" ]; then
      TAPPAS_PKG_CONFIG_FILE_BY_MODE=$TAPPAS_CORE_PKG_CONFIG_FILE_TEMPLATE
      shift 2
    else
      echo "Unknown parameters, exiting"
      print_usage
    fi
  done
}

function prepare_pkg_config_dir(){
  sudo rm -rf ${TAPPAS_PKG_CONFIG_DIR}
  sudo mkdir -p ${TAPPAS_PKG_CONFIG_DIR}
}

function update_pc_file(){
  TAPPAS_PKG_CONFIG_FILE=${TAPPAS_PKG_CONFIG_FILE_BY_MODE%_template}
  sed "s|tappas_workspace=|tappas_workspace=${TAPPAS_WORKSPACE}|; \
        s|arch=|arch=${ARCH}|; \
        s|Version:|Version: ${TAPPAS_VERSION}|" \
        pkg_config/${TAPPAS_PKG_CONFIG_FILE_BY_MODE} > pkg_config/${TAPPAS_PKG_CONFIG_FILE}
}

function copy_pc_file(){
  sudo cp pkg_config/${TAPPAS_PKG_CONFIG_FILE} ${TAPPAS_PKG_CONFIG_DIR}
}

function main(){
  prepare_pkg_config_dir
  pushd $(dirname $0)
  update_pc_file
  copy_pc_file
  popd
}

parse_args $@
main $@
