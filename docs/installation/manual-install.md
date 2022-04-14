# Manual installation

A guide about how to install our required components manually.

> **_NOTE:_**  Only Ubuntu 18.04 and 20.04 is supported

## Hailort install

First you will need to install HailoRT + HailoRT PCIe driver, follow HailoRT install guide for further instructions.
And then [Make sure that HailoRT works](./verify_hailoRT.md)

## Preparations

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

2. change directory to TAPPAS, make directory named "hailort" and clone `HailoRT` sources

    ```sh
    cd `tappas_VERSION`
    mkdir hailort
    git clone https://github.com/hailo-ai/hailort.git hailort/sources
    ```

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