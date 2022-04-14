# Yocto

This section will guide through the integration of Hailo's Yocto layer's into your own Yocto
environment.

Two layers are provided by Hailo, the first one is `meta-hailo` which compiles the `HailoRT` sources, and the second one is `meta-hailo-tappas` which compiles the `TAPPAS` sources.

`meta-hailo-tappas` is a layer that based un-top of `meta-hailo` that adds `TAPPAS` recipes.

The layers are stored in [Meta-Hailo Github](https://github.com/hailo-ai/meta-hailo.git), with branch for each supported yocto release:

- Zeus (kernel 5.4.24)
- Dunfell (kernel 5.4.85)
- Gatesgarth (kernel 5.10.9)
- Hardknott (kernel  5.10.72) (preview)

## Setup

### HailoRT

Add the following to your image in your `conf/bblayers.conf`:

```sh
BBLAYERS += " ${BSPDIR}/sources/meta-hailo/meta-hailo-accelerator \
              ${BSPDIR}/sources/meta-hailo/meta-hailo-libhailort"
```

Add the recipes to your image in your ``conf/local.conf``:

```sh
IMAGE_INSTALL_append = "hailo-firmware libhailort hailortcli hailo-pci libgsthailo"
```

### Tappas

Add the following to your image in your `conf/bblayers.conf`:

```sh
BBLAYERS += "${BSPDIR}/sources/meta-hailo/meta-hailo-tappas"
```

Add the following to your image in your `conf/local.conf`:

```sh
IMAGE_INSTALL_append = "libgsthailotools tappas-apps"
```

## Build your image

Run bitbake and build your image. After the build successfully finished, burn the Image to your embedded device.

### Copy the ARM apps

From within the x86 container, copy the `arm` apps into the embedded device using your preferred way of copying

### Validating the integration's success

Make sure that the following conditions have been met on the target device:

- Running `hailortcli fw-control identify` prints the right configurations

- Running `gst-inspect-1.0 | grep hailo` returns hailo elements:

  ```sh
  hailo:  hailonet: hailonet element
  hailodevicestats: hailodevicestats element
  ```

- Running `gst-inspect-1.0 | grep hailotools` returns hailotools elements:

  ```sh
  hailotools: hailomuxer: Muxer pipe fitting
  hailotools: hailofilter: Hailo postprocessing and drawing element
  ```

- post-processes shared object files exists at `/usr/lib/hailo-post-processes`

## Recipes

### libgsthailo

Hailo's GStreamer plugin for running inference on the hailo8 chip. Depends on `libhailort` and GStreamer.

The recipe compiles and copies the `libgsthailo.so` file to `/usr/lib/gstreamer-1.0` on the target device's
root file system, make it loadable by GStreamer as a plugin.

### libgsthailotools

Hailo's TAPPAS gstreamer elements and post-processes. Depends on `libgsthailo` and GStreamer.
the source files located in the TAPPAS release under `core/hailo/gstreamer`.
The recipe compiles with meson and copies the `libgsthailotools.so` file to `/usr/lib/gstreamer-1.0`
and the post processes to `/usr/lib/hailo-post-processes` on the target device's root file system.

### tappas-apps

Hailo's TAPPAS embedded application recipe, including GStreamer based detection app.
The recipe copies the app script and the hef file to /home/root/apps/detection.
To run the app enter /home/root/apps/detection and run:
```sh
./detection.sh -i /dev/<video device>
```