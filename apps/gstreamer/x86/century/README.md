# Century Pipeline

## Overview

`century.sh` demonstrates detection on one video file source over multiple Hailo-8 devices, either using the [Century platform](https://hailo.ai/product-hailo/hailo-8-century-evaluation-platform/), or other multi device configurations (i.e., multiple M.2 modules connected directly to the same host). While this application defaults to 4 devices, any number of Hailo-8 devices are supported.  
This pipeline runs the detection network YoloX.

## Options

```sh
./century.sh [--input FILL-ME]
```

- `--input` is an optional flag, a path to the video displayed (default is detection.mp4).
- `--show-fps`  is an optional flag that enables printing FPS to the console.
- `--print-gst-launch` is a flag that prints the ready gst-launch command without running it
- `--device-count` is an optional flag that sets the number of devices to use (default 4)

## Supported Networks

- 'yolox_l' - <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolox_l_leaky.yaml>

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

This app is based on our [multi device pipeline template](../../../../docs/pipelines/multi_device.md)