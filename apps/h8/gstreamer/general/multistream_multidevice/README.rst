
Multi-Stream RTSP object detection and pose estimation Pipeline
===============================================================

Overview
--------

This GStreamer pipeline demonstrates object detection on 8 camera streams over RTSP protocol.
This pipeline also demonstrates using two hailo8 devices in parallel.

All the streams are processed in parallel through the decode and scale phases, and enter the Hailo devices frame by frame.
**Each** hailo device is in charge of one inference task (one for yolov5 and the other for centerpose)

Afterwards postprocess and drawing phases add the classified object and bounding boxes to each frame. \
The last step is to match each frame back to its respective stream and output all of them to the display.

Read more about RTSP: `RTSP <../../../../../docs/terminology.rst#real-time-streaming-protocol-rtsp>`_

Prerequisites
-------------


* TensorPC
* Ubuntu 20.04
* `RTSP <../../../../../docs/terminology.rst#real-time-streaming-protocol-rtsp>`_ Cameras, We recommend using: `AXIS M10 Network Cameras <https://www.axis.com/products/axis-m1045-lw>`_
* Two Hailo-8 devices connected via PCIe

Preparations
------------


#. Before running, configuration of the RTSP camera sources is required.
   open the ``rtsp_detection_and_pose_estimation.sh`` in edit mode with your preffered editor.
   Configure the eight sources to match your own cameras.

.. code-block:: sh

   readonly SRC_0="rtsp://<ip address>/?h264x=4 user-id=<username> user-pw=<password>"
   readonly SRC_1="rtsp://<ip address>/?h264x=4 user-id=<username> user-pw=<password>"
   etc..

Run the pipeline
----------------

.. code-block:: sh

   ./rtsp_detection_and_pose_estimation.sh


#. ``--show-fps`` prints the fps to the output.
#. ``--num-of-sources`` sets the number of rtsp sources to use by given input. the default and recommended value in this pipeline is 8 sources
#. ``--print-gst-launch`` prints the ready gst-launch command without running it
#. ``--print-devices-stats`` prints the power and temperature measured
#. ``--debug`` uses gst-top to print time and memory consuming elements, saves the results as text and graph



   **NOTE** : When the debug flag is used and the app is running inside of a docker, exit the app by tying ``Ctrl+C`` in order to save the results. (Due to docker X11 display communication issues)


Models
------


* ``yolov5m_wo_spp_60p`` - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m_wo_spp_60p.yaml
* ``centerpose`` - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/centerpose_regnetx_1.6gf_fpn.yaml


Configuration
-------------

The yolo post process parameters can be configured by a json file located in $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/multistream_multidevice/resources/configs/yolov5.json


Overview of the pipeline
------------------------

These apps are based on our `multi stream pipeline template <../../../../../docs/pipelines/multi_stream.rst>`_

How to use Retraining to replace models
---------------------------------------

.. note:: It is recommended to first read the `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_ page. 

You can use Retraining Dockers (available on Hailo Model Zoo), to replace the following models with ones
that are trained on your own dataset:

- ``yolov5m``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/yolov5>`_

    - For best compatibility and performance with TAPPAS, use for compilation the corresponsing YAML file from above.
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update ``resources/configs/yolov5.json`` with your new post-processing parameters (NMS)
- ``centerpose``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/centerpose>`_
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update `centerpose.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/libs/postprocesses/pose_estimation/centerpose.cpp#L417>`_
       with your new paremeters, then recompile to create ``libcenterpose_post.so``