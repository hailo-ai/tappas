# Python Classification Pipeline

## Table of Contents

- [Python Classification Pipeline](#python-classification-pipeline)
  - [Table of Contents](#table-of-contents)
  - [Classification](#classification)
  - [Options](#options)
  - [Supported Networks:](#supported-networks)
  - [Run](#run)
  - [How it works](#how-it-works)

## Classification

The purpose of `classification.sh` is to demostrate classification on one video file source with python post-processing
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
cd $TAPPAS_WORKSPACE/apps/gstreamer/x86/classification
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
    filesrc location=$input_source ! decodebin ! videoconvert ! \
    videoscale ! video/x-raw, pixel-aspect-ratio=1/1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path debug=False is-active=true qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailopython module=$POSTPROCESS_MODULE qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay qos=false ! \
    videoconvert ! \
    fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=true text-overlay=false ${additonal_parameters}"
```
Let's explain this pipeline section by section:
1.  ```
    filesrc location=$video_device ! decodebin ! videoconvert !
    ```
    Specifies the location of the video used, then decode and convert to the required format.
    
2.  ```
    videoscale ! video/x-raw,pixel-aspect-ratio=1/1 ! \
    ```
    Re-scale the video dimensions to fit the input of the network. In this case it is rescaling the video to 112X112 with the caps negotiation of `hailonet`. `hailonet` Extracts the needed resolution from the HEF file during the caps negotiation, and makes sure that the needed resolution is passed from previous elements.
3.  ```
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Before sending the frames into `hailonet` element set a queue so no frames are lost (Read more about queues [here](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html?gi-language=c))
4.  ```
    hailonet hef-path=$hef_path debug=False is-active=true qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Performs the inference on the Hailo-8 device.
5.  ```
    hailopython so-path=$POSTPROCESS_MODULE qos=false debug=False ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Performs a given post-process, in this case, performs `resnet_v1_50` classification post-process, which is mainly doing top1 on the inference output.
6.  ```
    hailooverlay qos=False ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Performs drawing on the original image, in that case, performs drawing the top1 class name over the image.

7.  ```
    videoconvert ! \
    fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=true text-overlay=false ${additonal_parameters}"
    ```
    Apply the final convert to let GStreamer utilize the format required by the `fpsdisplaysink` element.
