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

The yolo post process parameters can be configured by a json file located in $TAPPAS_WORKSPACE/apps/gstreamer/general/network_switch/resources/configs/yolov5.json

Run
---

Exporting ``TAPPAS_WORKSPACE`` environment variable is a must before running the app.

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/gstreamer/general/network_switch/detection_and_depth_estimation_networks_switch


.. raw:: html

   <h4>The output should look like:</h4>

   <div align="center">
       <img src="readme_resources/networks_switch.gif" width="640px" height="240px"/>
   </div>


How the application works
-------------------------

This app uses HailoRT Model Scheduler, read more about HailoRT Model Scheduler GStreamer integration at `HailoNet  <../../../../docs/elements/hailo_net.rst>`_
