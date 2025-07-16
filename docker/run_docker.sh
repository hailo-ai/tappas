readonly DEFAULT_IMAGE_NAME="hailo_tappas"
readonly DEFAULT_TAG="latest"

IMAGE_NAME=$DEFAULT_IMAGE_NAME
TAG=$DEFAULT_TAG
RESUME_CONTAINER=false
OVERRIDE_CONTAINER=false
CONTAINER_NAME="hailo_tappas_container"
RESUME_COMMAND=/bin/bash
XAUTH_FILE_PC=/tmp/hailo_docker.xauth
XAUTH_FILE_CONTAINER=/home/hailo/.Xauthority
SHARED_DIR="shared_with_docker"
HAILORT_ENABLE_SERVICE=false
DOCKER_ARGS=""

function print_usage() {
    echo "Run Hailo Docker:"
    echo "The default mode is trying to create a new container"
    echo ""
    echo "Options:"
    echo "  --help                        Show this help"
    echo "  --resume                      Resume an old container"
    echo "  --resume-command              Resume command (used only when --resume flag is used)"
    echo "  --override                    Start a new container, if exists already, delete the previous one"
    echo "  --tag                         The selected tag, default is - $DEFAULT_TAG"
    echo "  --image-name                  The selected image name, default is - $DEFAULT_IMAGE_NAME"
    echo "  --hailort-enable-service      Enable HailoRT as a service"
    exit 1
}

function parse_args() {
    while test $# -gt 0; do
        if [[ "$1" == "-h" || "$1" == "--help" ]]; then
            print_usage
        elif [ "$1" == "--resume" ]; then
            RESUME_CONTAINER=true
        elif [ "$1" == "--resume-command" ]; then
            RESUME_COMMAND=$2
            shift
        elif [ "$1" == "--override" ]; then
            OVERRIDE_CONTAINER=true
        elif [ "$1" == "--tag" ]; then
            TAG=$2
            shift
        elif [ "$1" == "--image-name" ]; then
            IMAGE_NAME=$2
            shift
        elif [ "$1" == "--hailort-enable-service" ]; then
            HAILORT_ENABLE_SERVICE=true
        else
            echo "Unknown parameters, exiting"
            print_usage
        fi
        shift
    done

    CONTAINER_NAME="${CONTAINER_NAME}_${TAG}"
}

function prepare_docker_args() {
    DOCKER_ARGS="--privileged --net=host \
        --name "$CONTAINER_NAME" \
        --ipc=host \
        --device /dev/dri:/dev/dri \
        -v ${XAUTH_FILE_PC}:${XAUTH_FILE_CONTAINER} \
        -v /tmp/.X11-unix/:/tmp/.X11-unix/ \
        -v /dev:/dev \
        -v /lib/firmware:/lib/firmware \
        --group-add 44 \
        -e DISPLAY=$DISPLAY \
        -e XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR \
        -e hailort_enable_service=yes \
        -v $(pwd)/${SHARED_DIR}/:/local/${SHARED_DIR}:rw"
}

function handle_xauth() {
    # The function extracts auth entry for current display and saves it to specified file.
    # It's a workaround for ub22 random name of xauth file, which changes every reboot.
    touch $XAUTH_FILE_PC
    xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f $XAUTH_FILE_PC nmerge -
    chmod o+rw $XAUTH_FILE_PC
}

function create_shared_dir() {
    mkdir -p ${SHARED_DIR}
    chmod o+rwx ${SHARED_DIR}
}

function run_docker() {
    # Critical for display
    xhost local:root >/dev/null
    handle_xauth
    create_shared_dir

    num_of_containers_exists=$(docker ps -a -q -f "name=$CONTAINER_NAME" | wc -l)

    if [ "$RESUME_CONTAINER" = true ]; then
        # Finding out if a container already exists
        if [[ "$num_of_containers_exists" -lt "1" ]]; then
            echo "No container found. please run for the first time without --resume"
            exit 1
        fi

        echo "Resuming an old container"
        # Start and then exec in order to pass the DISPLAY env, because this vairble
        # might change from run to run (after reboot for example)
        docker start "$CONTAINER_NAME"
        docker exec -it -e DISPLAY=$DISPLAY "$CONTAINER_NAME" bash -c "$RESUME_COMMAND"
    else
        if [[ "$num_of_containers_exists" -ge "1" ]]; then
            if [ "$OVERRIDE_CONTAINER" = true ]; then
                echo "Old container found while overriding, deleting the old one"
                docker stop "$CONTAINER_NAME" >/dev/null
                docker rm "$CONTAINER_NAME" >/dev/null
            else
                echo "Can't start a new cotainer, already found one. Consider using --resume or --override"
                exit 1
            fi
        fi

        echo "Trying to start a new container"

        os_distribution=$(cat /etc/os-release | grep ^ID= | awk -F'=' '{print $2}' | xargs)

        if [[ $os_distribution != "ubuntu" && $os_distribution != "debian" ]]; then
            echo "OS: $os_distribution is not supported"
            exit 1
        fi

        prepare_docker_args

        docker run $DOCKER_ARGS -it $IMAGE_NAME:$TAG
    fi

}

parse_args "$@"
run_docker
