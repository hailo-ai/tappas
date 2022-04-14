# Face Detection Pipeline

## Table of Contents

- [Face Detection Pipeline](#face-detection-pipeline)
  - [Table of Contents](#table-of-contents)
  - [Overview:](#overview)
  - [Options](#options)
  - [Supported Networks](#supported-networks)
  - [Run](#run)
  - [How it works](#how-it-works)

## Overview:

The purpose of `face_detection.sh` is to demostrate face detection on one video file source and to verify Hailo’s configuration.
This is done by running a `single-stream face detection pipeline` on top of GStreamer using the Hailo-8 device.

## Options

```sh
/face_detection.sh
```

- `--network` is a flag that sets which network to use. choose from [lightface,retinaface], default is lightface.
  this will set the hef file to use, the `hailofilter` function to use, and the scales of the frame to match the width/height input dimensions of the network.
- `--input` is an optional flag, a path to the video displayed (default is face_detection.mp4).
- `--show-fps` is an optional flag that enables printing FPS on screen.
- `--print-gst-launch` is a flag that prints the ready gst-launch command without running it"

## Supported Networks

- 'retinaface' - <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/retinaface_mobilenet_v1.yaml>
- 'liteface' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/lightface_slim.yaml

## Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/x86/face_detection/
./face_detection.sh
```

The output should look like:

<div align="center">
    <img src="readme_resources/face_detection_pipeline.gif" width="320px" height="275px"/> 
</div>

## How it works

This section is optional and provides a drill-down into the implementation of the `face detection` app with a focus on explaining the `GStreamer` pipeline.
This setction uses `lightface_slim` as an example network so network input width, height, hef name, are set accordingly.

```sh
gst-launch-1.0 \
    filesrc location=$input_source ! decodebin ! \
    videoscale ! video/x-raw, pixel-aspect-ratio=1/1 ! videoconvert ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path debug=False is-active=true qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter2 function-name=$network_name so-path=$POSTPROCESS_SO qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert ! \
    fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=false text-overlay=false ${additonal_parameters}
```

Let's explain this pipeline section by section:
1.  ```sh
    filesrc location=$video_device ! decodebin ! videoconvert !
    ```
    Specifying the location of the video used, then decode and convert to the required format.
2.  ```sh
    videoscale ! video/x-raw,pixel-aspect-ratio=1/1 ! \
    ```

    Re-scale the video dimensions to fit the input of the network. In this case it is rescaling the video to 320x240 with the caps negotiation of `hailonet`. #

3.  ```sh
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Before sending the frames into `hailonet` element set a queue so no frame would be lost (Read more about queue [here](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html?gi-language=c))
4.  ```sh
    hailonet hef-path=$hef_path debug=False is-active=true qos=false ! \
    queue leaky=no max_size_buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Sending and receiving the data, separated by a non-leaky queue.
    > **NOTE**: `qos` must be disabled for hailonet since dropping frames may cause these elements to run out of alignment.
5.  ```sh
    hailofilter2 so-path=$POSTPROCESS_SO qos=false ! \
    queue leaky=no max_size_buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay  ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Performs given post-process, in that case, performers lightface_slim post-process and detection box drawing.
6.  ```sh
    videoconvert ! \
    fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=true
    ```
    Apply the final convert to let GStreamer find out the format required by the `fpsdisplaysink` element

> **_NOTE_**: Additional details about the pipeline provided in further examples
