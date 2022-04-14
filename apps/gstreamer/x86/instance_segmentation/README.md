# Instance Segmentation Pipeline

## Table of Contents
- [Instance Segmentation Pipeline](#instance-segmentation-pipeline)
  - [Table of Contents](#table-of-contents)
  - [Overview:](#overview)
  - [Options](#options)
  - [Run](#run)
  - [How it works](#how-it-works)

## Overview:
`instance_segmentation.sh` demonstrates instance segmentation on one video file source and verifies Hailo’s configuration.
 This is done by running a `single-stream instance segmentation pipeline` on top of GStreamer using the Hailo-8 device.

## Options
```sh
./instance_segmentation.sh [--input FILL-ME]
```
* `--input` is an optional flag, a path to the video displayed (default is detection.mp4).
* `--show-fps`  is an optional flag that enables printing FPS on screen. 
* `--print-gst-launch` is a flag that prints the ready gst-launch command without running it"
## Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/x86/instance_segmentation
./instance_segmentation.sh
```
The output should look like:
<div align="center">
    <img src="readme_resources/instance_segmentation.gif" width="640px" height="360px"/> 
</div>

## How it works
This section is optional and provides a drill-down into the implementation of the `instance_segmentation` app with a focus on explaining the `GStreamer` pipeline.
This section uses `yolact_regnetx_800mf_fpn_20classes` as an example network so network input width, height, and hef name are set accordingly.

```
gst-launch-1.0 \
    filesrc location=$video_device ! decodebin ! videoconvert ! \
    videoscale ! video/x-raw,width=512,height=512,pixel-aspect-ratio=1/1 ! \
    queue queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path debug=False is-active=true qos=false batch-size=8 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$POSTPROCESS_SO qos=false debug=False ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$DRAW_POSTPROCESS_SO qos=false debug=False  ! \
    videoconvert ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display sync=true text-overlay=false ${additonal_parameters}
```
Let's explain this pipeline section by section:
1. ```
    filesrc location=$video_device ! decodebin ! videoconvert !
    ```
    Specifies the location of the video used, then decodes and converts to the required format.
    
2. ```
   videoscale ! video/x-raw,pixel-aspect-ratio=1/1 ! \
   ```
   Re-scale the video dimensions to fit the input of the network. In this case it is rescaling the video to 512x512 with the caps negotiation of `hailonet`.

3. ```
    queue queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Before sending the frames into the `hailonet` element, set a queue so no frames are lost (Read more about queues [here](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html?gi-language=c))
4. ```
    hailonet hef-path=$hef_path debug=False is-active=true qos=false batch-size=8 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Performs the inference on the Hailo-8 device.
5. ```
    hailofilter so-path=$POSTPROCESS_SO qos=false debug=False ! \
    queue name=hailo_draw0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$DRAW_POSTPROCESS_SO qos=false debug=False  ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Each `hailofilter` performs a given post-process. In this case the first performs the `yolact` post-process and the second performs box and segmentation mask drawing.
6. ```
    videoconvert ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display sync=true text-overlay=false ${additonal_parameters}
    ```
    Apply the final convert to let GStreamer utilize the format required by the `fpsdisplaysink` element

> ***NOTE***: Additional details about the pipeline provided in further examples