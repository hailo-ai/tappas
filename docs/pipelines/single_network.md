# Single Network Pipeline Structure

<div align="center"><img src="../../resources/single_net_pipeline.jpg"/></div>

This page provides a drill-down into the template of our single network pipelines with a focus on explaining the `GStreamer` pipeline.

## Example pipeline

```sh
gst-launch-1.0 \
    filesrc location=$video_device ! decodebin ! videoconvert ! \
    videoscale ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path is-active=true qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter2 function-name=yolov5 so-path=$POSTPROCESS_SO qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! \
    videoconvert ! \
    fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=true text-overlay=false
```

Let's explain this pipeline section by section:

```
    filesrc location=$video_device ! decodebin ! videoconvert !
```

Specifies the location of the video used, then decodes and converts to the required format.

```
   videoscale ! \
```

   Re-scale the video dimensions to fit the input of the network. The video scale finds out the requires width and height using caps negotiation with `hailonet`.

```
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
```

Before sending the frames into the `hailonet` element, set a queue so no frames are lost (Read more about queues [here](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html?gi-language=c))

```

    hailonet hef-path=$hef_path is-active=true qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
```

Performs the inference on the Hailo-8 device.

```
    hailofilter2 function-name=$POSTPROCESS_NAME so-path=$POSTPROCESS_SO qos=false ! \
    queue name=hailo_draw0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay  ! \
```

[hailofilter2](../elements/hailo_filter2.md) performs a given post-process to extract the objects. The following [hailooverlay](../elements/hailo_overlay.md) element is able to draw standard `HailoObjects` to the buffer.

```
    videoconvert ! \
    fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=true text-overlay=false
```

Apply the final convert to let GStreamer utilize the format required by the `fpsdisplaysink` element

## Example pipeline with resolution preservation

Using this template the source resolution would be preserved, this is an extension to our [Example Pipeline](#example-pipeline)

<div align="center"><img src="../resources/single-net-resolution.png"/></div>


An example for pipelines who preserve the original resolution:

```sh
gst-launch-1.0 \
    $source_element ! videoconvert ! \
    tee name=t hailomuxer name=mux \
    t. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! mux. \
    t. ! videoscale ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailonet hef-path=$hef_path is-active=true qos=false ! \
    queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailofilter2 function-name=$network_name so-path=$postprocess_so qos=false ! mux. \
    mux. ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    hailooverlay ! queue leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
    videoconvert ! \
    fpsdisplaysink video-sink=$video_sink_element name=hailo_display sync=$sync_pipeline text-overlay=false
```

## Example pipeline single network with tiling

<div align="center"><img src="../resources/tiling-example.png"/></div>

Tiling introduces two new elements:

- `hailotilecropper` which splits the frame into tiles by separating the frame into rows and columns
   (given as parameters to the element).
- `hailotileaggregator` which aggregates the cropped tiles and stitches them back to the original resolution.

An example for the pipeline itself could be found on our [Tiling app](../../apps/gstreamer/x86/tiling/README.md).
