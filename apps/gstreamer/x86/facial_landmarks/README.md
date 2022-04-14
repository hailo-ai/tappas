# Facial Landmarks Pipeline

## Table of Contents
- [Facial Landmarks Pipeline](#facial-landmarks-pipeline)
  - [Table of Contents](#table-of-contents)
  - [Overview:](#overview)
  - [Options](#options)
  - [Run](#run)
  - [Model](#model)
  - [How it works](#how-it-works)

## Overview:
`facial_landmarks.sh` demonstrates facial landmarking on one video file source and verifies Hailo’s configuration.
 This is done by running a `single-stream facial landmarking pipeline` on top of GStreamer using the Hailo-8 device.

## Options
```sh
./face_landmarks.sh
```
* `--input` is an optional flag, a path to the video displayed (default is faces_120_120.mp4).
* `--show-fps`  is an optional flag that enables printing FPS on screen. 
* `--print-gst-launch` is a flag that prints the ready gst-launch command without running it"
## Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/x86/face_landmarks/
./face_landmarks.sh 
```
The output should look like:
<div align="center">
    <img src="readme_resources/face_landmarks_pipeline.gif" width="600px" height="500px"/> 
</div>

## Model
- `tddfa_mobilenet_v1` in resolution of 120X120X3 - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/tddfa_mobilenet_v1.yaml.


## How it works
This section is optional and provides a drill-down into the implementation of the `facial landmarks` app with a focus on explaining the `GStreamer` pipeline.
This setction uses `tddfa_mobilenet_v1` as an example network so network input width, height, hef name, are set accordingly.

```sh
gst-launch-1.0 \
    $source_element ! decodebin ! \
    videoscale ! video/x-raw,pixel-aspect-ratio=1/1 ! videoconvert ! \
    queue leaky=no max_size_buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path debug=False is-active=true qos=false ! \
    queue leaky=no max_size_buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter2 so-path=$POSTPROCESS_SO qos=false ! \
    queue leaky=no max_size_buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! \
    videoconvert ! \
    fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=true
```
Let's explain this pipeline section by section:
1. ```sh
    filesrc location=$video_device ! decodebin ! videoconvert !
    ```
    Specifies the location of the video used, then decodes and converts to the required format.
    
2. ```sh
   videoscale ! video/x-raw,pixel-aspect-ratio=1/1 ! \
   ```
   Re-scales the video dimensions to fit the input of the network. In this case it is rescaling the video to 120x120 with the caps negotiation of `hailonet`.

3. ```sh
    queue leaky=no max_size_buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Before sending the frames into `hailonet` element, set a queue so no frame are lost (Read more about queues [here](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html?gi-language=c)
4. ```sh
    hailonet hef-path=$hef_path debug=False is-active=true qos=false ! \
    queue leaky=no max_size_buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Performs the inference on the Hailo-8 device.
5. ```sh
    hailofilter2 so-path=$POSTPROCESS_SO qos=false ! \
    queue leaky=no max_size_buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! \
    ```
    Performs a given post-process, in that case, performs `tddfa_mobilenet_v1` post-process and then landmarks drawing.
6. ```sh
    videoconvert ! \
    fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=true
    ```
    Apply the final convert to let GStreamer utilize the format required by the `fpsdisplaysink` element.

> ***NOTE***: Additional details about the pipeline provided in further examples