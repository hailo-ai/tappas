from pathlib import Path

DOCKERFILE_ADD_SSH_KEYS_TEXT = """# Authorize SSH Host, Add the keys and set permissions
ARG ssh_prv_key
ARG ssh_pub_key
RUN mkdir -p ~/.ssh && \\
    chmod 0700 ~/.ssh && \\
    echo "$ssh_prv_key" > ~/.ssh/id_rsa && \\
    echo "$ssh_pub_key" > ~/.ssh/id_rsa.pub && \\
    chmod 600 ~/.ssh/id_rsa && \\
    chmod 600 ~/.ssh/id_rsa.pub && \\
    ssh-keyscan 192.168.12.21 freenas > ~/.ssh/known_hosts
"""

BUILD_DOCKER_BEFORE_MODIFY = """    --build-arg skip_headers_install="$skip_headers_install" \\
    --build-arg ssh_prv_key="$(cat ~/.ssh/id_rsa)" \\
    --build-arg ssh_pub_key="$(cat ~/.ssh/id_rsa.pub)" ."""

BUILD_DOCKER_CROSS_BEFORE_MODIFY = """    --build-arg gst_hailo_build_mode="$gst_hailo_build_mode" \\
    --build-arg ssh_prv_key="$(cat ~/.ssh/id_rsa)" \\
    --build-arg ssh_pub_key="$(cat ~/.ssh/id_rsa.pub)" ."""

BUILD_DOCKER_AFTER_MODIFY = """    --build-arg skip_headers_install="$skip_headers_install" ."""

BUILD_DOCKER_CROSS_AFTER_MODIFY = """    --build-arg gst_hailo_build_mode="$gst_hailo_build_mode" ."""


PARENT_TAPPAS_WORKSPACE = Path(__file__).resolve().parent.parent.parent.parent
DOCKERFILE_TAPPAS_BASE_PATH = PARENT_TAPPAS_WORKSPACE / "docker" / "Dockerfile.tappas_base"
BUILD_DOCKER_PATH = PARENT_TAPPAS_WORKSPACE / "build_docker.sh"
BUILD_DOCKER_CROSS_PATH = PARENT_TAPPAS_WORKSPACE / "build_docker_cross_compile.sh"


def remove_ssh_keys(dockerfile_path: Path):
    dockerfile_content = dockerfile_path.read_text()
    dockerfile_content = dockerfile_content.replace(DOCKERFILE_ADD_SSH_KEYS_TEXT, '')

    dockerfile_path.write_text(dockerfile_content)


def remove_ssh_keys_from_build_docker(build_docker_file_path: Path):
    build_docker_content = build_docker_file_path.read_text()
    build_docker_content = build_docker_content.replace(BUILD_DOCKER_BEFORE_MODIFY, BUILD_DOCKER_AFTER_MODIFY)

    build_docker_file_path.write_text(build_docker_content)


def remove_ssh_keys_from_build_docker_cross_compile(build_docker_file_path: Path):
    build_docker_content = build_docker_file_path.read_text()
    build_docker_content = build_docker_content.replace(BUILD_DOCKER_CROSS_BEFORE_MODIFY, BUILD_DOCKER_CROSS_AFTER_MODIFY)

    build_docker_file_path.write_text(build_docker_content)

if __name__ == '__main__':
    remove_ssh_keys(DOCKERFILE_TAPPAS_BASE_PATH)
    remove_ssh_keys_from_build_docker(BUILD_DOCKER_PATH)
    remove_ssh_keys_from_build_docker_cross_compile(BUILD_DOCKER_CROSS_PATH)
