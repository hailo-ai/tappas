#!/bin/bash
set -e

script_dir=$(dirname $(realpath "$0"))
source $script_dir/../misc/checks_before_run.sh --export-only

RED='\033[1;31m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
GREEN='\033[1;32m'
NC='\033[0m'
NO_LOG="none"

ubuntu_version=$(lsb_release -r | awk '{print $2}' | awk -F'.' '{print $1}')
if [ $ubuntu_version -eq 20 ]; then
    GCC_VERSION=9
else
    GCC_VERSION=12
fi

no_cache=false
# Occupy all the cores could sometimes freeze the PC
num_cores_to_use=$(($(nproc) / 2))
log_filename="tappas_vaapi.log"
skip_vainfo=false
skip_clean=false

function log() {
    message=$1
    echo -e "$message" >"$(tty)"
}

function log_info() {
    log "${BLUE}INFO:${NC} ${1}"
}

function log_error() {
    log "${RED}ERROR:${NC} ${1}"
}

function log_warning() {
    log "${YELLOW}WARN:${NC} ${1}"
}

function log_success() {
    log "${GREEN}SUCCESS:${NC} ${1}"
}

function prepare_log_file() {
    if [ $log_filename != "$NO_LOG" ]; then
        if [ -f $log_filename ]; then
            >$log_filename
        fi
    fi
}

err_report() {
    pushd ${TAPPAS_WORKSPACE}
    log_error "Error on line $1:"
    log $(awk 'NR>L-4 && NR<L+4 { printf "%-5d%3s%s\n",NR,(NR==L?">>>":""),$0 }' L=$1 $0)
    log_error "please check the log at $log_filename for more information."
    popd
}

function print_usage() {
    echo "Install Hailo GStreamer:"
    echo ""
    echo "Options:"
    echo "  --help                 Show this help"
    echo "  --no-cache             Do not use existing repositories"
    echo "  --log-name             Specify output log file name (default is $log_filename) - Pass $NO_LOG string to disable"
    echo "  --compile-num-of-cores Number of cpu cores to compile with (more cores makes the compilation process faster, but may cause 'out of swap memory' issue on weak machines)"
    echo "  --skip-vainfo          Skip VA-Info check"
    echo "  --skip-clean           Skip the cleaning of build artifacts"
    exit 1
}

function parse_args() {
    while test $# -gt 0; do
        if [[ "$1" == "-h" || "$1" == "--help" ]]; then
            print_usage
        elif [ "$1" == "--no-cache" ]; then
            no_cache=true
        elif [ "$1" == "--compile-num-of-cores" ]; then
            num_cores_to_use=$2
            shift
        elif [ "$1" == "--log-name" ]; then
            log_filename=$2
            shift
        elif [ "$1" == "--skip-vainfo" ]; then
            skip_vainfo=true
        elif [ "$1" == "--skip-clean" ]; then
            skip_clean=true
        else
            echo "Unknown parameters, exiting"
            print_usage
        fi
        shift
    done
}

function get_versions() {
    ubuntu_version=$(lsb_release -r | awk '{print $2}' | awk -F'.' '{print $1}')
    # Driver version is according to https://packages.ubuntu.com/search?keywords=intel-media-va-driver
    if [ $ubuntu_version -eq 22 ]; then
        # ------
        # Ubuntu 22 release settings
        readonly GMMLIB_VERSION="intel-gmmlib-22.3.3"
        readonly LIBVA_VERSION="2.17.0"
        readonly LIBVA_UTILS_VERSION="$LIBVA_VERSION"
        readonly MEDIA_DRIVER_VERSION="master"
        
        # We are cloning a specific commit (SW swizzling regression fix for Gen8/9/10) from Intel media-driver repo, until a proper release is out.
        # This fix is mandatory for the performance of video management system pipeline.
        # https://github.com/intel/media-driver/commit/4c2547e97347d832458002162897df306b7e99eb
        readonly MEDIA_DRIVER_COMMIT_HASH="4c2547e97347d832458002162897df306b7e99eb"
        # ------
    elif [ $ubuntu_version -eq 20 ]; then
        # ------
        # Ubuntu 20.04 settings
        readonly GMMLIB_VERSION="intel-gmmlib-20.1.1"
        readonly MEDIA_DRIVER_VERSION="intel-media-20.1.1"
        readonly LIBVA_VERSION="2.7.1"
        readonly LIBVA_UTILS_VERSION="$LIBVA_VERSION"
        readonly MEDIA_DRIVER_COMMIT_HASH=""
        # ------
    else
        log_error "ubuntu version $ubuntu_version is not supported. Supporting only ubuntu 20 or ubuntu 22"
        exit 1
    fi
}

