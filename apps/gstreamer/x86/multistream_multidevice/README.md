# Multi-Stream RTSP object detection and pose estimation Pipeline

## Table of Contents

- [Multi-Stream RTSP object detection and pose estimation Pipeline](#multi-stream-rtsp-object-detection-and-pose-estimation-pipeline)
  - [Table of Contents](#table-of-contents)
  - [Overview](#overview)
  - [Prerequisites](#prerequisites)
  - [Preparations](#preparations)
  - [Run the pipeline](#run-the-pipeline)
  - [Models](#models)
  - [Overview of the pipeline](#overview-of-the-pipeline)

## Overview

This GStreamer pipeline demonstrates object detection on 8 camera streams over RTSP protocol.
This pipeline also demostrates using two hailo8 devices in parallel.

All the streams are processed in parallel through the decode and scale phases, and enter the Hailo devices frame by frame.
**Each** hailo device is in charge of one inference task (one for yolov5 and the other for centerpose)

Afterwards postprocess and drawing phases add the classified object and bounding boxes to each frame. \
The last step is to match each frame back to its respective stream and output all of them to the display.

 `Real Time Streaming Protocol (RTSP)` is a network control protocol designed for use in entertainment and communications systems to control streaming media servers. The protocol is used for establishing and controlling media sessions between endpoint.

## Prerequisites

- TensorPC
- Ubuntu 18.04
- [RTSP](../../../../docs/terminology.md#real-time-streaming-protocol-rtsp) Cameras, We recommend using: [AXIS M10 Network Cameras](https://www.axis.com/products/axis-m1045-lw)
- Two Hailo-8 devices connected via PCIe

- [Multi-Stream RTSP object detection and pose estimation Pipeline](#multi-stream-rtsp-object-detection-and-pose-estimation-pipeline)
  - [Table of Contents](#table-of-contents)
  - [Overview](#overview)
  - [Prerequisites](#prerequisites)
  - [Preparations](#preparations)
  - [Run the pipeline](#run-the-pipeline)
  - [Models](#models)
  - [Overview of the pipeline](#overview-of-the-pipeline)

## Preparations

1. Before running, configuration of the RTSP camera sources is required.
open the `rtsp_detection_and_pose_estimation.sh` in edit mode with your preffered editor.
Configure the eight sources to match your own cameras.

```sh
readonly SRC_0="rtsp://<ip address>/?h264x=4 user-id=<username> user-pw=<password>"
readonly SRC_1="rtsp://<ip address>/?h264x=4 user-id=<username> user-pw=<password>"
etc..
```

## Run the pipeline

```sh
./rtsp_detection_and_pose_estimation.sh
```

1. `--show-fps` prints the fps to the output.
    ><img src="readme_resources/fps.png" alt="fps prints" width="900"/>
2. `--num-of-sources` sets the number of rtsp sources to use by given input. the default and recommended value in this pipeline is 8 sources
3. `--print-gst-launch` prints the ready gst-launch command without running it
4. `--print-devices-stats` prints the power and temperature measured
5. `--debug` uses gst-top to print time and memory consuming elements, saves the results as text and graph
   ><img src="readme_resources/runningdebug.png" alt="debug" width="700"/>


   Open the pipeline_report.txt to view the full report showing all elements, your report should be similar to this:
    ><img src="readme_resources/gst-top.png" alt="gst-top log" width="250"/>

> **NOTE**: When the debug flag is used and the app is running inside of a docker, exit the app by tying `Ctrl+C` in order to save the results. (Due to docker X11 display communication issues)

## Models

`YOLOv5` is a modern object detection architecture that is based on the `YOLOv3` meta-architecture with `CSPNet` backbone. The `YOLOv5` was released on 05/2020 with a very efficient design and SoTA accuracy results on the `COCO benchmark`.

in this pipeline, wer'e using a specific variant of the `YOLOv5` architecture - `yolov5m` that stands for medium sized networks.

- Pre trained and compiled `yolov5m` model stored as .hef file.
- Resolution: 640x640x3
- Full precision accuracy: 41.7mAP
- Dataset: COCO val2017 <https://cocodataset.org/#home>

Enter the git project to read further: <https://github.com/ultralytics/yolov5>
Link to the network yaml in Hailo Model Zoo - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m.yaml

`Centerpose` 
  - Hailo Trained. Based on centerpose architecture with RegnetX_1.6FG
  backbone
  - Resolution: 640x640x3
  - Dataset COCO-Pose

Link to the network yaml in Hailo Model Zoo - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/centerpose_repvgg_a0.yaml

## Overview of the pipeline

The following elements are the structure of the pipeline:

- `rtspsrc` makes a connection to an rtsp server and read the data. used as a src to get the video stream from rtsp-cameras.
- `rtph264depay` extracts h264 video from rtp packets.
    ><img src="readme_resources/rtspsrc.png" alt="rtspsrc" width="500"/>

- `decodebin` video decoding and scaling - in this pipeline, the bin is responsible for decoding h264 format and scaling the frame to 640X640.

    ><img src="readme_resources/decodebin.png" alt="decodebin" width="600"/>
- `videoconvert` converting the frame into RGB format
    ><img src="readme_resources/videoconvert.png" alt="videoconvert" width="600"/>
- `funnel` takes multiple input sinks and outputs one source. an N-to-1 funnel that attaches a streamid to each stream, can later be used to demux back into separate streams. this lets you queue frames from multiple streams to send to the hailo device one at a time.
    ><img src="readme_resources/funnel.png" alt="funnel" width="400"/>

- `tee` duplicates the incomming frame and passes it into two different streams - to perform two different inferences on different chips.
    ><img src="readme_resources/tee.png" alt="hailo send and hailorcv" width="800"/>

- `hailonet` Performs the inference on the Hailo-8 device - configures the chip with the hef and starts hailo's inference process - sets streaming mode and sends the buffers into the chip.
requires the following properties: `hef-path` - points to the compiled yolov5m hef, `qos` that must be set to false - to disable frame drops, and `batch-size`.
[read more about hailonet](../../../../docs/elements/hailo_net.md)

    ><img src="readme_resources/hailosendreceive.png" alt="hailo send and hailorcv" width="800"/>

- `hailofilter` performs given postprocess, chosen with the `so-path` property. in this pipeline, two filters performs yolov5m and centerpose in parallel. 
[read more about hailofilter](../../../../docs/elements/hailo_filter.md)
    ><img src="readme_resources/filters.png" alt="hailofilter" width="800"/>
     >**NOTE**:  If multiple hailofilters are present and dependent on each other, then `qos` must be disabled for each.
      If there is only one hailofilter, then qos may be enabled (although it is still recommended to disable).

- `hailomuxer` muxes 2 similar streams into 1 stream, holding both stream's metadata.
  [read more about hailomuxer](../../../../docs/elements/hailo_muxer.md)
    ><img src="readme_resources/muxer.png">

- `streamiddemux` a reverse to the funnel. It is a 1-to-N demuxer that splits a serialized stream based on stream id to multiple outputs.
- `compositor` composites pictures from multiple sources. handy for multi-stream/tiling like applications, as it lets you input many streams and draw them all together as a grid.
    ><img src="readme_resources/sidandcompositor.png" alt="sid and compositor" width="600"/>

- `fpsdisplaysink` outputs video into the screen, and displays the current and average framerate.
    > **NOTE**: `sync=false` property in `fpsdisplaysink` element disables real-time synchronization with the pipeline - it is mandatory on this case to reach the best performance.

    ><img src="readme_resources/display.png" alt="display" width="300"/>