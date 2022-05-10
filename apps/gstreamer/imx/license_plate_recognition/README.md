# License Plate Recognition

## Table of Contents

- [License Plate Recognition](#license-plate-recognition)
  - [Table of Contents](#table-of-contents)
  - [Overview](#overview)
  - [Run](#run)
  - [How the application works](#how-the-application-works)

## Overview

`license_plate_recognition.sh` demonstrates network switching in a complex pipeline with inference based decision making. The overall task is to `detect and track vehicles` in the pipeline and then `detect/extract license plate numbers` from newly tracked instances. The pipeline is comprised of two general contexts: vehicle detection/tracking (`context 0`), and license plate detection/extraction (`context 1`). When enough newly tracked vehicles are detected (single class yolov5m) in `context 0`, the network switch is made to `context 1`, where two networks - tiny-yolov4 based licence plate recognition & lprnet license plate text extraction (OCR) - are joined on the same HEF in one context. Once the license plate is detected and its text extracted, the pipeline updates the `hailotracker` JDE Tracking element upstream with the text for the corresponding vehicle. From there the vehicle is tracked along with its license plate number, and is omitted from being re-inferred in `context 1` on new frames.
This application is a C++ executable that runs a GStreamer application with extra logic applied through probes  

## Run

```sh
./apps/license_plate_recognition/license_plate_recognition.sh
```

The output should look like:
<div align="center">
    <img src="readme_resources/lpr_pipeline.gif"/>
</div>

## How the application works

This section explains the network switch.
The app builds a gstreamer pipeline (that is explained below) and modifies the `is-active` property of its hailonet elements. This is done by applying buffer-probe callbacks on the input pad (sink pad) of each hailonet element. The callbacks perform network switching by blocking a hailonet element when it is time to switch: turning off one hailonet and turning on the other. Before turning a hailonet element on, it has to flush the buffers out of the element, this is done by sending the `flush` signal. [read more about hailonet](../../../../docs/elements/hailo_net.md)