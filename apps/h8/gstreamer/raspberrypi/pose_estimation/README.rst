
Pose Estimation Pipeline
========================

Overview
--------

``hailo_pose_estimation.sh`` demonstrates human pose estimation on one video file source and verifies Hailo’s configuration.
 This is done by running a ``single-stream pose estimation pipeline`` on top of GStreamer using the Hailo-8 device.

Options
-------

.. code-block:: sh

   ./hailo_pose_estimation.sh [--input FILL-ME]


* ``--input`` is an optional flag, a path to the video displayed (default is detection.mp4).
* ``--show-fps``  is an optional flag that enables printing FPS on screen.
* ``--network``   Set network to use. choose from [centerpose, centerpose_416], default is centerpose
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it"

Run
---

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/raspberrypi/pose_estimation
   ./hailo_pose_estimation.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/centerpose.gif" width="640px" height="360px"/>
   </div>

Models
------


* ``centerpose_regnetx_1.6gf_fpn``: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/centerpose_regnetx_1.6gf_fpn.yaml
* ``centerpose_repvgg_a0``: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/centerpose_repvgg_a0.yaml

How it works
------------

This app is based on our `single network pipeline template <../../../../../docs/pipelines/single_network.rst>`_

With small modifications:


#. Use decode elements instead of ``decodebin``
#. Increase the number of threads on the ``videoconvert``

How to use Retraining to replace models
---------------------------------------

.. note:: It is recommended to first read the `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_ page. 

You can use Retraining Dockers (available on Hailo Model Zoo), to replace the following models with ones
that are trained on your own dataset:

- ``centerpose_regnetx_1.6gf_fpn``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/centerpose>`_
  - TAPPAS changes to replace model:

    - Replace HEF_PATH on the .sh file
    - Update `centerpose.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/libs/postprocesses/pose_estimation/centerpose.cpp#L417>`_
      with your new paremeters, then recompile to create ``libcenterpose_post.so``
