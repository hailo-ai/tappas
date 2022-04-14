# Segmentation Pipelines

## Table of Contents
- [Segmentation Pipelines](#segmentation-pipelines)
  - [Table of Contents](#table-of-contents)
  - [Semantic Segmentation](#semantic-segmentation)
  - [Options](#options)
  - [Supported Network](#supported-network)
  - [Run](#run)
  - [How it works](#how-it-works)
  - [Model](#model)

## Semantic Segmentation
`semantic_segmentation.sh` demonstrates semantic segmentation on one video file source.
 This is done by running a `single-stream object semantic segmentation pipeline` on top of GStreamer using the Hailo-8 device.

## Options
```sh
./semantic_segmentation.sh [--input FILL-ME]
```
* `--input` is an optional flag, a path to the video displayed (default is full_mov_slow.mp4).
* `--print-gst-launch` is a flag that prints the ready gst-launch command without running it
* `--show-fps`  is an optional flag that enables printing FPS on screen

## Supported Network
* 'fcn8_resnet_v1_18' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/fcn8_resnet_v1_18.yaml

## Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/x86/segmentation
./semantic_segmentation.sh
```
The output should look like:
<div align="center">
    <img src="readme_resources/pipeline_run.gif" width="600px" height="500px"/> 
</div>

## How it works
This section is optional and provides a drill-down into the implementation of the `semantic segmentation` app with a focus on explaining the `GStreamer` pipeline.
This section uses `resnet18_fcn8_fhd` as an example network so network input width, height, hef name, are set accordingly.

## Model
- `fcn8_resnet_v1_18` in resolution of 1920x1024x3.
- Numeric accuracy 65.18mIOU.
- Pre trained on cityscapes using GlounCV and a resnet-18-
FCN8 architecture.

```
gst-launch-1.0 \
    filesrc location=$video_device ! decodebin ! \
    videoscale ! video/x-raw,pixel-aspect-ratio=1/1 ! videoconvert ! \
    queue leaky=no max-size-buffers=13 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path debug=False is-active=true qos=false batch-size=8 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$DRAW_POSTPROCESS_SO qos=false debug=False ! \
    videoconvert ! \
    fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=true text-overlay=false ${additonal_parameters}
```
Let's explain this pipeline section by section:
1. ```
    filesrc location=$video_device ! decodebin ! videoconvert !
    ```
    Specifies the location of the video used, then decodes and converts to the required format.
    
2. ```
   videoscale ! video/x-raw,pixel-aspect-ratio=1/1 ! \
   ```
   Re-scales the video dimensions to fit the input of the network. In this case it is rescaling the video to 1920x1024 with the caps negotiation of `hailonet`.

3. ```
    queue leaky=no max-size-buffers=13 max-size-bytes=0 max-size-time=0 ! \
    ```
    Before sending the frames into `hailonet` element, set a queue so no frames are lost (Read more about queues [here](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html?gi-language=c))
4. ```
    hailonet hef-path=$hef_path debug=False is-active=true qos=false batch-size=8 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Performs the inference on the Hailo-8 device.
5. ```
    hailofilter so-path=$DRAW_POSTPROCESS_SO qos=false debug=False ! \
    ```
    Performs a given draw-process, in this case, performs `resnet18_fcn8_fhd` semantic segmentation drawing per pixel.
6. ```
    videoconvert ! \
    fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=true text-overlay=false ${additonal_parameters}
    ```
    Apply the final convert to let GStreamer utilize the format required by the `fpsdisplaysink` element

> ***NOTE***: Additional details about the pipeline provided in further examples