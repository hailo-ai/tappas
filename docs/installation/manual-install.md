# Manual installation

Manual installing TAPPAS requires preparations, our recommended method is to begin with `Hailo SW Suite` or `Pre-built Docker image`.
In this guide we instruct how to install our required components manually.

> **_NOTE:_**  Only Ubuntu 18.04 and 20.04 is supported (Not on Raspberry PI )

## Hailort install

First you will need to install [HailoRT](https://github.com/hailo-ai/hailort) + [HailoRT PCIe driver](https://github.com/hailo-ai/hailort-drivers), follow HailoRT install guide for further instructions.
And then [Make sure that HailoRT works](./verify_hailoRT.md)

## Preparations

Download from Hailo developer zone `tappas_VERSION_linux_installer.zip`.

### X86

Unzip `tappas_VERSION_linux_installer.zip`

```sh
unzip tappas_VERSION_linux_installer.zip
```

### Non-X86

1. Unzip TAPPAS and clone HailoRT sources

    ```sh
    unzip tappas_VERSION_linux_installer.zip
    ```

2. change directory to TAPPAS, make directory named `hailort` and clone `HailoRT` sources

    ```sh
    cd `tappas_VERSION`
    mkdir hailort
    git clone https://github.com/hailo-ai/hailort.git hailort/sources
    ```

## Required Packages

Install the following APT packages:

- ffmpeg
- x11-utils
- python3 (pip and setuptools)
- virtualenv
- python-gi-dev
- libgirepository1.0-dev
- gcc-9 and g++-9

To install the above packages, run the following command:
```apt-get install -y ffmpeg x11-utils python3-dev python3-pip python3-setuptools virtualenv python-gi-dev libgirepository1.0-dev gcc-9 g++-9```

The following packages are required as well:

- Opencv4, Installation manual [here](https://linuxize.com/post/how-to-install-opencv-on-ubuntu-18-04/)
- Gstreamer, Installation manual [here](https://gstreamer.freedesktop.org/documentation/installing/on-linux.html?gi-language=c#install-gstreamer-on-ubuntu-or-debian)
- pygobject, Installation manual[here](https://pygobject.readthedocs.io/en/latest/getting_started.html#ubuntu-getting-started)

In case any requirements are missing, a requirements table will be printed when calling manual install.

## Install TAPPAS

Run `install.sh --skip-hailort`

## Upgrade TAPPAS

To Upgrade TAPPAS, first clean GStreamer cache

```sh
rm -rf ~/.cache/gstreamer-1.0/
```

Remove old `libgsthailotools.so`

```sh
rm /usr/lib/$(uname -p)-linux-gnu/gstreamer-1.0/libgsthailotools.so
```

And then, run `install.sh --skip-hailort`
