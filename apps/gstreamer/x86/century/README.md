# Century Pipeline

## Table of Contents
- [Century Pipeline](#century-pipeline)
  - [Table of Contents](#table-of-contents)
  - [Overview:](#overview)
  - [Options](#options)
  - [Supported Networks:](#supported-networks)
  - [Run](#run)
  - [How it works](#how-it-works)

## Overview:
`century.sh` demonstrates detection on one video file source over multiple Hailo-8 devices, either using the [Century platform](https://hailo.ai/product-hailo/hailo-8-century-evaluation-platform/), or other multi device configurations (i.e., multiple M.2 modules connected directly to the same host). While this application defaults to 4 devices, any number of Hailo-8 devices are supported.  
This pipeline runs the detection network YoloX.

## Options
```sh
./century.sh [--input FILL-ME]
```
* `--input` is an optional flag, a path to the video displayed (default is detection.mp4).
* `--show-fps`  is an optional flag that enables printing FPS to the console. 
* `--print-gst-launch` is a flag that prints the ready gst-launch command without running it
* `--device-count` is an optional flag that sets the number of devices to use (default 4)

## Supported Networks:

* 'yolox_l' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolox_l_leaky.yaml

## Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/x86/century
./century.sh
```
The output should look like:
<div align="center">
    <img src="readme_resources/pipeline_run.gif" width="600px" height="500px"/> 
</div>

## How it works
This section is optional and provides a drill-down into the implementation of the `century` app with a focus on explaining the `GStreamer` pipeline.
This section uses `yolox` as an example network so network input width, height, and hef name are set accordingly.

```
gst-launch-1.0 \
    filesrc location=$video_device ! decodebin ! videoconvert ! \
    videoscale ! video/x-raw, pixel-aspect-ratio=1/1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path device-count=$device_count is-active=true ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter function-name=yolox so-path=$POSTPROCESS_SO qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay qos=false ! 
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert ! \
    fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=false text-overlay=false ${additonal_parameters}
```
Let's explain this pipeline section by section:
1. ```
    filesrc location=$video_device ! decodebin ! videoconvert !
    ```
    Specifies the location of the video used, then decodes and converts to the required format.
    
2. ```
   videoscale ! video/x-raw,pixel-aspect-ratio=1/1 ! \
   ```
   Re-scale the video dimensions to fit the input of the network. In this case it is rescaling the video to 640x640 with the caps negotiation of `hailonet`.

3. ```
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Before sending the frames into the `hailonet` element, set a queue so no frames are lost (Read more about queues [here](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html?gi-language=c))
4. ```
    hailonet hef-path=$hef_path device-count=$device_count is-active=true ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Performs the inference on the Hailo-8 devices. The number of physical devices to utilize is set via the device-count property (defaults to 4 in this app). You can adjust this number when running the application with the `--device-count` option.
5. ```
    hailofilter2 function-name=yolox so-path=$POSTPROCESS_SO qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    `hailofilter` performs a given post-process. In this case the `YoloX` post-process.
6. ```
    hailooverlay qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    `hailooverlay` draws the post-processed boxes on the frame.

7. ```
    videoconvert ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=false text-overlay=false ${additonal_parameters}
    ```
    Apply the final image format conversion to the format required by the `fpsdisplaysink` element and then display the frame on screen.
