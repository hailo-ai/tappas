
Detection Pipeline
==================

Detection
---------

The purpose of ``detection.sh`` is to demonstrate object detection using a camera/file as input with i.MX6 hardware accelerators.
This is done by running a ``single-stream object detection pipeline`` on top of GStreamer i.MX6 elements using the Hailo-8 device.

.. warning:: 
    This application is currently not supported on i.MX6

.. raw:: html
  
  <div align="center"><img src="readme_resources/pipeline_run.gif"/></div>


Options
-------

.. code-block:: sh

   ./detection.sh


* ``--input`` is an optional flag, a path to the video displayed (default is detection.mp4).
* ``--show-fps`` is a flag that prints the pipeline's fps to the screen.
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it.

Configuration
-------------

The yolo post processes parameters can be configured by a json file located in /home/root/apps/detection/resources/configs


Supported Networks
------------------


* ``yolov5s_personface``: yolov5s pre-trained on Hailo's dataset - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5s_personface.yaml

Run
---

.. code-block:: sh

   cd /home/root/apps/detection
   ./detection.sh


.. raw:: html
   
  <h2>The output should look like:</h2>

  <div align="center"><img src="readme_resources/pipeline_run.gif" width="600px" height="500px"/></div>


How does it work?
-----------------

This app is based on our `Single Network Pipeline template <../../../../../docs/pipelines/single_network.rst>`_


