# Overview

This folder contains apps that runs multiple networks in parallel.

## Detection and Depth Estimation Pipeline

`detection_and_depth_estimation.sh` demonstrates depth estimation and detection on one video file source.
This is done by running two streams on top of GStreamer using one Hailo-8 device with using two `hailonet` elements.

## Options

```sh
./detection_and_depth_estimation.sh [--input INPUT]
```

- `-i --input` is an optional flag, a path to the video displayed.
- `--print-gst-launch` is a flag that prints the ready gst-launch command without running it
- `--show-fps` is an optional flag that enables printing FPS on screen

## Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/x86/multinetworks_parallel
./detection_and_depth_estimation.sh
```

The output should look like:

<div align="center">
    <img src="readme_resources/pipeline_run.gif" width="600px" height="250px"/>
</div>

## Model

- `fast_depth` in resolution of 224X224X3.
- `mobilenet_ssd` in resolution of 300X300X3.

## How it works

This app is based on our [parallel networks pipeline template](../../../../docs/pipelines/parallel_networks.md)

# Detection and Pose Estimation Pipeline

`detection_and_pose_estimation.sh` demonstrates pose estimation and detection on one video file source.
This is done by running two streams on top of GStreamer using one Hailo-8 device with using two `hailonet` elements.

## Options

```sh
./detection_and_pose_estimation.sh [--input INPUT]
```

- `-i --input` is an optional flag, a path to the video displayed.
- `--print-gst-launch` is a flag that prints the ready gst-launch command without running it
- `--show-fps` is an optional flag that enables printing FPS on screen

## Run

```sh
cd /local/workspace/tappas/apps/gstreamer/x86/multinetworks_parallel
./detection_and_pose_estimation.sh
```

## Model

- `centerpose` in resolution of 416X416X3.
- `yolov5m` in resolution of 640X640X3.

## How it works

This app is based on our [parallel networks pipeline template](../../../../docs/pipelines/parallel_networks.md)