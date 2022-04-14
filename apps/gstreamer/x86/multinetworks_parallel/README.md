# Overview

This folder contains apps that runs multiple networks in parallel.

# Table of Contents

- [Overview](#overview)
- [Table of Contents](#table-of-contents)
- [Detection and Depth Estimation Pipeline](#detection-and-depth-estimation-pipeline)
  - [Options](#options)
  - [Run](#run)
  - [Model](#model)
  - [How it works](#how-it-works)
  - [Run](#run-1)
  - [Model](#model-1)
  - [How it works](#how-it-works-1)

# Detection and Depth Estimation Pipeline

`detection_and_depth_estimation.sh` demonstrates depth estimation and detection on one video file source.
This is done by running two streams on top of GStreamer using one Hailo-8 device with using two `hailonet` elements.

## Options

```sh
./detection_and_depth_estimation.sh [--input INPUT]
```

- `-i --input` is an optional flag, a path to the video displayed.
- `--print-gst-launch` is a flag that prints the ready gst-launch command without running it
- `--show-fps` is an optional flag that enables printing FPS on screen

## Run

```sh
cd /local/workspace/tappas/apps/gstreamer/x86/multinetworks_parallel
./detection_and_depth_estimation.sh
```

The output should look like:

<div align="center">
    <img src="readme_resources/pipeline_run.gif" width="600px" height="250px"/>
</div>

## Model

- `fast_depth` in resolution of 224X224X3.
- `mobilenet_ssd` in resolution of 300X300X3.

## How it works

This section is optional and provides a drill-down into the implementation of the app with a focus on explaining the `GStreamer` pipeline.
This section uses `fast_depth` as an example network so network input width, height, hef name, are set accordingly.

```sh
gst-launch-1.0 \
    filesrc location=$video_device ! decodebin ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert !
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    tee name=t ! \
    aspectratiocrop aspect-ratio=1/1 ! \
    queue ! videoscale ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true net-name=$depth_estimation_net_name qos=false batch-size=1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$depth_estimation_draw_so qos=false debug=False ! videoconvert ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=false text-overlay=false \
    t. ! \
    videoscale ! queue ! \
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true net-name=$detection_net_name qos=false batch-size=1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter2 so-path=$detection_post_so function-name=mobilenet_ssd_merged qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! videoconvert ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display2 sync=false text-overlay=false ${additonal_parameters} 
```

Let's explain this pipeline section by section:

1. ```
   filesrc location=$video_device ! decodebin ! videoconvert !
   ```

   Specifies the location of the video used, then decodes and converts to the required format.
2.
2. ```
   tee name=t !
   ```

   Split into two threads - one for mobilenet_ssd and the other for fast_depth.

3. ```
    aspectratiocrop aspect-ratio=1/1 ! videoscale \
   ```

   Re-scales the video dimensions to fit the input of the network. In this case it is cropping the video and rescaling the video to 224x224 with the caps negotiation of `hailonet`.

4. ```
   queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
   ```

   Before sending the frames into `hailonet` element, set a queue so no frames are lost (Read more about queues [here](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html?gi-language=c))
5.
5. ```
    hailonet hef-path=$hef_path device-id=$hailo_bus_id is-active=true net-name=$depth_estimation_net_name qos=false batch-size=1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
   ```

   Performs the inference on the Hailo-8 device.

   > **_NOTE_**: We pre define the input and the output layers of each network, giving the net-name argument.

6. ```
   hailofilter so-path=$DRAW_POSTPROCESS_SO qos=false debug=False ! \
   ```

   Performs a given draw-process, in this case, performs `fast_depth` depth estimation drawing per pixel.
7.
7. ```
   videoconvert ! \
   fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=true text-overlay=false ${additonal_parameters}
   ```

   Apply the final convert to let GStreamer utilize the format required by the `fpsdisplaysink` element

# Detection and Pose Estimation Pipeline

`detection_and_pose_estimation.sh` demonstrates pose estimation and detection on one video file source.
This is done by running two streams on top of GStreamer using one Hailo-8 device with using two `hailonet` elements.

## Options

```sh
./detection_and_pose_estimation.sh [--input INPUT]
```

- `-i --input` is an optional flag, a path to the video displayed.
- `--print-gst-launch` is a flag that prints the ready gst-launch command without running it
- `--show-fps` is an optional flag that enables printing FPS on screen

## Run

```sh
cd /local/workspace/tappas/apps/gstreamer/x86/multinetworks_parallel
./detection_and_pose_estimation.sh
```

## Model

- `centerpose` in resolution of 416X416X3.
- `yolov5m` in resolution of 640X640X3.

## How it works

This section is optional and provides a drill-down into the implementation of the app with a focus on explaining the `GStreamer` pipeline.
This section uses `centerpose` as an example network so network input width, height, hef name, are set accordingly.

```sh
gst-launch-1.0 \
    filesrc location=$video_device ! decodebin ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert !
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    tee name=t ! \
    queue ! videoscale ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path batch-size=$batch_size vdevice-key=$vdevice_key is-active=true net-name=$pose_estimation_net_name qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$pose_estimation_post_so  function-name=centerpose_merged qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$pose_estimation_draw_so qos=false ! videoconvert ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display_pose sync=false text-overlay=false \
    t. ! \
    videoscale ! queue ! \
    hailonet hef-path=$hef_path batch-size=$batch_size vdevice-key=$vdevice_key is-active=true net-name=$detection_net_name qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter2 so-path=$detection_post_so function-name=yolov5 qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay qos=false ! videoconvert ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=false text-overlay=false ${additonal_parameters} 
```

Let's explain this pipeline section by section:

1. ```
   filesrc location=$video_device ! decodebin ! videoconvert !
   ```

   Specifies the location of the video used, then decodes and converts to the required format.
2.
2. ```
   tee name=t !
   ```

   Split into two threads - one for yolov5m and the other for centerpose.

3. ```
    videoscale ! \
   ```

   Re-scales the video dimensions to fit the input of the network. In this case it is rescaling the video to 416X416 with the caps negotiation of `hailonet`.

4. ```
   queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
   ```

   Before sending the frames into `hailonet` element, set a queue so no frames are lost (Read more about queues [here](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html?gi-language=c))
5.
5. ```
   hailonet hef-path=$hef_path batch-size=$batch_size vdevice-key=$vdevice_key is-active=true net-name=$pose_estimation_net_name qos=false ! \
   queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
   ```

   Performs the inference on the Hailo-8 device.

   > **_NOTE_**: We pre define vdevice-key to both hailonet's in order to inform we want both networks to run on same device.

6. ```
   hailofilter so-path=$pose_estimation_post_so  function-name=centerpose_merged qos=false ! \
   queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
   hailofilter so-path=$pose_estimation_draw_so qos=false
   ```

   Performs given post-process and draw-process, in this case, performs `centerpose` pose estimation post-processing and drawing.
7.
7. ```
   videoconvert ! \
   fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=true text-overlay=false ${additonal_parameters}
   ```

   Apply the final convert to let GStreamer utilize the format required by the `fpsdisplaysink` element
