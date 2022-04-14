# Pose Estimation Pipeline

## Table of Contents
- [Pose Estimation Pipeline](#pose-estimation-pipeline)
  - [Table of Contents](#table-of-contents)
  - [Overview:](#overview)
  - [Options](#options)
  - [Run](#run)
  - [How it works](#how-it-works)

## Overview:
`hailo_pose_estimation.sh` demonstrates human pose estimation on one video file source and verifies Hailo’s configuration.
 This is done by running a `single-stream pose estimation pipeline` on top of GStreamer using the Hailo-8 device.

## Options
```sh
./hailo_pose_estimation.sh [--input FILL-ME]
```
* `--input` is an optional flag, a path to the video displayed (default is detection.mp4).
* `--show-fps`  is an optional flag that enables printing FPS on screen. 
* `--network`   Set network to use. choose from [centerpose, centerpose_416], default is centerpose
* `--print-gst-launch` is a flag that prints the ready gst-launch command without running it"
## Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/raspberrypi/pose_estimation
./hailo_pose_estimation.sh
```
The output should look like:
<div align="center">
    <img src="readme_resources/centerpose.gif" width="640px" height="360px"/> 
</div>

## How it works
This section is optional and provides a drill-down into the implementation of the `pose_estimation` app with a focus on explaining the `GStreamer` pipeline.
This section uses `centerpose_regnetx_1.6gf_fpn` as an example network so network input width, height, and hef name are set accordingly.

```
gst-launch-1.0 \
    filesrc location=$input_source name=src_0 ! qtdemux ! h264parse ! avdec_h264 ! \
    videoscale n-threads=8 ! video/x-raw, pixel-aspect-ratio=1/1 ! videoconvert n-threads=8 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true qos=false batch-size=1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$postprocess_so qos=false debug=False function-name=$network_name ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$draw_so qos=false debug=False ! \
    videoconvert n-threads=8 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display sync=$sync_pipeline text-overlay=false ${additonal_parameters}
```
Let's explain this pipeline section by section:
1. ```
    filesrc location=$input_source name=src_0 ! qtdemux ! h264parse ! avdec_h264 ! \
    videoscale n-threads=8 ! video/x-raw, pixel-aspect-ratio=1/1 ! videoconvert n-threads=8 ! \
    ```
    Specifies the location of the video used, then decodes the data.
    Re-scale the video dimensions to fit the input of the network, In this case it is rescaling the video to 640x640 with the caps negotiation of `hailonet`. 
    Converts to the required format using 8 threads for acceleration.

2. ```
    queue leaky=no max-size-buffers=30 max-size-bytes=0max-size-time=0 ! \
    ```
    Before sending the frames into the `hailonet` element, set a queue so no frames are lost (Read more about queues [here](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html?gi-language=c))
3. ```
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true qos=false batch-size=1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Performs the inference on the Hailo-8 device.
4. ```
    hailofilter so-path=$postprocess_so qos=false debug=False function-name=$network_name ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$draw_so qos=false debug=False ! \
    ```
    Each `hailofilter` performs a given post-process. In this case the first performs the `centerpose` post-process and the second performs box and skeleton drawing.
5. ```
    videoconvert n-threads=8 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display sync=$sync_pipeline text-overlay=false ${additonal_parameters}
    ```
    Apply the final convert to let GStreamer utilize the format required by the `fpsdisplaysink` element

> ***NOTE***: Additional details about the pipeline provided in further examples
