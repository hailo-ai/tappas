
Multi-Stream object detection Pipeline
======================================

Overview
--------

This GStreamer pipeline demonstrates object detection on multiple camera streams over RTSP protocol / Files.

All the streams are processed in parallel through the decode and scale phases, and enter the Hailo device frame by frame.

Afterwards postprocess and drawing phases add the classified object and bounding boxes to each frame. \
The last step is to match each frame back to its respective stream and output all of them to the display.

Read more about RTSP: `RTSP <../../../../../docs/terminology.rst#real-time-streaming-protocol-rtsp>`_

Prerequisites
-------------


* TensorPC
* In case of using RTSP cameras: `RTSP <../../../../../docs/terminology.rst#real-time-streaming-protocol-rtsp>`_ Cameras, We recommend using: `AXIS M10 Network Cameras <https://www.axis.com/products/axis-m1045-lw>`_
* Hailo-8 device connected via PCIe

Preparations
------------

In case of using RTSP cameras, configuration of the RTSP camera sources is required before running.
open the ``multi_stream_detection_rtsp.sh`` in edit mode with your preferred editor.
Configure the eight sources to match your own cameras.

.. code-block:: sh

   readonly SRC_0="rtsp://<ip address>/?h264x=4 user-id=<username> user-pw=<password>"
   readonly SRC_1="rtsp://<ip address>/?h264x=4 user-id=<username> user-pw=<password>"
   etc..

Run the pipeline
----------------

.. code-block:: sh

   ./multi_stream_detection.sh

OR

.. code-block:: sh

   ./multi_stream_detection_rtsp.sh


#. ``--show-fps`` Prints the fps to the output.
#. ``--debug`` Uses gst-top to print time and memory consuming elements, saves the results as text and graph.
#. ``--num-of-sources`` Sets the number of sources to use by given input. The default and recommended value in this pipeline is 12 (for files) or 8 (for camera streams over RTSP protocol).
#. ``--set-live-source`` Use the live source given (example: /dev/video2). this flag is optional. if it's in use, num_of_sources is limited to 4.
#. ``--tcp-address`` If specified, set the sink be a TCP (expected format is 'host:port').
#. ``--print-gst-launch`` Prints the ready gst-launch command without running it.


   **NOTE:** : When the debug flag is used and the app is running inside of a docker, exit the app by tying ``Ctrl+C`` in order to save the results. (Due to docker X11 display communication issues)


The output should look like:


.. image:: readme_resources/example.jpg
   :width: 300px 
   :height: 250px


Configuration
-------------

The app post process parameters can be configured by a json file located in $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/multistream_detection/resources/configs/yolov5.json

Supported Networks
------------------


* 'yolov5m_wo_spp' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m_wo_spp_60p.yaml

Overview of the pipeline
------------------------

These apps are based on our `multi stream pipeline template <../../../../../docs/pipelines/multi_stream.rst>`_

RTSP specific elements used
^^^^^^^^^^^^^^^^^^^^^^^^^^^


* ``rtspsrc`` Makes a connection to an rtsp server and read the data. Used as a src to get the video stream from rtsp-cameras.
* ``rtph264depay`` Extracts h264 video from rtp packets.


HailoRT Stream Multiplexer example
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


* This app shows the usage of the HailoRT Stream Multiplexer. This feature controls the time shared on the Hailo device between all streams. The Stream Multiplexer is enabled by the ``Hailonet`` scheduling-algorithm property when in use in multiple ``Hailonet`` elements that run the same HEF file. When the Stream Multiplexer is in use, there is no need to use ``funnel`` and ``streamiddemux`` like elements because the logic is handeled internally.

How to use Retraining to replace models
---------------------------------------

.. note:: It is recommended to first read the `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_ page. 

You can use Retraining Dockers (available on Hailo Model Zoo), to replace the following models with ones
that are trained on your own dataset:

- ``yolov5m``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/yolov5>`_

    - For best compatibility and performance with TAPPAS, use for compilation the corresponsing YAML file from above.
    - Should use ModelZoo to compile together with ``centerpose`` for this pipeline. 
      See `detection_pose_estimation.yaml <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/multi-networks/detection_pose_estimation/detection_pose_estimation.yaml>`_
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update ``resources/configs/yolov5.json`` with your new post-processing parameters (NMS)
