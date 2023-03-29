Detection and Depth Estimation - networks switch App
====================================================

Overview
--------

``detection_and_depth_estimation_networks_switch`` demonstrates network switch between two networks: Detection network and Depth estimation network on one video source using one Hailo-8 device.
The switch is done every frame, so all frames are inferred by both networks.

Options
-------

.. code-block:: sh

   ./detection_and_depth_estimation_networks_switch [--input FILL-ME --show-fps]


* ``--input`` is an optional flag, a path to the video displayed (default is instance_segmentation.mp4).
* ``--show-fps``  is an optional flag that enables printing FPS on screen.

Configuration
-------------

The yolo post process parameters can be configured by a json file located in $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/network_switch/resources/configs/yolov5.json

Run
---

Exporting ``TAPPAS_WORKSPACE`` environment variable is a must before running the app.

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/network_switch/detection_and_depth_estimation_networks_switch


.. raw:: html

   <h4>The output should look like:</h4>

   <div align="center">
       <img src="readme_resources/networks_switch.gif" width="640px" height="240px"/>
   </div>

Models
------


* ``fast_depth``: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/fast_depth.yaml
* ``yolov5s``: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5s.yaml

How the application works
-------------------------

This app uses HailoRT Model Scheduler, read more about HailoRT Model Scheduler GStreamer integration at `HailoNet  <../../../../../docs/elements/hailo_net.rst>`_

How to use Retraining to replace models
---------------------------------------

.. note:: It is recommended to first read the `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_ page. 

You can use Retraining Dockers (available on Hailo Model Zoo), to replace the following models with ones
that are trained on your own dataset:

- ``yolov5s``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/yolov5>`_

    - For best compatibility and performance with TAPPAS, use for compilation the corresponsing YAML file from above.
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update ``resources/configs/yolov5.json`` with your new post-processing parameters (NMS)