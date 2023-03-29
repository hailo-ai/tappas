
Face Detection Pipeline
=======================

Overview:
---------

The purpose of ``face_detection.sh`` is to demonstrate face detection on one video file source and to verify Hailo’s configuration.
This is done by running a ``single-stream face detection pipeline`` on top of GStreamer using the Hailo-8 device.

Options
-------

.. code-block:: sh

   /face_detection.sh


* ``--network`` is a flag that sets which network to use. choose from [lightface,retinaface], default is lightface.
  this will set the hef file to use, the ``hailofilter`` function to use, and the scales of the frame to match the width/height input dimensions of the network.
* ``--input`` is an optional flag, a path to the video displayed (default is face_detection.mp4).
* ``--show-fps`` is an optional flag that enables printing FPS on screen.
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it"

Supported Networks
------------------


* 'retinaface' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/retinaface_mobilenet_v1.yaml
* 'lightface' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/lightface_slim.yaml

Run
---

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/face_detection/
   ./face_detection.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/face_detection_pipeline.gif" width="320px" height="275px"/> 
   </div>


How does it work?
-----------------

This app is based on our `single network with resolution preservation pipeline template <../../../../../docs/pipelines/single_network.rst#example-pipeline-with-resolution-preservation>`_
