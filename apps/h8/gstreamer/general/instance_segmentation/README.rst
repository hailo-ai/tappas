
Instance Segmentation Pipeline
==============================

Overview
--------

``instance_segmentation.sh`` demonstrates instance segmentation on one video file source and verifies Hailo’s configuration.
 This is done by running a ``single-stream instance segmentation pipeline`` on top of GStreamer using the Hailo-8 device.

Options
-------

.. code-block:: sh

   ./instance_segmentation.sh [--input FILL-ME]


* ``--network`` is an optional flag that allowes you to choose the network. The options are yolov5seg, yolact_20_classes, yolact1_6gf, yolact800mf (default is yolov5seg).
* ``--input`` is an optional flag, a path to the video displayed (default is detection.mp4).
* ``--show-fps``  is an optional flag that enables printing FPS on screen.
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it.

Run
---

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/instance_segmentation
   ./instance_segmentation.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/instance_segmentation_run.gif" width="640px" height="360px"/>
   </div>

Model
-----

* ``yolov5n_seg`` - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5n_seg.yaml
* ``yolact_regnetx_800mf_20classes`` - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolact_regnetx_800mf_20classes.yaml
* ``yolact_regnetx_800mf`` - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolact_regnetx_800mf.yaml
* ``yolact_regnetx_1.6gf`` - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolact_regnetx_1.6gf.yaml


How does it work?
-----------------

This app is based on our `single network pipeline template <../../../../../docs/pipelines/single_network.rst>`_

How to use Retraining to replace models
---------------------------------------

.. note:: It is recommended to first read the `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_ page. 

You can use Retraining Dockers (available on Hailo Model Zoo), to replace the following models with ones
that are trained on your own dataset:

- ``yolact_regnetx_800mf, yolact_regnetx_800mf_20classes, yolact_regnetx_1.6gf``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/yolact>`_

   - For best compatibility and performance with TAPPAS, use for compilation the corresponsing YAML file from above.
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update `yolact.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/libs/postprocesses/instance_segmentation/yolact.cpp#L458>`_
      with your new paremeters, then recompile to create ``libyolact_post.so``

- ``yolov5n_seg``

  - No retraining docker is available.
  - Post process CPP file edit update post-processing:

    - Update `yolov5seg.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/libs/postprocesses/instance_segmentation/yolov5seg.cpp>`_
      with your new paremeters, then recompile to create ``libyolov5seg_post.so``

