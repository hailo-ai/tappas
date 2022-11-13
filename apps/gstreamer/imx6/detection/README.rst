
Detection Pipeline
==================

Detection
---------

The purpose of ``detection.sh`` is to demonstrate object detection using a camera as input.
This is done by running a ``single-stream object classification pipeline`` on top of GStreamer using the Hailo-8 device.

.. raw:: html
  
  <div align="center"><img src="readme_resources/pipeline_run.gif"/></div>


Options
-------

.. code-block:: sh

   ./detection.sh [--input FILL-ME]


* ``--input`` is an optional flag, a path to the camera (default is /dev/video0).
* ``--show-fps`` is a flag that prints the pipeline's fps to the screen.
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it.

Supported Networks
------------------


* 'mobilenet_ssd' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/ssd_mobilenet_v1.yaml

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

This app is based on our `single network with resolution preservation pipeline template <../../../../docs/pipelines/single_network.rst#example-pipeline-with-resolution-preservation>`_


