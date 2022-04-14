# Detection Pipeline

## Table of Contents
- [Detection Pipeline](#detection-pipeline)
  - [Table of Contents](#table-of-contents)
  - [Overview:](#overview)
  - [Options](#options)
  - [Supported Networks:](#supported-networks)
  - [Run](#run)
  - [How it works](#how-it-works)

## Overview:
`detection.sh` demonstrates detection on one video file source and verifies Hailo’s configuration.
 This is done by running a `single-stream object detection pipeline` on top of GStreamer using the Hailo-8 device.

## Options
```sh
./detection.sh [--input FILL-ME]
```
* `--network`   is an optional flag that sets which network to use. Choose from [yolov3, yolov4, yolov5, mobilenet_ssd], default is yolov5.
This will set which `hef file` to use, the corresponding `hailofilter` function, and the scaling of the frame to match the width/height input dimensions of the network.
* `--input` is an optional flag, a path to the video displayed (default is detection.mp4).
* `--show-fps`  is an optional flag that enables printing FPS on screen. 
* `--print-gst-launch` is a flag that prints the ready gst-launch command without running it.
* `--print-device-stats` Print the power and temperature measured on the Hailo device.

## Supported Networks:

* 'yolov5' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m.yaml
* 'yolov4' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov4_leaky.yaml
* 'yolov3' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov3_gluon.yaml
* 'mobilenet_ssd' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/ssd_mobilenet_v1.yaml

## Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/x86/detection
./detection.sh
```
The output should look like:
<div align="center">
    <img src="readme_resources/pipeline_run.gif" width="600px" height="500px"/> 
</div>

## How it works
This section is optional and provides a drill-down into the implementation of the `detection` app with a focus on explaining the `GStreamer` pipeline.
This section uses `yolov5` as an example network so network input width, height, and hef name are set accordingly.

```
gst-launch-1.0 \
    filesrc location=$video_device ! decodebin ! videoconvert ! \
    videoscale ! video/x-raw,width=640,height=640,pixel-aspect-ratio=1/1 ! \
    queue ! \
    hailonet hef-path=$hef_path debug=False is-active=true qos=false batch-size=8 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter2 function-name=yolov5 so-path=$POSTPROCESS_SO qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! \
    videoconvert ! \
    fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=true text-overlay=false ${additonal_parameters}
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
    queue ! \
    ```
    Before sending the frames into the `hailonet` element, set a queue so no frames are lost (Read more about queues [here](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html?gi-language=c))
4. ```
    hailonet hef-path=$hef_path debug=False is-active=true qos=false batch-size=8 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Performs the inference on the Hailo-8 device.
5. ```
    hailofilter2 function-name=yolov5 so-path=$POSTPROCESS_SO qos=false ! \
    queue name=hailo_draw0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay  ! \
    ```
    Each [hailofilter2](../../../../docs/elements/hailo_filter2.md) performs a given post-process to extract the detections. In this it performs the `Yolov5m` post-process. The following [hailooverlay](../../../../docs/elements/hailo_overlay.md) element is able to draw standard `HailoObjects` to the buffer.
6. ```
    videoconvert ! \
    fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=true text-overlay=false ${additonal_parameters}
    ```
    Apply the final convert to let GStreamer utilize the format required by the `fpsdisplaysink` element

> ***NOTE***: Additional details about the pipeline provided in further examples

`Pipeline Structure (queues removed for simplicity)`
<div align="center">
    <img src="readme_resources/pipeline_structure.png"/> 
</div>