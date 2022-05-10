# Using Dockers

## Install Docker

The section below would help you with the installation of Docker.

```sh
# Install curl
sudo apt-get install -y curl

# Get and install docker
curl -fsSL https://get.docker.com -o get-docker.sh
sh get-docker.sh

# Add your user (who has root privileges) to the Docker group
sudo usermod -aG docker $USER

# Reboot/log out in order to apply the changes to the group  
sudo reboot
```

> **Note**: Consider reading [Running out of disk space](#running-out-of-disk-space) if your system is space limited

## Running TAPPAS container from pre-built Docker image

### Preparations

[HailoRT PCIe driver](https://github.com/hailo-ai/hailort-drivers) is required - install instructions are provided in HailoRT documentations. make sure that the driver is installed correctly by: [Verify Hailo installation](./verify_hailoRT.md).

Download from Hailo developer zone `tappas_VERSION_ARCH_docker.zip` and unzip the file, it should contain the following files:

- **hailo_docker_tappas_VERSION.tar**: the pre-built docker image
- **run_tappas_docker.sh**: Script that loads and runs the docker image
- **dockerfile.tappas_run**: Dockerfile used within the first load

### Running for the first time

In order to use TAPPAS release Docker image, you should run the following script:

```sh
./run_tappas_docker.sh --tappas-image TAPPAS_IMAGE_PATH
```

> **NOTE:** TAPPAS_IMAGE_PATH is the path to the **hailo_docker_tappas_VERSION.tar**

The script would load the docker image, and start a new container.
The script might take a couple of minutes, and after that, you are ready to go.

### Resuming (Second time and on)

From now an on you should run the script with the `--resume` flag

```sh
./run_tappas_docker.sh --resume
```

> **NOTE**: The reason that you want to use the `--resume` flag is that the container already exists, so only attaching to the container is required.

### Flags and advanced use-cases

```sh
./run_tappas_docker.sh [options] 
Options:
  --help               Show this help
  --tappas-image       Path to tappas image
  --resume             Resume an old container
  --container-name     Start a container with a specific name, defaults to hailo_tappas_container
```

#### Use-cases

For building a new container with the default name:

```sh
./run_tappas_docker.sh --tappas-image TAPPAS_IMAGE_PATH
```

For resuming an old container:

```sh
./run_tappas_docker.sh --resume
```

Both of this methods can receive a container name:

```sh
./run_tappas_docker.sh --tappas-image TAPPAS_IMAGE_PATH --container-name CONTAINER_NAME
./run_tappas_docker.sh --resume  --container-name CONTAINER_NAME
```

for example:

```sh
./run_hailort_docker.sh hailo_docker_tappas_3.14.0.tar --container-name hailo_tappas_container
```

## Upgrade Version

To upgrade, run: [Running TAPPAS container from pre-built Docker image](#running-tappas-container-from-pre-built-docker-image)

> Note: TAPPAS requires a specific HailoRT version, therefore, you will might need to upgrade HailoRT version as well. To check which HailoRT version is supported, please visit [This Link](../../README.md#prerequisites)

## Troubleshooting

### Hailo containers are taking to much space

Creating new docker containers with `--override` does not assure that the directory of cached images and containers is cleaned.
to prevent your system to ran out of memory and clean /var/lib/docker run `docker system prune` from time to time.

### Running out of disk space

**Change Docker root directory** - By default, Docker stores most of its data inside the `/var/lib/docker` directory on Linux systems. There may come a time when you want to move this storage space to a new location. For example, the most obvious reason might be that you’re running out of disk space.

Firstly, stop the Docker from running

```sh
$ sudo systemctl stop docker.service
$ sudo systemctl stop docker.socket
```

Next, we need to edit the `/lib/systemd/system/docker.service` file

```sh
$ sudo vim /lib/systemd/system/docker.service
```

The line we need to edit looks like this:

```sh
ExecStart=/usr/bin/dockerd -H fd://
```

Edit the line by putting a `-g` and the new desired location of your Docker directory. When you’re done making this change, you can save and exit the file.

```sh
ExecStart=/usr/bin/dockerd -g /new/path/docker -H fd://
```

![image](../resources/change_docker_path.png)

If you haven’t already, create the new directory where you plan to move your Docker files to.

```sh
$ sudo mkdir -p /new/path/docker
```

Next, reload the systemd configuration for Docker, since we made changes earlier. Then, we can start Docker.

```sh
$ sudo systemctl daemon-reload
$ sudo systemctl start docker
```

Just to make sure that it worked, run the ps command to make sure that the Docker service is utilizing the new directory location.

```sh
$ ps aux | grep -i docker | grep -v grep
```

![image](../resources/ps_after_change.png)
