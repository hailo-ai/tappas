# Classification Pipeline

## Table of Contents
- [Classification Pipeline](#classification-pipeline)
  - [Table of Contents](#table-of-contents)
  - [Classification](#classification)
  - [Options](#options)
  - [Supported Networks:](#supported-networks)
  - [Run](#run)
  - [How it works](#how-it-works)

## Classification
The purpose of `classification.sh` is to demostrate classification on one video file source.
 This is done by running a `single-stream object classification pipeline` on top of GStreamer using the Hailo-8 device.

<img src="readme_resources/pipeline_run.gif">

## Options
```sh
./classification.sh [--input FILL-ME]
```
* `--input` is an optional flag, a path to the video displayed (default is classification_movie.mp4).
* `--show-fps` is a flag that prints the pipeline's fps to the screen.
* `--print-gst-launch` is a flag that prints the ready gst-launch command without running it.

## Supported Networks:
* 'resnet_v1_50' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/resnet_v1_50.yaml
  
## Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/raspberrypi/classification
./classification.sh
```
<!-- The output should look like: -->
<!-- <div align="center">
    <img src="readme_resources/pipeline_run.gif" width="600px" height="500px"/> 
</div> -->

## How it works
This section is optional and provides a drill-down into the implementation of the `classification` app with a focus on explaining the `GStreamer` pipeline.
This section uses `resnet_v1_50` as an example network so network input width, height, and hef name are set accordingly.

```
gst-launch-1.0 \
    filesrc location=$input_source ! qtdemux ! h264parse ! avdec_h264 ! videoconvert n-threads=8 ! \
    tee name=t hailomuxer name=hmux \
    t. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! hmux. \
    t. ! videoscale n-threads=8 ! video/x-raw, pixel-aspect-ratio=1/1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter2 so-path=$postprocess_so qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! hmux. \
    hmux. ! hailooverlay ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert n-threads=8 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display sync=false text-overlay=false ${additonal_parameters}
```
Let's explain this pipeline section by section:
1.  ```
    filesrc location=$input_source ! qtdemux ! h264parse ! avdec_h264 ! videoconvert n-threads=8 ! \
    ```
    Specifies the location of the video used, then decode and convert to the required format using 8 threads for acceleration.

2.  ```
    tee name=t 
    ``` 
    Declare a tee that splits the pipeline into two branches in order to keep the original resolution.
3.  ```
    hailomuxer name=hmux
    ``` 
    Declare a hailomuxer.
4.  ```
    t. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! hmux. \
    ``` 
    A connection between the first split of the tee to the hailomuxer.
5.  ```
    t. ! videoscale n-threads=8 ! video/x-raw, pixel-aspect-ratio=1/1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    The first split of the tee. 
    Re-scale the video dimensions to fit the input of the network. In this case it is rescaling the video to 112X112 with the caps negotiation of `hailonet`. `hailonet` Extracts the needed resolution from the HEF file during the caps negotiation, and makes sure that the needed resolution is passed from previous elements.
    Before sending the frames into `hailonet` element set a queue so no frames are lost (Read more about queues [here](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html?gi-language=c))

4.  ```
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Performs the inference on the Hailo-8 device.
5.  ```
    hailofilter2 so-path=$postprocess_so qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! hmux. \
    ```
    Performs a given post-process, in this case, performs `resnet_v1_50` classification post-process, which is mainly doing top1 on the inference output. Connected to the hailomuxer.

6.  ```
    hmux. ! hailooverlay ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    A connection between the hailomuxer output to the hailooverlay element.
    Performs classification draw-process. 
7.  ```
    videoconvert n-threads=8 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display sync=false text-overlay=false ${additonal_parameters}
    ```
    Apply the final convert to let GStreamer utilize the format required by the `fpsdisplaysink` element.
