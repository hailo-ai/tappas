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
* `--input` is an optional flag, a path to the video file displayed (default is detection.mp4).
* `--network`   is a flag that sets which network to use. choose from [yolov5, mobilenet_ssd], default is yolov5.
this will set the hef file to use, the `hailofilter` function to use and the scales of the frame to match the width and heigh input dimensions of the network.
* `--show-fps`  is an optional flag that enables printing FPS on screen. 
* `--print-gst-launch` is a flag that prints the ready gst-launch command without running it
* `--print-device-stats` Print the power and temperature measured

## Supported Networks:

* 'yolov5' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m.yaml
* 'mobilenet_ssd' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/ssd_mobilenet_v1.yaml

## Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/raspberrypi/detection
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
    gst-launch-1.0 ${stats_element} \
    filesrc location=$input_source name=src_0 ! qtdemux ! h264parse ! avdec_h264 ! \
    videoscale n-threads=8 ! video/x-raw, pixel-aspect-ratio=1/1 ! videoconvert n-threads=8 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true qos=false batch-size=$batch_size ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter2 function-name=$network_name so-path=$postprocess_so qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! \
    videoconvert n-threads=8 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display sync=$sync_pipeline text-overlay=false ${additonal_parameters}"
```
Let's explain this pipeline section by section:
1.  ```
    filesrc location=$input_source name=src_0 ! qtdemux ! h264parse ! avdec_h264 ! \
    ```
    Specifies the location of the video used, then decodes it. 
2.  ```
    filesrc location=$input_source name=src_0 ! qtdemux ! h264parse ! avdec_h264 ! \
    ```
    Specifies the location of the video used, then decodes it. 
    
3. ```
   videoscale n-threads=8 ! video/x-raw, pixel-aspect-ratio=1/1 ! videoconvert n-threads=8 ! \
   ```
   Re-scale the video dimensions to fit the input of the network. In this case it is rescaling the video to 640x640 with the caps negotiation of `hailonet`. Then convert it to the required format. 

4.  ```
    queue ! \
    ```
    Before sending the frames into the `hailonet` element, set a queue so no frames are lost (Read more about queues [here](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html?gi-language=c))

5.  ```
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true qos=false batch-size=$batch_size ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Performs the inference on the Hailo-8 device.
6.  ```
    hailofilter2 function-name=$network_name so-path=$postprocess_so qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Each `hailofilter` performs a given post-process. In this case performs the `Yolov5m` post-process.

7.  ```
    hailooverlay ! \
    ```
    Performs the drawing. 

8.  ```
    videoconvert n-threads=8 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display sync=$sync_pipeline text-overlay=false ${additonal_parameters}"
    ```
    Apply the final convert to let GStreamer utilize the format required by the `fpsdisplaysink` element

> ***NOTE***: Additional details about the pipeline provided in further examples
