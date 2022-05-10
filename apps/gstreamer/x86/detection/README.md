# Detection Pipeline

## Overview

`detection.sh` demonstrates detection on one video file source and verifies Hailo’s configuration.
 This is done by running a `single-stream object detection pipeline` on top of GStreamer using the Hailo-8 device.

## Options

```sh
./detection.sh [--input FILL-ME]
```

* `--network`   is an optional flag that sets which network to use. Choose from [yolov3, yolov4, yolov5, mobilenet_ssd], default is yolov5.
This will set which `hef file` to use, the corresponding `hailofilter` function, and the scaling of the frame to match the width/height input dimensions of the network.
- `--input` is an optional flag, a path to the video displayed (default is detection.mp4).
- `--show-fps`  is an optional flag that enables printing FPS on screen.
- `--print-gst-launch` is a flag that prints the ready gst-launch command without running it.
- `--print-device-stats` Print the power and temperature measured on the Hailo device.

## Supported Networks

- 'yolov5' - <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m.yaml>
- 'yolov4' - <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov4_leaky.yaml>
- 'yolov3' - <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov3_gluon.yaml>
- 'mobilenet_ssd' - <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/ssd_mobilenet_v1.yaml>

## Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/x86/detection
./detection.sh
```

The output should look like:
<div align="center">
    <img src="readme_resources/detection_run.png" width="600px" height="500px"/>
</div>

## How it works?

This app is based on our [single network pipeline template](../../../../docs/pipelines/single_network.md)