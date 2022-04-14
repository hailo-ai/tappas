# Detection and Depth Estimation Pipeline

## Table of Contents

- [Detection and Depth Estimation Pipeline](#detection-and-depth-estimation-pipeline)
  - [Table of Contents](#table-of-contents)
  - [Detection and Depth Estimation](#detection-and-depth-estimation)
  - [Options](#options)
  - [Run](#run)
  - [Model](#model)
  - [How it works](#how-it-works)

## Detection and Depth Estimation

`detection_and_depth_estimation.sh` demonstrates depth estimation and detection on one video file source.
This is done by running two streams on top of GStreamer using one Hailo-8 device with using two `hailonet` elements.

## Options

```sh
./detection_and_depth_estimation.sh [--video-src FILL-ME]
```

- `-i --input` is an optional flag, a path to the video displayed.
- `--print-gst-launch` is a flag that prints the ready gst-launch command without running it
- `--show-fps` is an optional flag that enables printing FPS on screen

## Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/raspberrypi/multinetworks_parallel/
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
    $source_element ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert n-threads=8 !
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    tee name=t ! \
    aspectratiocrop aspect-ratio=1/1 ! \
    queue ! videoscale n-threads=8 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true net-name=$depth_estimation_net_name qos=false batch-size=1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$depth_estimation_draw_so qos=false debug=False ! videoconvert n-threads=8 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display sync=false text-overlay=false \
    t. ! \
    videoscale n-threads=8 ! queue ! \
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true net-name=$detection_net_name qos=false batch-size=1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter2 so-path=$detection_post_so function-name=mobilenet_ssd_merged qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! videoconvert n-threads=8 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display2 sync=false text-overlay=false ${additonal_parameters} "
```

Let's explain this pipeline section by section:
 
1.  ```
    filesrc location=$video_source name=src_0 ! qtdemux ! h264parse ! avdec_h264 \
    ```
    Specifies the location of the video used and then decodes
2.  ``` 
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Before sending the frames into `hailonet` element, set a queue so no frames are lost     (Read more about queues [here](https://gstreamer.freedesktop.org/documentation/ coreelements/queue.html?gi-language=c))
3.  ```
    videoconvert n-threads=8 !
    ```
    converts to the required format.
4.  ```
    tee name=t !
    ```

    Split into two threads - one for mobilenet_ssd and   the other for fast_depth.

5. ```
    aspectratiocrop aspect-ratio=1/1 ! videoscale n-threads=8 ! \
   ```

    Re-scales the video dimensions to fit the input of the network using 8 threads for acceleration. In this case it is cropping the video and rescaling the video to 224x224 with the caps negotiation of `hailonet`. 
   

6.  ```
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true net-name=$depth_estimation_net_name qos=false batch-size=1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```

    Performs the inference on the Hailo-8 device.

   > **_NOTE_**: We pre define the input and the output layers of each network, giving the net-name argument.

6.  ```
    hailofilter so-path=$DRAW_POSTPROCESS_SO qos=false debug=False ! \
    ```
    Performs a given draw-process, in this case, performs `fast_depth` depth estimation drawing per pixel.

7.  ```
    videoconvert n-threads=8 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display sync=false text-overlay=false \
    ```
    Apply the final convert to let GStreamer utilize the format required by the `fpsdisplaysink` element

8.  ```
    t. ! \
    ```
    beggining of another split of the tee

9.  ```
    videoscale n-threads=8 !        
    ```
    Re-scales the video dimensions to fit the input of the network using 8 threads for acceleration.

10. ```
    hailonet hef-path=$hef_path device-id=$hailo_bus_id debug=False is-active=true net-name=$detection_net_name qos=false   batch-size=1 ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    ```
    Performs the inference on the Hailo-8 device.

11. ```
    hailofilter2 so-path=$detection_post_so function-name=mobilenet_ssd_merged qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \

    ```
    Performs a given post-process, in this case - detection post process.

11. ```
    hailooverlay ! \
    ```
    Performs a draw process, based on the meta data of the buffers. this is a newer api (comparing to using hailofilter for drawing).

11. ```
    videoconvert n-threads=8 ! \
    fpsdisplaysink video-sink=ximagesink name=hailo_display2 sync=false text-overlay=false ${additonal_parameters} ! \
    ```
    Apply the final convert to let GStreamer utilize the format required by the `fpsdisplaysink` element


> **_NOTE_**: Additional details about the pipeline provided in further examples
