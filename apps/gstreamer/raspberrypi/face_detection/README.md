# Face Detection Pipeline

## Table of Contents

- [Face Detection Pipeline](#face-detection-pipeline)
  - [Table of Contents](#table-of-contents)
  - [Overview:](#overview)
  - [Options](#options)
  - [Supported Networks](#supported-networks)
  - [Run](#run)
  - [How it works](#how-it-works)

## Overview:

The purpose of `face_detection.sh` is to demostrate face detection on one video file source and to verify Hailo’s configuration.
This is done by running a `single-stream face detection pipeline` on top of GStreamer using the Hailo-8 device.

## Options

```sh
/face_detection.sh
```

- `--input` is an optional flag, a path to the video displayed (default is face_detection.mp4).
- `--show-fps` is an optional flag that enables printing FPS on screen.
- `--print-gst-launch` is a flag that prints the ready gst-launch command without running it"

## Supported Networks

- 'liteface' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/lightface_slim.yaml

## Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/raspberrypi/face_detection/
./face_detection.sh
```

The output should look like:

<div align="center">
    <img src="readme_resources/face_detection_pipeline.gif" width="320px" height="275px"/> 
</div>

## How it works

This section is optional and provides a drill-down into the implementation of the `face detection` app with a focus on explaining the `GStreamer` pipeline.
This setction uses `lightface_slim` as an example network so network input width, height, hef name, are set accordingly.

```sh
gst-launch-1.0 \
    filesrc location=$input_source name=src_0 ! qtdemux ! h264parse ! avdec_h264 ! videoconvert n-threads=8 ! tee name=t hailomuxer name=mux \
    t. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! mux. \
    t. ! videoscale n-threads=8 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter2 function-name=$network_name so-path=$postprocess_so qos=false ! mux. \
    mux. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert n-threads=8 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display sync=$sync_pipeline text-overlay=false ${additonal_parameters}
```

Let's explain this pipeline section by section:
1.  ```sh
    filesrc location=$input_source name=src_0 ! qtdemux ! h264parse ! avdec_h264 ! videoconvert n-threads=8 
    ```
    Specifying the location of the video used, then decode and convert to the required format using 8 threads for acceleration.
2.  ```sh 
    tee name=t
    ```
    splitting to two branches of the pipeline

3.  ```sh 
       hailomuxer name=mux 
    ```  
    decleration of hailomuxer element

4. ```sh
    t. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! mux. \
    ```
    a branch of the tee, passing the original frame to the muxer without re-scale 

5.  ```sh
    t. ! videoscale n-threads=8 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```

    Another branch of the tee that will perdorm the inference. 
    Re-scale the video dimensions to fit the input of the network. In this case it is rescaling the video to 320x240 with the caps negotiation of `hailonet`. 

6.  ```sh
    hailonet hef-path=$hef_path device-id=$hailo_bus_id      debug=False is-active=true qos=false ! \
        queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ``` 
    Performs the inference on the Hailo-8 device.
    > **NOTE**: `qos` must be disabled for hailonet since dropping frames may cause these elements to run out of alignment.



7.  ```sh
    hailofilter2 function-name=$network_name so-path=$postprocess_so qos=false ! mux. \
    mux.
    ```
    Each `hailofilter` performs a given post-process. In this case the first performs the `face detection` post-process.
    Enters the mux. 

8.  ```sh
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Performs the Drawing. 

9.  ```sh
    videoconvert n-threads=8 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display sync=$sync_pipeline text-overlay=false ${additonal_parameters}
    ```
    Apply the final convert to let GStreamer utilize the format required by the `fpsdisplaysink` element



> **_NOTE_**: Additional details about the pipeline provided in further examples
