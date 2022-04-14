# Detection and Depth Estimation - networks switch App

## Table of Contents

- [Detection and Depth Estimation - networks switch App](#detection-and-depth-estimation---networks-switch-app)
  - [Table of Contents](#table-of-contents)
  - [Overview:](#overview)
  - [Options](#options)
  - [Run](#run)
  - [How the application works](#how-the-application-works)
  - [How the pipeline works](#how-the-pipeline-works)
  - [Pipeline diagram](#pipeline-diagram)

## Overview:
`detection_and_depth_estimation_networks_switch` demonstrates network switch between two networks: Detection network and Depth estimation network on one video source using one Hailo-8 device. 
The switch is done every frame, so all frames are inferred by both networks. 
This is a C++ executable that runs a GStreamer application with extra logic applied through probes  

## Options
```sh
./detection_and_depth_estimation_networks_switch [--input FILL-ME --show-fps]
```
* `--input` is an optional flag, a path to the video displayed (default is instance_segmentation.mp4).
* `--show-fps`  is an optional flag that enables printing FPS on screen. 

## Run

Exporting `TAPPAS_WORKSPACE` environment variable is a must before running the app.

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/x86/network_switch/detection_and_depth_estimation_networks_switch
```
The output should look like:
<div align="center">
    <img src="readme_resources/networks_switch.gif" width="640px" height="240px"/> 
</div>

## How the application works
This section explains the network switch.
The app builds a gstreamer pipeline (that is explained below) and modifies the `is-active` property of its hailonet elements. This is done by applying buffer-probe callbacks on the input pad (sink pad) of each hailonet element. The callbacks perform network switching by blocking a hailonet element when it is time to switch: turning off one hailonet and turning on the other. Before turning a hailonet element on, it has to flush the buffers out of the element, this is done by sending the `flush` signal. [read more about hailonet](../../../../docs/elements/hailo_net.md)

## How the pipeline works
This section is optional and provides a drill-down into the implementation of the `Detection and Depth Estimation networks switch` app with a focus on explaining the `GStreamer` pipeline.

## Pipeline diagram

<img src="readme_resources/network_switch_diagram.png" alt="pipeline graph" width="1000"/>

The following elements are the structure of the pipeline:

- `filesrc` reads data from a file in the local file system.
- `decodebin`  constructs a decoding sub-pipeline using available decoders and demuxers 
- `videoconvert` converts the frame into RGB format.
- `tee` splits data to multiple pads. After this, the pipeline splits into two branches. 
    - `branch 1` detection
      - `videoscale` resizes a video frame to the input size of hailonet.
      - `identity` dummy element that passes incoming data through unmodified. In this pipeline it is used for catching EOS events before hailonet 1. 
      - `hailonet`  Performs the inference on the Hailo-8 device.
      Requires the `is-active` property that controls whether this element should be active. In case there are two hailonets in a pipeline and each one uses a different hef-file (like in this case) they can't be active at the same time, so when initiallizing the pipeline this instance of hailonet is set to is-active=false and the other one is set to true.  
      This intance of hailonet performs yolov5s network inferencefor detection.[read more about hailonet](../../../../docs/elements/hailo_net.md) 
      - `hailofilter` performs the given postprocess, chosen with the `so-path` property. This instance is in charge of yolo post processing.
      - `hailofilter` this instance is in charge of the yolo drawing process.
      - `videoconvert` converts the frame into negotiated format.
      - `fpsdisplaysink` outputs video onto the screen, and displays the current and average framerate.
        > **NOTE**: `sync=false` property in `fpsdisplaysink` element disables real-time synchronization within the pipeline - it is mandatory in this case to reach the best performance.

  - `branch 2` depth estimation
      - `aspectratiocrop` crops video frames to specified ratio. If it's not included in the pipeline then padding is added to the frames and this behavior is unwanted in case of depth estimation. 
      - `videoscale` same as in branch 1
      - `identity` dummy element that passes incoming data through unmodified. In this pipeline it is used for catching EOS events before hailonet 2. 
      - `hailonet` this intance of hailonet performs fast-depth network inference for depth estimation. When initiallizing the pipeline this instance of hailonet is set to is-active=true.
      - `hailofilter` this instance of hailofilter is in charge of depth-estimation post processing and drawing.
      - `videoconvert` same as in branch 1
      - `fpsdisplaysink` same as in branch 1

> **NOTE**: `queue` elements were not presented for clearness. Queue positions can be observed here:
<img src="/local/workspace/tappas/apps/gstreamer/x86/network_switch/readme_resources/perf.svg" alt="pipeline graph" width="1000"/>

