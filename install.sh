#!/bin/bash
set -e

skip_hailort=false
core_only=false
target_platform="x86"
compile_num_cores=""
cross_compile_command=""
gcc_version=12
python_version="3"

if [[ -z "$TAPPAS_WORKSPACE" ]]; then
  export TAPPAS_WORKSPACE=$(dirname "$(realpath "$0")")
  echo "No TAPPAS_WORKSPACE in environment found, using the default one $TAPPAS_WORKSPACE"
fi

readonly GST_HAILO_BUILD_MODE='release'
readonly VENV_NAME='hailo_tappas_venv'
readonly VENV_PATH="$(pwd)"
readonly INSTALLATION_DIR=/usr
readonly TAPPAS_LIB_PATH=${INSTALLATION_DIR}/lib/$(uname -m)-linux-gnu
readonly TAPPAS_GST_PLUGIN_PATH=${TAPPAS_LIB_PATH}/gstreamer-1.0
readonly HAILO_USER_DIR=${HOME}/.hailo/tappas
readonly TAPPAS_BASH_ENV=${HAILO_USER_DIR}/tappas_env
readonly TAPPAS_VERSION=$(grep -a1 project core/hailo/meson.build | grep version | cut -d':' -f2 | tr -d "', ")
readonly DL_REQ_DIR="${TAPPAS_WORKSPACE}/downloader/config/requirements"
declare -A ARCH_TO_DPKG=(
  ["x86"]="amd64"
  ["aarch64"]="arm64"
  ["rockchip"]="amd64"
  ["rpi5"]="arm64"
  ["rpi"]="arm64"
)

function print_usage() {
  echo $python_version
  echo "TAPPAS Install:"
  echo ""
  echo "Options:"
  echo "  --help                 Show this help"
  echo "  --skip-hailort         Skips installation of HailoRT Deb package"
  echo "  --target-platform      Target platform [x86, aarch64, rockchip, rpi(raspberry pi 4),  rpi5(raspberry pi 5)], used for downloading only required media and hef files (Default is $target_platform)"
  echo "  --compile-num-of-cores Number of cpu cores to compile with (more cores makes the compilation process faster, but may cause 'out of swap memory' issue on weak machines)"
  echo "  --download-apps-data   Comma separated list (without spaces) of apps to download data for. Does not work with option '--target-platform'"
  echo "  --list-apps            Show the list of available apps"
  echo "  --core-only            Install tappas core only (no apps data)"
  echo "  --cross-compile-arch   Which arch tappas will cross compile to."
  echo "  --python-version       Will compile to not default python version. Python version - for example '3.11'"

  exit 1
}

function parse_args() {
  while test $# -gt 0; do
    if [[ "$1" == "-h" || "$1" == "--help" ]]; then
      print_usage
    elif [ "$1" == "--skip-hailort" ]; then
      skip_hailort=true
    elif [ "$1" == "--core-only" ]; then
      core_only=true
    elif [ "$1" == "--target-platform" ]; then
      target_platform=$2
      platform_arg_passed=true
      if [ "$target_platform" == "rpi" ]; then
        compile_num_cores="--compile-num-of-cores 1"
      fi
      shift
    elif [ "$1" == "--compile-num-of-cores" ]; then
      compile_num_cores="--compile-num-of-cores $2"
      shift
    elif [ "$1" == "--download-apps-data" ]; then
      apps_to_set=$(echo $2 | tr ',' ' ')
      check_apps_supported
      shift
    elif [ "$1" == "--list-apps" ]; then
      list_supported_apps
    elif [ "$1" == "--cross-compile-arch" ] && [ "$(uname -m)" != "$2" ]; then
      cross_compile_command="--cross-file ${TAPPAS_WORKSPACE}/scripts/gstreamer/$2_cross_file.txt"
      shift
    elif [ "$1" == "--python-version" ]; then
      python_version=$2
      shift
    else
      echo "Unknown parameters, exiting"
      print_usage
    fi
    shift
  done
  if [[ ${apps_to_set} && ${platform_arg_passed} ]]; then
    print_usage
  fi
}

