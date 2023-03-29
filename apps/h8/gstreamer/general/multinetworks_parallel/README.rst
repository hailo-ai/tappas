
Overview
========

This folder contains apps that runs multiple networks in parallel.

Detection and Depth Estimation Pipeline
---------------------------------------

``detection_and_depth_estimation.sh`` demonstrates depth estimation and detection on one video file source.
This is done by running two streams on top of GStreamer using one Hailo-8 device with using two ``hailonet`` elements.

Options
-------

.. code-block:: sh

   ./detection_and_depth_estimation.sh [--input INPUT]


* ``-i --input`` is an optional flag, a path to the video displayed.
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it
* ``--show-fps`` is an optional flag that enables printing FPS on screen

Configuration
-------------

The yolo post process parameters can be configured by a json file located in $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/multinetworks_parallel/resources/configs/yolov5.json

Run
---

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/multinetworks_parallel
   ./detection_and_depth_estimation.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/pipeline_run.gif" width="600px" height="250px"/>
   </div>


Models
------

Joined together:

* ``fast_depth`` in resolution of 224X224X3: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/fast_depth.yaml
* ``mobilenet_ssd`` in resolution of 300X300X3: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/ssd_mobilenet_v1_no_alls.yaml

How it works
------------

This app is based on our `parallel networks pipeline template <../../../../../docs/pipelines/parallel_networks.rst>`_

Detection and Pose Estimation Pipeline
======================================

``detection_and_pose_estimation.sh`` demonstrates pose estimation and detection on one video file source.
This is done by running two streams on top of GStreamer using one Hailo-8 device with using two ``hailonet`` elements.

Options
-------

.. code-block:: sh

   ./detection_and_pose_estimation.sh [--input INPUT]


* ``-i --input`` is an optional flag, a path to the video displayed.
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it
* ``--show-fps`` is an optional flag that enables printing FPS on screen

Run
---

.. code-block:: sh

   cd /local/workspace/tappas/apps/h8/gstreamer/general/multinetworks_parallel
   ./detection_and_pose_estimation.sh

Models
------

Joined together:

* ``centerpose`` in resolution of 416X416X3: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/centerpose_repvgg_a0.yaml
* ``yolov5m_wo_spp`` in resolution of 640X640X3: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m_wo_spp_60p.yaml

How it works
------------

This app is based on our `parallel networks pipeline template <../../../../../docs/pipelines/parallel_networks.rst>`_

How to use Retraining to replace models
---------------------------------------

.. note:: It is recommended to first read the `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_ page. 

You can use Retraining Dockers (available on Hailo Model Zoo), to replace the following models with ones
that are trained on your own dataset:

- ``yolov5m_wo_spp``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/yolov5>`_

    - For best compatibility and performance with TAPPAS, use for compilation the corresponsing YAML file from above.
    - Should use ModelZoo to compile together with ``centerpose`` for this pipeline. 
      See `detection_pose_estimation.yaml <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/multi-networks/detection_pose_estimation/detection_pose_estimation.yaml>`_
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update ``resources/configs/yolov5.json`` with your new post-processing parameters (NMS)
- ``centerpose``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/centerpose>`_
    
    - This retraining docker is for the ``centerpose_regnetx_1.6gf_fpn`` model, therefore it won't fit this pipeline
      (that uses centerpose_repvgg_a0). If you wish to retrain centernet using Hailo retraining dockers, refer to the 
      ``multistream_multidevice`` or ``pose_estimation`` pipelines that use the ``centerpose_regnetx_1.6gf_fpn`` model.
- ``mobilenet_ssd``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/ssd>`_
 
    - Should use ModelZoo to compile together with ``fast_depth`` for this pipeline.
      See `fast_depth_ssd.yaml <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/multi-networks/fast_depth_ssd/fast_depth_ssd.yaml>`_
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update `mobilenet_ssd.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/libs/postprocesses/detection/mobilenet_ssd.cpp#L141>`_
      with your new paremeters, then recompile to create ``libmobilenet_ssd_post.so``