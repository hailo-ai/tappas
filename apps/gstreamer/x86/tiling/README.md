# Tiling Pipeline

## Table of Contents

- [Tiling Pipeline](#tiling-pipeline)
  - [Table of Contents](#table-of-contents)
  - [Single Scale Tiling](#single-scale-tiling)
    - [Model](#model)
    - [Options](#options)
    - [Run](#run)
    - [How it works](#how-it-works)
  - [Multi Scale Tiling](#multi-scale-tiling)
    - [Model](#model-1)
    - [Options](#options-1)
    - [Run](#run-1)
    - [How it works](#how-it-works-1)

## Single Scale Tiling

Single scale tiling FHD Gstreamer pipeline demonstrates splitting each frame into several tiles which are processed independently by `hailonet` element.
This method is especially effective for detecting small objects in high-resolution frames.
This process is separated into 4 elements - 
- `hailotilecropper` which splits the frame into tiles, by seperating the frame into rows and columns 
   (given as parameters to the element).
- `hailonet` which performs an inference on each frame on the Hailo8 device.
- `hailofilter` which performs the postprocess - parses the tensor output to detections.
- `hailotileaggregator` which aggregates the cropped tiles and stitches them back to the original resolution.

### Model

- `ssd_mobilenet_v1_visdrone` in resolution of 300X300 - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/ssd_mobilenet_v1_visdrone.yaml.

The VisDrone dataset consists of only small objects which we can assume are always confined within an single tile. As such it is better suited for running single-scale tiling with little overlap and without additional filtering.


### Options
```sh
./tiling.sh [OPTIONS] [-i INPUT_PATH]
```
* `-i --input` is an optional flag, a path to the video file displayed.
* `--print-gst-launch` prints the ready gst-launch command without running it
* `--show-fps`  optional - enables printing FPS on screen
* `--tiles-x-axis` optional - set number of tiles along x axis (columns)
* `--tiles-y-axis` optional - set number of tiles along y axis (rows)
* `--overlap-x-axis` optional - set overlap in percentage between tiles along x axis (columns)
* `--overlap-y-axis` optional - set overlap in percentage between tiles along y axis (rows)
* `--iou-threshold ` optional - set iou threshold for NMS.
* `--sync-pipeline` optional - set pipeline to sync to video file timing.

### Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/x86/tiling
./tiling.sh
```
The output should look like:
<div align="center">
    <img src="readme_resources/pipeline_run.gif" width="1000px" height="600px"/> 
</div>

### How it works
<div align="center">
    <img src="readme_resources/tiling_pipeline.png" width="840px" height="260px"/> 
    <img src="readme_resources/elements.png" width="320px" height="260px"/>
</div>
<div align="center">

</div>

```sh
filesrc location=$input_source name=src_0 ! decodebin ! videoconvert qos=false
```
`filesrc`  - source of the pipeline reads the video file and decodes it.

```sh
TILE_CROPPER_ELEMENT="hailotilecropper internal-offset=$internal_offset name=cropper \
tiles-along-x-axis=$tiles_along_x_axis tiles-along-y-axis=$tiles_along_y_axis overlap-x-axis=$overlap_x_axis overlap-y-axis=$overlap_y_axis"
```

```sh
$TILE_CROPPER_ELEMENT hailotileaggregator iou-threshold=$iou_threshold name=agg \
cropper. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! agg. \
cropper. ! $DETECTION_PIPELINE ! agg. \
```

`hailotilecropper` splits the pipeline into 2 threads, the first thread passes the original frame, the other thread passes the crops of the original frame created by `hailotilecropper` according to given tiles number per x/y axis and overlap parameters.
The buffers are also scaled to the following `hailonet`, done by caps negotiation.
The `hailotileaggregator` gets the original frame and then knows to wait for all related cropped buffers and add all related metadata on the original frame, sending everything together once aggregated.
It also performs NMS process to merge detections on overlap tiles.
 
 ```sh
 DETECTION_PIPELINE="\
hailonet hef-path=$hef_path device-id=$hailo_bus_id is-active=true qos=false batch-size=1 ! \
queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
hailofilter2 function-name=$postprocess_func_name so-path=$detection_postprocess_so qos=false ! \
queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0"
 ```

focusing on the detection part:
`hailonet` performs inference on the Hailo-8 device running `mobilenet_v1_visdrone.hef` for each tile crop.
`hailofilter` performs the mobilenet postprocess and creates the detection objects to pass through the pipeline.

```sh
agg. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
hailooverlay qos=false ! \
```

`hailotileaggregator` sends the frame forward into the `hailooverlay` which draws the detections over the frame.

```sh
queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! videoconvert ! \

fpsdisplaysink video-sink=xvimagesink name=hailo_display
```

Pipeline ends in the sink to the display.

## Multi Scale Tiling

Multi-scale tiling FHD Gstreamer pipeline demonstrates a case where the video and the training dataset includes objects in different sizes. Dividing the frame to small tiles might miss large objects or “cut" them to small objects.
The solution is to split each frame into number of scales (layers) each includes several tiles.

Multi-scale tiling strategy also allows us to filter the correct detection over several scales.
For example we use 3 sets of tiles at 3 different scales:

- Large scale, one tile to cover the entire frame (1x1)
- Medium scale dividing the frame to 2x2 tiles.
- Small scale dividing the frame to 3x3 tiles.

In this mode we use 1 + 4 + 9 = 14 tiles for each frame.
We can simplify the process by highliting the main tasks:
crop -> inference -> post-process -> aggregate → remove exceeded boxes → remove large landscape → perform NMS

### Model

- `mobilenet_ssd` in resolution of 300X300X3. https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/ssd_mobilenet_v1.yaml

### Options
```sh
./multi_scale_tiling.sh [OPTIONS] [-i INPUT_PATH]
```
* `-i --input` is an optional flag, a path to the video file displayed.
* `--print-gst-launch` prints the ready gst-launch command without running it
* `--show-fps`  optional - enables printing FPS on screen
* `--tiles-x-axis` optional - set number of tiles along x axis (columns)
* `--tiles-y-axis` optional - set number of tiles along y axis (rows)
* `--overlap-x-axis` optional - set overlap in percentage between tiles along x axis (columns)
* `--overlap-y-axis` optional - set overlap in percentage between tiles along y axis (rows)
* `--iou-threshold ` optional - set iou threshold for NMS.
* `--border-threshold` optional - set border threshold to Remove tile's exceeded objects.
* `--scale-level` optional - set scales (layers of tiles) in addition to the main layer. 1: [(1 X 1)] 2: [(1 X 1), (2 X 2)] 3: [(1 X 1), (2 X 2), (3 X 3)]]'

### Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/x86/tiling
./multi_scale_tiling.sh
```
The output should look like:
<div align="center">
    <img src="readme_resources/multi_scale_tiling.gif" width="1000px" height="600px"/> 
</div>

### How it works

As multi scale tiling is almost equal to single scale i will mention the differences:

```sh
TILE_CROPPER_ELEMENT="hailotilecropper internal-offset=$internal_offset name=cropper tiling-mode=1 scale-level=$scale_level
```
`hailotilecropper` sets `tiling-mode` to 1 (0 - single-scale, 1 - multi-scale) and `scale-level` to define what is the structure of scales/layers in addition to the main scale.

`hailonet` hef-path is `mobilenet_ssd` which is training dataset includes objects in different sizes.

```sh
 hailotileaggregator iou-threshold=$iou_threshold border-threshold=$border_threshold name=agg
 ```

 `hailotileaggregator` sets `border-threshold` used in remove tile's exceeded objects process.