
Multi-Stream Object Detection Pipeline
======================================

Overview
--------

This GStreamer pipeline demonstrates object detection on multiple camera streams over RTSP protocol / files.
All the streams are processed in parallel via the hardware accelerated decoder, and after scaling, the frames are sent to the Hailo chip frame by frame.

After postprocess and optional drawing phases, the classified object and bounding boxes are added to each frame.
Each frame is then separated by the input stream, the Hailo meta data is saved to a .json file, the image stream is encoded and sent to a remote host.
On the remote host, the RTP server receives the data from each stream.

This pipeline uses the Rockchip mpp elements for hardware-accelerated decoding and encoding capabilities.

Read more about RTSP: `RTSP <../../../../../docs/terminology.rst#real-time-streaming-protocol-rtsp>`_
Read more about MPP: `MPP <../../../../../docs/terminology.rst#rockchip-media-process-platform-mpp>`_

Prerequisites
-------------


* Firefly ITX-3588J
* In cases where RTSP cameras are used: `RTSP <../../../../../docs/terminology.rst#real-time-streaming-protocol-rtsp>`_ Cameras, we recommend using: `AXIS M10 Network Cameras <https://www.axis.com/products/axis-m1045-lw>`_
* Hailo-8 device connected via PCIe

Preparations
------------

Before running the pipeline, configuration of the RTSP camera sources are required.
Open the ``multi_stream_detection_rtsp.sh`` in edit mode with your preferred editor.
Configure the eight sources to match your own cameras.

.. code-block:: sh

   readonly SRC_0="rtsp://<ip address>/?h264x=4 user-id=<username> user-pw=<password>"
   readonly SRC_1="rtsp://<ip address>/?h264x=4 user-id=<username> user-pw=<password>"
   etc..

Run the Pipeline
----------------

On the remote machine is recommended to run first the receiving script:

.. code-block:: sh

   ./rtp-src_streams.sh


#. ``--host-ip`` Change the host to where to get the rtp stream.
#. ``--port`` Set the port to listen for the rtp stream."
#. ``--caps`` A string that defines the input stream capabilities, used by the pipeline for decode and display.

Then on the Rockchip run the next script:

.. code-block:: sh

   ./multi_stream_detection.sh

OR

.. code-block:: sh

   ./multi_stream_detection_rtsp.sh



#. ``--num-of-sources`` Sets the number of sources to use by given input. The default and recommended value in this pipeline is 10 "
#. ``--use-overlay`` If set the pipeline will print the bounding boxes after post processing, it may affect the application performance on streams which have a large amount of detections.
#. ``--udp-sink-ip`` Sets the ip of the remote host to send the video streams after processing.


The output should look like:

.. raw:: html

   <div align="center">
       <img src="readme_resources/example.gif" width="600px" height="250px"/>
   </div>

Configuration
-------------

The app post process parameters can be configured by a json file located in $TAPPAS_WORKSPACE/apps/h8/gstreamer/rockchip/multistream_detection/resources/configs/yolov5.json

Supported Networks
------------------

* 'yolov5s_nv12' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5s_nv12.yaml

Overview of the Pipeline
------------------------

These apps are based on our `multi stream pipeline template <../../../../../docs/pipelines/multi_stream.rst>`_

Specific RTSP Elements Used
^^^^^^^^^^^^^^^^^^^^^^^^^^^

* ``rtspsrc`` Makes a connection to an rtsp server and read the data. Used as a src to get the video stream from rtsp-cameras.
* ``rtph264depay`` Extracts h264 video from rtp packets.
* ``rtph264pay`` Insert the h264 video to rtp packets.
* ``udpsink`` Sends packets using the UDP protocol.

MPP specific elements used
^^^^^^^^^^^^^^^^^^^^^^^^^^

* ``mppvideodec`` Decodes the input h264 encoded stream using the MPP to allow for HW acceleration.
* ``mpph264enc`` Encodes the input video stream in to h264 video stream using the MPP to allow for HW acceleration.

