# Detection Pipeline

## Overview

`detection.sh` demonstrates detection on one video file source and verifies Hailo’s configuration.
 This is done by running a `single-stream object detection pipeline` on top of GStreamer using the Hailo-8 device.

## Options

```sh
./detection.sh [--input FILL-ME]
```

- `--input` is an optional flag, a path to the video file displayed (default is detection.mp4).
- `--network`   is a flag that sets which network to use. choose from [yolov5, mobilenet_ssd], default is yolov5.
this will set the hef file to use, the `hailofilter` function to use and the scales of the frame to match the width and heigh input dimensions of the network.
- `--show-fps`  is an optional flag that enables printing FPS on screen.
- `--print-gst-launch` is a flag that prints the ready gst-launch command without running it
- `--print-device-stats` Print the power and temperature measured

## Supported Networks

- 'yolov5' - <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m.yaml>
- 'mobilenet_ssd' - <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/ssd_mobilenet_v1.yaml>

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

This app is based on our [single network pipeline template](../../../../docs/pipelines/single_network.md)

With small modifications:

1. Use decode elements instead of `decodebin`
2. Increase the number of threads on the `videoconvert`