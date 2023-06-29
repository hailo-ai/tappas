
iMX8 GStreamer based applications
=================================

.. warning:: The Kirkstone applications portfolio is reduced on i.MX8-based devices, since the Kirkstone branch does not support OpenGL.

#. `Detection <detection/README.rst>`_ - single-stream object detection pipeline on top of GStreamer using the Hailo-8 device.
#. `Depth Estimation <depth_estimation/README.rst>`_ - single-stream depth estimation pipeline on top of GStreamer using the Hailo-8 device.
#. `Facial landmark <facial_landmarks/README.rst>`_ - Facial landmarking application.
#. `Face Detection and Facial Landmarking Pipeline <cascading_networks/README.rst>`_ - Face detection and then facial landmarking.
#. `License Plate Recognition <license_plate_recognition/README.rst>`_ - LPR app using ``yolov5m`` vehicle detection, ``tiny-yolov4`` license plate detection, and ``lprnet`` OCR extraction with Hailonet network-switch capability.
#. `Pose Estimation <pose_estimation/README.rst>`_ - Human pose estimation using ``centerpose416`` network.
#. `Segmentation <segmentation/README.rst>`_ - Semantic segmentation using ``resnet18_fcn8`` network on top of GStreamer using the Hailo-8 device.
