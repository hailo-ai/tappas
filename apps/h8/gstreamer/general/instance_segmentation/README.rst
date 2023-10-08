
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


* ``--input`` is an optional flag, a path to the video displayed (default is detection.mp4).
* ``--show-fps``  is an optional flag that enables printing FPS on screen.
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it.

Run
---

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/instance_segmentation
   ./instance_segmentation.sh

The output should display as:


.. raw:: html

   <div align="center">
       <img src="readme_resources/instance_segmentation_run.gif" width="640px" height="360px"/>
   </div>

Model
-----

* ``yolov5n_seg`` - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5n_seg.yaml


Method of Operation
-------------------

This app is based on the `single network pipeline template <../../../../../docs/pipelines/single_network.rst>`_

Using Retraining to Replace Models
---------------------------------------

.. note:: It is recommended to first read the `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_ page. 

Retraining Dockers (available on Hailo Model Zoo), can be used to replace the following models with ones
that are trained on your own dataset:

- ``yolov5n_seg``

  - No retraining docker is available.
  - Post process CPP file edit update post-processing:

    - Update `yolov5seg.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/libs/postprocesses/instance_segmentation/yolov5seg.cpp>`_
      with your new parameters, then recompile to create ``libyolov5seg_post.so``

