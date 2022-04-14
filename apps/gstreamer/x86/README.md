# GStreamer based x86 applications

## Where should I start?

That's a great question! Hailo provides a [Sanity Pipeline](sanity_pipeline/README.md) that helps you verify that the installation phase went well. This is a good starting point.


1. [Sanity Pipeline](sanity_pipeline/README.md) - Helps you verify that all the required components are installed correctly
2. [Detection](detection/README.md) - single-stream object detection pipeline on top of GStreamer using the Hailo-8 device.
3. [Depth Estimation](depth_estimation/README.md) - single-stream depth estimation pipeline on top of GStreamer using the Hailo-8 device.
4. [Multinetworks parallel](multinetworks_parallel/README.md) - single-stream multi-networks pipeline on top of GStreamer using the Hailo-8 device.
5. [Instance segmentation](instance_segmentation/README.md) - single-stream instance segmentation on top of GStreamer using the Hailo-8 device.
6. [Multi-stream detection](multistream_detection/README.md) - Multi stream object detection (up to 8 RTSP camera into one Hailo-8 chip).
7. [Pose Estimation](pose_estimation/../README.md) - Human pose estimation using `centerpose` network.
8. [Segmentation](segmentation/README.md) - Semantic segmentation using `resnet18_fcn8` network on top of GStreamer using the Hailo-8 device.
9. [Facial landmark](facial_landmarks/README.md) - Facial landmarking application.
10. [Face Detection](face_detection/README.md) - Face detection application.
11.  [Face Detection and Facial Landmarking Pipeline](cascading_networks/README.md) - Face detection and then facial landmarking.
12. [Tiling](tiling/README.md) - Single scale tiling detection application.
13. [Classification](classification/README.md) - Classification app using `resnet_v1_50`.
14. [Multi-stream Multi-device](multistream_multidevice/README.md) - Demonstrate Hailo's capabilities using multiple-chips and multiple-streams.
15. [Detection and Depth Estimation - networks switch App](network_switch/README.MD) - Demonstrates Hailonet network-switch capability. 
16. [Python](python/README.md) - Classification app using `resnet_v1_50` with python post-processing.
17. [License Plate Recognition](license_plate_recognition/README.md) - LPR app using `yolov5m` vehicle detection, `tiny-yolov4` license plate detection, and `lprnet` OCR extraction with Hailonet network-switch capability.