function get_repo() {
    trap 'err_report $LINENO' ERR
    repo_name=$1
    repo_version=$2
    commit_hash=$3

    if [ $no_cache = true ]; then
        rm -rf $repo_name
    fi
    if [ ! -d "$repo_name" ]; then
        git clone https://github.com/intel/$repo_name.git -b $repo_version
    fi
    pushd $repo_name
    # Checkout specific commit if commit hash is provided
    if [ ! -z "$commit_hash" ]; then
        git checkout $commit_hash
    fi
}

function install_va_requirements() {
    trap 'err_report $LINENO' ERR
    log_info "Installing Requirements"
    sudo apt-get update
    sudo DEBIAN_FRONTEND=noninteractive apt-get install -y libxfixes-dev autoconf libtool libdrm-dev xorg xorg-dev openbox libx11-dev libgl1-mesa-glx libgl1-mesa-dev libxcb-dri3-0 libxcb-dri3-dev
}

function install_vaapi_prerequisites() {
    trap 'err_report $LINENO' ERR
    log_info "Installing Prerequisites"
    sudo DEBIAN_FRONTEND=noninteractive apt-get install -y intel-gpu-tools
}

function gmmlib_install() {
    trap 'err_report $LINENO' ERR
    log_info "Compiling gmmlib"
    get_repo gmmlib "$GMMLIB_VERSION"
    mkdir -p build && pushd build
    cmake -DCMAKE_BUILD_TYPE=Release CMAKE_C_COMPILER=/usr/bin/gcc-$GCC_VERSION -DCMAKE_CXX_COMPILER=/usr/bin/g++-$GCC_VERSION ../
    cmake --build . --config Release -j $num_cores_to_use
    sudo cmake --install .
    
    popd
    if [ "$skip_clean" = false ]; then
        rm -rf build
    fi
    popd
}

function libva_install() {
    trap 'err_report $LINENO' ERR
    log_info "Compiling libva"
    get_repo libva "$LIBVA_VERSION"
    meson libva_build --buildtype release -Dwith_x11=yes
    ninja -C libva_build
    sudo env "PATH=$PATH" ninja -C libva_build install
    
    if [ "$skip_clean" = false ]; then
        rm -rf libva_build
    fi

    popd
}

function libva_utils_install() {
    trap 'err_report $LINENO' ERR
    log_info "Compiling libva utils"
    get_repo libva-utils "$LIBVA_UTILS_VERSION"
    meson libva_build --buildtype release
    ninja -C libva_build
    sudo env "PATH=$PATH" ninja -C libva_build install

    if [ "$skip_clean" = false ]; then
        rm -rf libva_build
    fi

    popd
}

function media_driver_install() {
    trap 'err_report $LINENO' ERR
    log_info "Compiling intel media driver"

    get_repo media-driver "$MEDIA_DRIVER_VERSION" "$MEDIA_DRIVER_COMMIT_HASH"
    mkdir -p build && pushd build

    cmake -DCMAKE_BUILD_TYPE=Release CMAKE_C_COMPILER=/usr/bin/gcc-$GCC_VERSION -DCMAKE_CXX_COMPILER=/usr/bin/g++-$GCC_VERSION ../
    make -j $num_cores_to_use
    sudo make install

    popd
    if [ "$skip_clean" = false ]; then
        rm -rf build
    fi
    popd
}

function check_va_info() {
    trap 'err_report $LINENO' ERR
    log_info "Checking validity via vainfo"

    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/x86_64-linux-gnu/
    export LIBVA_DRIVER_NAME=iHD

    vainfo
}

function install_gstreamer_vaapi() {
    trap 'err_report $LINENO' ERR
    log_info "Installing gstreamer1.0-vaapi"

    sudo apt-get install -y gstreamer1.0-vaapi

    export GST_VAAPI_ALL_DRIVERS=1

    set +e
    gst_inspect_vaapi_output=$(gst-inspect-1.0 vaapi)
    gst_inspect_vaapi_return_code=$?
    set -e

    if [[ $gst_inspect_vaapi_return_code -ne 0 ]]; then
        log_error "gst-inspect-1.0 failed, please look at the log."
        exit 1
    fi
}

function install_libva_essentials() {
    trap 'err_report $LINENO' ERR
    install_va_requirements
    install_vaapi_prerequisites
    gmmlib_install
    libva_install
    libva_utils_install
    media_driver_install
}

function main() {
    trap 'err_report $LINENO' ERR
    pushd ${TAPPAS_WORKSPACE}/sources
    install_libva_essentials
    install_gstreamer_vaapi

    if [ "$skip_vainfo" = false ]; then
        check_va_info
    fi

    log_success "Installed VA-API successfully"
}

trap 'err_report $LINENO' ERR
parse_args $@
prepare_log_file
get_versions

if [ "$log_filename" == "$NO_LOG" ]; then
    main
else
    main >$log_filename 2>&1
fi
