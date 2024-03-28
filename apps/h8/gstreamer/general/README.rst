
GStreamer Based General Applications
================================

Where to Begin?
---------------

For the starting point Hailo provides a `Sanity Pipeline <sanity_pipeline/README.rst>`_ that helps verify that the installation phase went well.

.. important:: Example applications performance varies on different hosts (affected by the host's processing power and throughput).

#. `Sanity Pipeline <sanity_pipeline/README.rst>`_ - Helps you verify that all the required components are installed correctly
#. `Detection <detection/README.rst>`_ - single-stream object detection pipeline on top of GStreamer using the Hailo-8 device.
#. `Depth Estimation <depth_estimation/README.rst>`_ - single-stream depth estimation pipeline on top of GStreamer using the Hailo-8 device.
#. `Instance segmentation <instance_segmentation/README.rst>`_ - single-stream instance segmentation on top of GStreamer using the Hailo-8 device.
#. `Multi-stream detection <multistream_detection/README.rst>`_ - Multi stream object detection (up to 8 RTSP camera into one Hailo-8 chip).
#. `Face Detection and Facial Landmarking Pipeline <cascading_networks/README.rst>`_ - Face detection and then facial landmarking.
#. `Tiling <tiling/README.rst>`_ - Single scale tiling detection application.
#. `Multi-stream Multi-device <multistream_multidevice/README.rst>`_ - Demonstrate Hailo's capabilities using multiple-chips and multiple-streams.
#. `Python <python/README.rst>`_ - Classification app using ``resnet_v1_50`` with python post-processing.
#. `License Plate Recognition <license_plate_recognition/README.rst>`_ - LPR app using ``yolov5m`` vehicle detection, ``tiny-yolov4`` license plate detection, and ``lprnet`` OCR extraction with Hailonet network-switch capability.
#. `Multi Person Multi Camera Tracking Pipeline <multi_person_multi_camera_tracking/README.rst>`_ - Tracking persons across multiple streams.
#. `Century Pipeline <century/README.rst>`_ - Detection Pipeline with multiple devices.
