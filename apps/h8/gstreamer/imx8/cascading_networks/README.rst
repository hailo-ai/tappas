
Object Detection And Pose Estimation Pipeline
============================================

Object Detection And Pose Estimation
------------------------------------

``object_detection_and_pose_estimation.sh`` demonstrates object detection and pose estimation on one video file source.
 This is done by running an object detection pipeline, cropping and scaling each detected person, and sending them into a 2nd network of pose estimation. All resulting detections and landmarks are then aggregated and drawn on the original frame. The two networks are running using one Hailo-8 device with two ``hailonet`` elements.

Options
-------

.. code-block:: sh

   ./object_detection_and_pose_estimation.sh [OPTIONS] [-i INPUT_PATH]


* ``-i --input`` is an optional flag, a path to the video/camera displayed.
* ``--print-gst-launch`` prints the ready gst-launch command without running it
* ``--show-fps``  optional - enables printing FPS on screen

Run
---

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/gstreamer/general/cascading_networks
   ./object_detection_and_pose_estimation.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/object_detection_and_pose_estimation.gif" width="600px" height="250px"/>
   </div>


Model
-----

Joined together:

* 'yolov5m_wo_spp_60p' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m_wo_spp_60p.yaml
* 'mspn_regnetx_800mf' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/mspn_regnetx_800mf.yaml

Method of Operation
-------------------

This app is based on our `cascaded networks pipeline template <../../../../../docs/pipelines/cascaded_nets.rst>`_
