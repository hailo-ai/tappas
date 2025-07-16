#!/bin/bash
# Build docker must be in the parent directory. Otherwise copying the whole directory tree is not allowed
set -e

dockerfile_dir="docker"
dockerfile_tappas_base="Dockerfile.tappas_base"
dockerfile_tappas="Dockerfile.tappas"

tappas_base_image_name="hailo_tappas_base"
tappas_image_name="hailo_tappas"

tag=latest
flags=""
gst_hailo_build_mode="release"
target_platform="x86"
ubuntu_version="20.04"
gcc_version="12"
install_vaapi=false

RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

function print_usage() {
    echo "Run Hailo Docker:"
    echo ""
    echo "Options:"
    echo "  --help                   Show this help"
    echo "  --no-cache               Build Docker without cache"
    echo "  --build-mode             Build mode for Hailo's GStreamer elements - release/debug (Default is $gst_hailo_build_mode)"
    echo "  --target-platform        Target platform [x86, arm, imx8, rpi(raspberry pi)], used for downloading only required media and hef files (Default is $target_platform)"
    echo "  --tag                    The selected tag, default is - $tag"
    echo "  --ubuntu-version         Ubuntu version of the docker [20.04 / 21.04 / 22.04] - Default is $ubuntu_version"
    echo "  --install-vaapi          Whether the Docker should be build VA-API as well"
    exit 1
}

function check_docker_version() {
    # https://stackoverflow.com/a/72057185/5708016
    if [[ "${ubuntu_version}" == "22.04" ]]; then
        docker_version=$(docker version --format '{{.Server.Version}}')
        # Docker version could be x.y.z or x.y.z-beta.a
        # This awk with multiple delimiters support them both
        docker_version_major=$(echo "$docker_version" | awk -F'[.-]' '{print $1}')
        docker_version_minor=$(echo "$docker_version" | awk -F'[.-]' '{print $2}')
        docker_version_build=$(echo "$docker_version" | awk -F'[.-]' '{print $3}')

        if [ "${docker_version_major}" -lt 20 ] &&
            [ "${docker_version_minor}" -lt 10 ] &&
            [ "${docker_version_build}" -lt 9 ]; then
            echo "Docker version $docker_version is less than 20.10.9, can't build Ubuntu 22.04"
            exit 1
        fi
    fi
}

function log_error() {
    echo -e "${RED}ERROR:${NC} ${1}"
}

function log_warning() {
    echo -e "${YELLOW}WARN:${NC} ${1}"
}

function confirm() {
    read -r -p "${1:-Are you sure? [y/N]} " response
    case "$response" in
    [yY][eE][sS] | [yY])
        true
        ;;
    *)
        false
        ;;
    esac
}

function check_if_old_mp4_or_hef_found() {
    mp4_or_hef_files_wc=$(find . -name '*.mp4' -o -name '*.hef' -not -path './hailort*' | wc -l)

    if [[ $mp4_or_hef_files_wc -gt 0 ]]; then
        log_warning "Some HEFs and MP4s found, which could cause issues. It is recommended to delete them before proceeding"
        confirm "Should we delete them? [y/N]" && (find . -name '*.mp4' -o -name '*.hef' -not -path './hailort*' | xargs rm)
    fi
}

function parse_args() {
    while test $# -gt 0; do
        if [[ "$1" == "-h" || "$1" == "--help" ]]; then
            print_usage
        elif [ "$1" == "--no-cache" ]; then
            flags="--no-cache"
        elif [ "$1" == "--build-mode" ]; then
            gst_hailo_build_mode=$2
            shift
        elif [ "$1" == "--target-platform" ]; then
            target_platform=$2
            if [ "$target_platform" == "rpi" ]; then
                gcc_version="9"
            fi
            shift
        elif [ "$1" == "--tag" ]; then
            tag=$2
            shift
        elif [ "$1" == "--ubuntu-version" ]; then
            ubuntu_version="$2"
            shift

            if [[ "${ubuntu_version}" != @(20.04|21.04|22.04) ]]; then
                echo "Ubuntu version provided is not supported: $ubuntu_version"
                print_usage
            fi

            if [[ "${ubuntu_version}" == 20.04 ]]; then
                gcc_version="9"
            fi

            check_docker_version
        elif [ "$1" == "--install-vaapi" ]; then
            install_vaapi=true
        else
            echo "Unknown parameters, exiting"
            print_usage
        fi
        shift
    done
}

check_if_old_mp4_or_hef_found
parse_args "$@"

# BuildKit is an improved backend to replace the legacy builder
export DOCKER_BUILDKIT=1

docker build -f ${dockerfile_dir}/${dockerfile_tappas_base} -t "$tappas_base_image_name":"$tag" \
    --build-arg ubuntu_version="$ubuntu_version" \
    --build-arg ssh_prv_key="$(cat ~/.ssh/id_rsa)" \
    --build-arg ssh_pub_key="$(cat ~/.ssh/id_rsa.pub)" .
docker build $flags -f ${dockerfile_dir}/${dockerfile_tappas} -t "$tappas_image_name":"$tag" \
    --build-arg gcc_version="$gcc_version" \
    --build-arg target_platform="$target_platform" \
    --build-arg hailo_tappas_tag="$tag" \
    --build-arg gst_hailo_build_mode="$gst_hailo_build_mode" \
    --build-arg install_vaapi=$install_vaapi .
