# Face Detection and Facial Landmarks Pipeline

## Table of Contents

- [Face Detection and Facial Landmarks Pipeline](#face-detection-and-facial-landmarks-pipeline)
  - [Table of Contents](#table-of-contents)
  - [Face Detection and Facial Landmarks](#face-detection-and-facial-landmarks)
  - [Options](#options)
  - [Run](#run)
  - [Model](#model)
  - [How it works](#how-it-works)

## Face Detection and Facial Landmarks
`face_detection_and_landmarks.sh` demonstrates face detection and    facial landmarking on one video file source.
 This is done by running a face detection pipeline (infer + postprocessing), cropping and scaling all detected faces, and sending them into z 2nd network of facial landmarking. All resulting detections and landmarks are then aggregated and drawn on the original frame. The two networks are running using one Hailo-8 device with two `hailonet` elements.

## Options
```sh
./face_detection_and_landmarks.sh [OPTIONS] [-i INPUT_PATH]
```
* `-i --input` is an optional flag, a path to the video displayed.
* `--print-gst-launch` prints the ready gst-launch command without running it
* `--show-fps`  optional - enables printing FPS on screen

## Run

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/raspberrypi/cascading_networks
./face_detection_and_landmarks.sh
```

## Model
- `lightface_slim` in resolution of 320X240X3 - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/lightface_slim.yaml.
- `tddfa_mobilenet_v1` in resolution of 120X120X3 - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/tddfa_mobilenet_v1.yaml.

## How it works
This section is optional and provides a drill-down into the implementation of the app with a focus on explaining the `GStreamer` pipeline.
This section uses `lightface_slim` as an example network so network input width, height, hef name, are set accordingly.
    
```sh
FACE_DETECTION_PIPELINE="videoscale n-threads=8 qos=false ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailonet net-name=joined_lightface_slim_tddfa_mobilenet_v1/lightface_slim \
    hef-path=$hef_path is-active=true qos=false ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$detection_postprocess_so function-name=lightface qos=false ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0"

FACIAL_LANDMARKS_PIPELINE="queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailonet net-name=joined_lightface_slim_tddfa_mobilenet_v1/tddfa_mobilenet_v1 \
    hef-path=$hef_path is-active=true qos=false ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailofilter function-name=facial_landmarks_merged so-path=$landmarks_postprocess_so qos=false ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0"

gst-launch-1.0 \
    $source_element ! \
    tee name=t hailomuxer name=hmux \
    t. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! hmux. \
    t. ! $FACE_DETECTION_PIPELINE ! hmux. \
    hmux. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailocropper internal-offset=$internal_offset name=cropper hailoaggregator name=agg \
    cropper. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! agg. \
    cropper. ! $FACIAL_LANDMARKS_PIPELINE ! agg. \
    agg. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$landmarks_draw_so qos=false ! \
    queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! videoconvert n-threads=8 ! \
    fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=false text-overlay=false ${additonal_parameters}
```
Let's explain this pipeline section by section:
1. ```sh
    filesrc location=$video_device ! qtdemux ! h264parse ! avdec_h264 ! videoconvert n-threads=8 !
   ```
    Specifies the location of the video used, then decodes and converts to the required format.
    
2. ```sh
    tee name=t hailomuxer name=hmux
   ``` 
   Split into two threads - one for doing face detection, the other one for getting the original frame.
   We merge those 2 threads back by using hailomuxer, which takes the frame from it's first sink and adds the metadata from the other sink

3. ```sh
    t. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! hmux. \
   ```
   The first thread, only passes the original frame.

4. ```sh
    t. ! $FACE_DETECTION_PIPELINE ! hmux. \
   ```
   The second thread performs the face detection pipeline where FACE_DETECTION_PIPELINE is:
   -  ```sh
      videoscale n-threads=8 qos=false ! \
      ```
      Scales the picture to a resolution negotiated with the hailonet down the pipeline, according to the needed resolution by the hef file.
   -  ```sh
      queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
      ```
      Before sending the frames into `hailonet` element, set a queue so no frames are lost (Read more about queues [here](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html?gi-language=c))
   -  ```sh
      hailonet hef-path=$hef_path debug=False is-active=true net-name=joined_lightface_slim_tddfa_mobilenet_v1/lightface_slim qos=false batch-size=1 
      queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
      ```
      Performs the inference on the Hailo-8 device.

   -  ```sh
      hailofilter so-path=$detection_postprocess_so qos=false debug=False ! \
      queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
      ```
      Performs a given post-process, in this case, performs `lightface_slim` face detection post-processing.
5. ```sh
   hmux. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
   hailocropper internal-offset=$internal_offset name=cropper hailoaggregator name=agg \
   ```
   links the hailomuxer to a queue and defines the cascading network elements hailocropper and hailoaggregator.
   hailocropper splits the pipeline into 2 threads, the first thread passes the original frame, the other thread passes the croppes of the original frame created by hailocropper according to the detections added to the buffer by prior hailofilter post-processing, the buffers are also scaled to the following hailonet, done by caps negotiation.
   The hailoaggregator gets the original frame and then knows to wait for all related cropped buffers and add all related metadata on the original frame, and send it forward.
6. ```sh
   cropper. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! agg. \
   ``` 
   The first part of the cascading network pipeline, passes the original frame on the bypass pads to hailoaggregator.
7. ```sh
   cropper. ! $FACIAL_LANDMARKS_PIPELINE ! agg. \
   ```
   The second part of the cascading network pipeline, performs a second network on all detections, which are cropped and scaled to the needed resolution by the HEF in the hailonet. FACIAL_LANDMARKS_PIPELINE consists of:
   -  ```sh
      queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
      ```
      Before sending the frames into the `hailonet` element, set a queue so no frames are lost (Read more about queues [here](https://gstreamer.freedesktop.org/documentation/coreelements/queue.html?gi-language=c))
   -  ```sh
      hailonet net-name=joined_lightface_slim_tddfa_mobilenet_v1/tddfa_mobilenet_v1 \
      hef-path=$hef_path is-active=true qos=false ! \
      ```
      Performs inference on the Hailo-8 device.

   -  ```sh
      hailofilter function-name=facial_landmarks_merged so-path=$landmarks_postprocess_so qos=false ! \
      queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0
      ```
      Performs a given post-process, in this case, performs `tddfa_mobilenet_v1` facial landmarks post-processing.
8.  ```sh
    agg. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
    hailofilter so-path=$landmarks_draw_so qos=false ! \
    ```
    Aggregates all detected faces with thier landmarks on the original frame, and draws them over the frame using the hailofilter with specific drawing function.
9. ```sh
   queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! videoconvert n-threads=8 ! \
   fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=false text-overlay=false
   ```
   Display the final image using fpsdisplaysink.

> ***NOTE***: Additional details about the pipeline provided in further examples