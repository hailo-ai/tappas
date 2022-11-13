set -e

RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

function log_info() {
    echo -e "${BLUE}INFO:${NC} ${1}" >$(tty)
}

function log_error() {
    echo -e "${RED}ERROR:${NC} ${1}" >$(tty)
}

function log_warning() {
    echo -e "${YELLOW}WARN:${NC} ${1}" >$(tty)
}


INSTALL_LOG_FILE="tappas_vaapi.log"
function prepare_log_file(){
    if [ -f $INSTALL_LOG_FILE ]
        then
           > $INSTALL_LOG_FILE
    fi
}

err_report() {
    pushd ${TAPPAS_WORKSPACE}
    log_error "Error on line $1:"
    awk 'NR>L-4 && NR<L+4 { printf "%-5d%3s%s\n",NR,(NR==L?">>>":""),$0 }' L=$1 $0 > $(tty)
    log_error "please check the log at $INSTALL_LOG_FILE for more information."
    popd
}

function get_versions() {
    ubuntu_version=$(lsb_release -r | awk '{print $2}' | awk -F'.' '{print $1}')
    if [ $ubuntu_version -eq 22 ]; then
        # ------
        # Ubuntu 22 release settings
        readonly GMMLIB_VERSION="intel-gmmlib-22.1.2"
        readonly MEDIA_DRIVER_VERSION="intel-media-22.3.1"
        readonly LIBVA_VERSION="2.14.0"
        readonly LIBVA_UTILS_VERSION="$LIBVA_VERSION"
        # ------
    elif [ $ubuntu_version -eq 20 ]; then
        # ------
        # Ubuntu 20.04 settings
        readonly GMMLIB_VERSION="intel-gmmlib-19.3.1"
        readonly MEDIA_DRIVER_VERSION="intel-media-19.3.0"
        readonly LIBVA_VERSION="2.6.0.pre1"
        readonly LIBVA_UTILS_VERSION="$LIBVA_VERSION"
        # ------
    else
        log_error "ubuntu version $ubuntu_version is not supported. Supporting only ubuntu 20 or ubuntu 22"
        exit 1
    fi
}


function gmmlib_install() {
    trap 'err_report $LINENO' ERR
    log_info "Compiling gmmlib"
    rm -rf gmmlib
    git clone https://github.com/intel/gmmlib -b "$GMMLIB_VERSION"
    pushd gmmlib
    mkdir build && pushd build
    cmake -DCMAKE_BUILD_TYPE=Release CMAKE_C_COMPILER=/usr/bin/gcc-9 -DCMAKE_CXX_COMPILER=/usr/bin/g++-9 ../
    cmake --build . --config Release -j 8
    sudo cmake --install .
    popd
    popd
}

function libva_install() {
    trap 'err_report $LINENO' ERR
    log_info "Compiling libva"
    rm -rf libva
    git clone https://github.com/intel/libva.git -b "$LIBVA_VERSION"
    pushd libva
    meson libva_build --buildtype release
    ninja -C libva_build
    sudo ninja -C libva_build install
    popd
}

function libva_utils_install() {
    trap 'err_report $LINENO' ERR
    log_info "Compiling libva utils"
    rm -rf libva-utils
    git clone https://github.com/intel/libva-utils.git -b "$LIBVA_VERSION"
    pushd libva-utils
    meson libva_build --buildtype release
    ninja -C libva_build
    sudo ninja -C libva_build install
    popd
}

function media_driver_install() {
    trap 'err_report $LINENO' ERR
    log_info "Compiling intel media driver"
    sudo apt-get update
    sudo DEBIAN_FRONTEND=noninteractive apt-get install -y autoconf libtool libdrm-dev xorg xorg-dev openbox libx11-dev libgl1-mesa-glx libgl1-mesa-dev

    rm -rf media-driver
    git clone https://github.com/intel/media-driver.git -b "$MEDIA_DRIVER_VERSION"
    pushd media-driver
    mkdir build && pushd build

    cmake -DCMAKE_BUILD_TYPE=Release CMAKE_C_COMPILER=/usr/bin/gcc-9 -DCMAKE_CXX_COMPILER=/usr/bin/g++-9 ../
    make -j $(nproc)
    sudo make install
    popd
    popd
}

function check_va_info() {
    trap 'err_report $LINENO' ERR
    log_info "Checking validity via vainfo"
    log_info "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/x86_64-linux-gnu/"
    log_info "export LIBVA_DRIVER_NAME=iHD"
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib/x86_64-linux-gnu/
    export LIBVA_DRIVER_NAME=iHD

    vainfo
}

function install_gstreamer_vaapi() {
    trap 'err_report $LINENO' ERR
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
    gmmlib_install
    libva_install
    libva_utils_install
    media_driver_install
}

function main() {
    trap 'err_report $LINENO' ERR
    pushd ${TAPPAS_WORKSPACE}/sources
    install_libva_essentials
    check_va_info
    install_gstreamer_vaapi
}

trap 'err_report $LINENO' ERR
prepare_log_file
get_versions
main > $INSTALL_LOG_FILE 2>&1   
