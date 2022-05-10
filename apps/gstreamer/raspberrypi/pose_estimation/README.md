# Pose Estimation Pipeline

## Overview

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

This app is based on our [single network pipeline template](../../../../docs/pipelines/single_network.md)

With small modifications:

1. Use decode elements instead of `decodebin`
2. Increase the number of threads on the `videoconvert`