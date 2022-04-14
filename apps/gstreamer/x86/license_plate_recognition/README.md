# License Plate Recognition

## Table of Contents

- [License Plate Recognition](#license-plate-recognition)
  - [Table of Contents](#table-of-contents)
  - [Overview:](#overview)
  - [Options](#options)
  - [Run](#run)
  - [How the application works](#how-the-application-works)
  - [How the pipeline works](#how-the-pipeline-works)
  - [Pipeline diagram](#pipeline-diagram)

## Overview:
`license_plate_recognition.sh` demonstrates network switching in a complex pipeline with inference based decision making. The overall task is to `detect and track vehicles` in the pipeline and then `detect/extract license plate numbers` from newly tracked instances. The pipeline is comprised of two general contexts: vehicle detection/tracking (`context 0`), and license plate detection/extraction (`context 1`). When enough newly tracked vehicles are detected (single class yolov5m) in `context 0`, the network switch is made to `context 1`, where two networks - tiny-yolov4 based licence plate recognition & lprnet license plate text extraction (OCR) - are joined on the same HEF in one context. Once the license plate is detected and its text extracted, the pipeline updates the `hailotracker` JDE Tracking element upstream with the text for the corresponding vehicle. From there the vehicle is tracked along with its license plate number, and is omitted from being re-inferred in `context 1` on new frames.
This application is a C++ executable that runs a GStreamer application with extra logic applied through probes  

## Options
```sh
./license_plate_recognition.sh [--show-fps]
```
* `--show-fps`  is an optional flag that enables printing FPS on screen. 

## Run

Exporting `TAPPAS_WORKSPACE` environment variable is a must before running the app.

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/x86/license_plate_recognition/license_plate_recognition.sh
```
The output should look like:
<div align="center">
    <img src="readme_resources/lpr_pipeline.gif"/> 
</div>

## How the application works
This section explains the network switch.
The app builds a gstreamer pipeline (that is explained below) and modifies the `is-active` property of its hailonet elements. This is done by applying buffer-probe callbacks on the input pad (sink pad) of each hailonet element. The callbacks perform network switching by blocking a hailonet element when it is time to switch: turning off one hailonet and turning on the other. Before turning a hailonet element on, it has to flush the buffers out of the element, this is done by sending the `flush` signal. [read more about hailonet](../../../../docs/elements/hailo_net.md)

## How the pipeline works
This section is optional and provides a drill-down into the implementation of the `Lice Plate Recognition` app with a focus on explaining the `GStreamer` pipeline.

## Pipeline diagram

<div align="center">
    <img src="readme_resources/lpr_pipeline.png"/> 
</div>

The following elements are the structure of the pipeline:

- `Context 0` - Vehicle Detection and Tracking
  - `filesrc` reads data from a file in the local file system.
  - `decodebin`  constructs a decoding sub-pipeline using available decoders and demuxers 
  - `videoconvert` converts the frame into RGB format.
  - `hailonet`  Performs the inference on the Hailo-8 device.
  Requires the `is-active` property that controls whether this element should be active. In case there are three hailonets in a pipeline and each context uses a different hef-file (so `context 0` in this case) they can't be active at the same time, so when initiallizing the pipeline this instance of hailonet is set to is-active=true and the other two (`context 1`) are set to false.  
  This intance of hailonet performs yolov5m network inference for vehicle detection.  
  [read more about hailonet](../../../../docs/elements/hailo_net.md) 
  - `hailofilter` performs the given postprocess, chosen with the `so-path` property. This instance is in charge of yolo post processing.
  - `hailotracker` performs JDE Tracking using a kalman filter, applying a unique id to tracked vehicles. This element also catches upstream events with license plate text OCR and associates them to their corresponding tracked vehicle.  
  [read more about hailotracker](../../../../docs/elements/hailo_tracker.md) 
  - `tee` splits the piepline into two branches. While one buffer continues the drawing and displaying, the other continues to `context 1` for license plate detection and extraction.
  - `hailooverlay` draws the postprocess results on the frame.
  - `fpsdisplaysink` outputs video onto the screen, and displays the current and average framerate.

<br/>

- `Context 1` - License Plate Detection and Extraction (OCR)
  - `hailocropper` crops vehicle detections from the original full HD image and resizes them to the input size of the following `hailonet` (licence plate detection). The original full HD is also sent through another pad to a `hailoaggregator` downstream. This ensures that all detections are returned to the space of the full HD image. Decision making is also applied here to only pass vehicles that do not already have a trcaked OCR label, meaning, the `hailotracker` has not recieved an upstream event that tags a license plate number to the tracked unique id.  
  [read more about hailocropper](../../../../docs/elements/hailo_cropper.md)
    - `queue` a normal `GStreamer` queue. While the other queues in the pipeline are omittied from the diagram for simplicity, this one is worth noting as it triggers the network switch - when this queue emits the `overrun signal`, it is full so `hailonet` activity is switched to `context 1`. When the queue emits the `underrun signal`, the queue is empty `hailonet` activity is switched to `context 0`.
    - `hailonet` this intance of hailonet performs tiny-yolov4 network inference for license plate detection. When initiallizing the pipeline this instance of hailonet is set to is-active=false.
    - `hailofilter` this instance of hailofilter is in charge of tiny-yolov4 post processing.
  - `hailoaggregator` waits for all crops belonging to the original frame to arrive and merges all metas into their original frame. So, for example, if the upstream `hailocropper` cropped 4 vehicles from the original frame, then this `hailoaggregator` will wait to recieve 4 buffers along witht he original frame.
  [read more about hailoaggregator](../../../../docs/elements/hailo_aggregator.md)
  - `hailocropper` another cropping element, this time the decision making is an image quality estimator - if the license plate detection is determined to be too blurry for OCR, then it is dropped. If the detection is not too blurry, then a crop of the license plate is taken from the original full HD image and sent to for OCR inference.
    - `hailonet` this intance of hailonet performs lprnet network inference for license plate text extraction. When initiallizing the pipeline this instance of hailonet is set to is-active=false.
    - `hailofilter` this instance of hailofilter is in charge of OCR post processing.
  - `hailoaggregator` waits for all crops belonging to the original frame to arrive and merges all metas into their original frame.
  - `appsink` captures incoming buffers. From these the ocr text is extracted and sent uupstream as a [GstEvent](https://gstreamer.freedesktop.org/documentation/gstreamer/gstevent.html). These events contain both the OCR postprocess results and the unique tracking id of the vehicle they were extracted from. The event is caught by the `hailotracker` element which updates the corresponding entry in its tracked vehicle database. 

