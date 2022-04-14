# Depth Estimation Pipelines

## Table of Contents

- [Depth Estimation Pipelines](#depth-estimation-pipelines)
  - [Table of Contents](#table-of-contents)
  - [Depth Estimation](#depth-estimation)
  - [Options](#options)
  - [Run](#run)
  - [Model](#model)
  - [How it works](#how-it-works)

## Depth Estimation

`depth_estimation.sh` demonstrates depth estimation on one video file source.
This is done by running a `single-stream object depth estimation pipeline` on top of GStreamer using the Hailo-8 device.

## Options

```sh
./depth_estimation.sh [--video-src FILL-ME]
```

- `-i --input` is an optional flag, a path to the video displayed.
- `--print-gst-launch` is a flag that prints the ready gst-launch command without running it
- `--show-fps` is an optional flag that enables printing FPS on screen

## Run

```sh
cd /local/workspace/tappas/apps/gstreamer/x86/depth_estimation
./depth_estimation.sh
```

The output should look like:

<div align="center">
    <img src="readme_resources/pipeline_run.gif" width="600px" height="250px"/> 
</div>

## Model

- `fast_depth` in resolution of 224X224X3.

## How it works

This section is optional and provides a drill-down into the implementation of the `depth estimation` app with a focus on explaining the `GStreamer` pipeline.
This section uses `fast_depth` as an example network so network input width, height, hef name, are set accordingly.

```sh
gst-launch-1.0 \
    $source_element ! queue ! videoconvert ! queue ! \
    tee name=t ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    aspectratiocrop aspect-ratio=1/1 ! queue ! videoscale ! queue ! \
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true qos=false batch-size=1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$draw_so qos=false debug=False ! videoconvert ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert ! fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=false text-overlay=false \
    t. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoscale ! video/x-raw, width=300, height=300 ! queue ! videoconvert ! \
    xvimagesink sync=false ${additonal_parameters}
```

Let's explain this pipeline section by section:

1. ```
   filesrc location=$video_device ! decodebin ! videoconvert !
   ```
   Specifies the location of the video used, then decodes and converts to the required format.
2. ```
    aspectratiocrop aspect-ratio=1/1 ! queue ! videoscale
   ```

   The network used expect no borders, so a crop mechanism is needed.  
   Re-scales the video dimensions to fit the input of the network. In this case it is cropping the video and rescaling the video to 224x224 with the caps negotiation of `hailonet`.

3. ```
   tee name=t ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0
   ```
   Declare a `tee` in order to keep the original resolution. Before sending the frames into `hailonet` element, set a queue so no frames are lost (Read more about queues [here](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html?gi-language=c))
4. ```
   hailonet hef-path=$hef_path debug=False is-active=true qos=false batch-size=1 ! \
   queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
   ```
   Performs the inference on the Hailo-8 device.
5. ```
   hailofilter so-path=$DRAW_POSTPROCESS_SO qos=false debug=False ! \
   ```
   Performs a given draw-process, in this case, performs `fast_depth` depth estimation drawing per pixel.
6. ```
   videoconvert ! \
   fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=true text-overlay=false ${additonal_parameters}
   ```
   Apply the final convert to let GStreamer utilize the format required by the `fpsdisplaysink` element

> **_NOTE_**: Additional details about the pipeline provided in further examples