function python_venv_create_and_install() {
  # Create new venv or skip
  if [ ! -z $VIRTUAL_ENV ]; then
    echo "Installing into active virtualenv: $VIRTUAL_ENV"
  else
    # Delete old venv
    rm -rf ${VENV_PATH}/$VENV_NAME

    echo "Creating new virtualenv ($VENV_NAME) in ($VENV_PATH) and installing into it"
    # if target platform is rpi5 add --system-site-packages flag to the virtualenv creation
    if [ "$target_platform" == "rpi5" ]; then
      python3 -m virtualenv --system-site-packages $VENV_PATH/$VENV_NAME
    elif [[ ${#python_version} -gt 1 ]]; then
      # request specifc python_version requires venv
      python${python_version} -m venv $VENV_PATH/$VENV_NAME
    else
      python3 -m virtualenv $VENV_PATH/$VENV_NAME
    fi
    source ${VENV_PATH}/$VENV_NAME/bin/activate
  fi
  # Install pip packages & Call the downloader script
  pip3 install --upgrade pip 'setuptools<=66.0.0'
  pip3 install -r $TAPPAS_WORKSPACE/core/requirements/requirements.txt
  pip3 install -r $TAPPAS_WORKSPACE/core/requirements/gstreamer_requirements.txt
  pip3 install -r $TAPPAS_WORKSPACE/downloader/requirements.txt
  # if rpi5 (core_only) is set dont download apps data (TAPPAS Core mode)
  if [ "$core_only" = false ]; then
    if [[ ${apps_to_set} ]]; then
      python3 $TAPPAS_WORKSPACE/downloader/main.py $target_platform --apps-list $apps_to_set
    else
      python3 $TAPPAS_WORKSPACE/downloader/main.py $target_platform
    fi
  fi
}

function install_hailo() {
  if [ "$skip_hailort" = false ]; then
    yes N | sudo dpkg -i ${TAPPAS_WORKSPACE}/hailort/hailort_*_${ARCH_TO_DPKG[$target_platform]}.deb
  fi

  if [ "$target_platform" != "x86" ]; then
    echo "Skipping run_app tool on non x86 target platform..."
  else
    pip3 install -e ${TAPPAS_WORKSPACE}/tools/run_app
    pip3 install -e ${TAPPAS_WORKSPACE}/tools/trace_analyzer/dot_visualizer
    mkdir -p ${TAPPAS_WORKSPACE}/scripts/bash_completion.d
    activate-global-python-argcomplete --dest=${TAPPAS_WORKSPACE}/scripts/bash_completion.d
    echo ". $TAPPAS_WORKSPACE/scripts/bash_completion.d/_python-argcomplete" >> ${TAPPAS_BASH_ENV}
  fi
  # Add GstHailo to the known paths
  # Note, extracting the user site should support both within and without a venv
  # https://stackoverflow.com/a/46071447/5708016
  USER_SITE_DIR=$(python3 -c 'import sysconfig; print(sysconfig.get_paths()["purelib"])')
  mkdir -p $USER_SITE_DIR
  echo "$TAPPAS_WORKSPACE/core/hailo/python/" > "$USER_SITE_DIR/gsthailo.pth"

  $TAPPAS_WORKSPACE/scripts/gstreamer/install_gstreamer.sh --target-platform $target_platform $cross_compile_command

  libgsthailo_version=$(ldd /usr/lib/$(uname -m)-linux-gnu/gstreamer-1.0/libgsthailo.so | grep -o 'libhailort.*' | awk '{print $1}')
  libgsthailo_ver_num=${libgsthailo_version#*libhailort.so.}
  libhailort_version=$(ls /usr/lib/libhailort.so -l)
  libhailort_version_num=${libhailort_version#*libhailort.so.}

  ${TAPPAS_WORKSPACE}/scripts/gstreamer/install_hailo_gstreamer.sh --build-mode $GST_HAILO_BUILD_MODE --target-platform $target_platform --gcc-version $gcc_version $compile_num_cores $cross_compile_command
  
  # Install source files
  sudo mkdir -p /usr/include/hailo/tappas/sources
  sudo cp -r $TAPPAS_WORKSPACE/core/hailo/libs/postprocesses/	/usr/include/hailo/tappas
  sudo cp -r $TAPPAS_WORKSPACE/core/hailo/general/	/usr/include/hailo/tappas
  sudo cp -r $TAPPAS_WORKSPACE/core/hailo/tracking/	/usr/include/hailo/tappas
  sudo cp -r $TAPPAS_WORKSPACE/core/hailo/metadata/	/usr/include/hailo/tappas
  sudo cp -r $TAPPAS_WORKSPACE/core/hailo/plugins/common/	/usr/include/hailo/tappas
  sudo cp -r $TAPPAS_WORKSPACE/sources/	/usr/include/hailo/tappas/sources
  
  # Copyright
  sudo mkdir -p /usr/share/doc/hailo-tappas-core-${TAPPAS_VERSION}
  sudo cp $TAPPAS_WORKSPACE/LICENSE	/usr/share/doc/hailo-tappas-core-${TAPPAS_VERSION}/copyright

}

function set_gcc_version(){
  if [ "$target_platform" == "rpi" ] || [ "$target_platform" == "rockchip" ]; then
    gcc_version=9
  else
    ubuntu_version=$(lsb_release -r | awk '{print $2}' | awk -F'.' '{print $1}')
    if [ $ubuntu_version -eq 20 ]; then
        gcc_version=9
    fi
  fi
}

function check_systems_requirements(){
  GCC_VERSION=$gcc_version ./check_system_requirements.sh
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

function handle_bash_env(){
  mkdir -p ${HAILO_USER_DIR}
  rm -f ${TAPPAS_BASH_ENV}
  unset GST_PLUGIN_PATH LD_LIBRARY_PATH
  echo "export GST_PLUGIN_PATH=${TAPPAS_GST_PLUGIN_PATH}:\${GST_PLUGIN_PATH}" >> ${TAPPAS_BASH_ENV}
  echo "export LD_LIBRARY_PATH=${TAPPAS_LIB_PATH}:\${LD_LIBRARY_PATH}" >> ${TAPPAS_BASH_ENV}
  echo "export PATH=${INSTALLATION_DIR}/bin:\${PATH}" >> ${TAPPAS_BASH_ENV}
  echo "export PKG_CONFIG_PATH=${INSTALLATION_DIR}/pkgconfig:\${PKG_CONFIG_PATH}" >> ${TAPPAS_BASH_ENV}
  echo ". $TAPPAS_WORKSPACE/core/hailo/hailo_shortcuts.bash" >> ${TAPPAS_BASH_ENV}
  if ! grep -Fxq "[[ -s ${TAPPAS_BASH_ENV} ]] && . ${TAPPAS_BASH_ENV}" ${HOME}/.bashrc; then
    echo "" >> ${HOME}/.bashrc
    echo "[[ -s ${TAPPAS_BASH_ENV} ]] && . ${TAPPAS_BASH_ENV}" >> ${HOME}/.bashrc
  fi
}

function uninstall(){
  ./uninstall.sh
  if [ "$?" != "0"  ]; then
    exit 1
  fi
}

function print_success(){
cat << EOF

    Tappas was successfully installed.
    To start using it please set required environment variables, by running:

    source ${TAPPAS_BASH_ENV}

EOF
}

function setup_pkg_config(){
  ./scripts/misc/pkg_config_setup.sh \
    --tappas-workspace "null" \
    --tappas-version ${TAPPAS_VERSION} \
    --core-only true \
    --target-platform ${target_platform}
}

function _generate_list_of_supported_platforms(){
  list_of_supported_platforms=()
  for platform in $(ls ${DL_REQ_DIR})
  do
    list_of_supported_platforms+=( ${platform} )
  done
}

function generate_list_of_supported_apps(){
  _generate_list_of_supported_platforms
  list_of_supported_apps=()
  for platform in ${list_of_supported_platforms[@]}
  do
    for app in $(ls ${DL_REQ_DIR}/${platform} | cut -f1 -d'.')
    do
      if [[ $platform != "general" ]]; then
        list_of_supported_apps+=( ${platform}/${app#*_} )
      else
        list_of_supported_apps+=( ${platform}/$app )
      fi
    done
  done
}

function check_apps_supported(){
  for app in ${apps_to_set}
  do
    if [[ ! " ${list_of_supported_apps[@]} " =~ " ${app} " ]]; then
      echo "The requested app: ${app} is not supported"
      exit 1
    fi
  done
}

function list_supported_apps(){
  for app in ${list_of_supported_apps[@]}
  do
    echo ${app}
  done
  exit 0
}

function main() {
  uninstall
  set_gcc_version
  check_systems_requirements
  verify_that_hailort_found_if_needed
  python_venv_create_and_install
  $TAPPAS_WORKSPACE/scripts/build_scripts/clone_external_packages.sh
  handle_bash_env
  install_hailo
  setup_pkg_config
  print_success
}

generate_list_of_supported_apps
parse_args "$@"
main "$@"
