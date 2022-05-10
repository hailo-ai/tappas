# Segmentation Pipelines

## Semantic Segmentation

`semantic_segmentation.sh` demonstrates semantic segmentation on one video file source.
 This is done by running a `single-stream object semantic segmentation pipeline` on top of GStreamer using the Hailo-8 device.

## Options

```sh
./semantic_segmentation.sh [--input FILL-ME]
```

- `--input` is an optional flag, a path to the video displayed (default is full_mov_slow.mp4).
- `--print-gst-launch` is a flag that prints the ready gst-launch command without running it
- `--show-fps`  is an optional flag that enables printing FPS on screen

## Supported Network

- 'fcn8_resnet_v1_18' - <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/fcn8_resnet_v1_18.yaml>

## Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/x86/segmentation
./semantic_segmentation.sh
```

The output should look like:
<div align="center">
    <img src="readme_resources/pipeline_run.gif" width="600px" height="500px"/>
</div>

## Model

- `fcn8_resnet_v1_18` in resolution of 1920x1024x3.
- Numeric accuracy 65.18mIOU.
- Pre trained on cityscapes using GlounCV and a resnet-18-
FCN8 architecture.

## How it works?

This app is based on our [single network pipeline template](../../../../docs/pipelines/single_network.md)
