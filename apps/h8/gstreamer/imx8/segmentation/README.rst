
Segmentation Pipelines
======================

Semantic Segmentation
---------------------

``semantic_segmentation.sh`` demonstrates semantic segmentation on one video file source.
 This is done by running a ``single-stream object semantic segmentation pipeline`` on top of GStreamer using the Hailo-8 device.

Options
-------

.. code-block:: sh

   ./semantic_segmentation.sh [--input FILL-ME]


* ``--input`` is an optional flag, a path to the video displayed (default is full_mov_slow.mp4).
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it
* ``--show-fps``  is an optional flag that enables printing FPS on screen

Model
-----


* 'fcn8_resnet_v1_18' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/fcn8_resnet_v1_18.yaml

Run
---

.. code-block:: sh

   cd /home/root/apps/segmentation
   ./semantic_segmentation.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/pipeline_run.gif" width="600px" height="500px"/>
   </div>


Model
-----


* ``fcn8_resnet_v1_18`` in resolution of 1920x1024x3.
* Numeric accuracy 65.18mIOU.
* Pre trained on cityscapes using GlounCV and a resnet-18-
  FCN8 architecture.

How does it work?
-----------------

This app is based on our `single network pipeline template <../../../../../docs/pipelines/single_network.rst>`_

How to use Retraining to replace models
---------------------------------------

.. note:: It is recommended to first read the `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_ page. 

You can use Retraining Dockers (available on Hailo Model Zoo), to replace the following models with ones
that are trained on your own dataset:

- ``fcn8_resnet_v1_18``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/fcn>`_
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update `semantic_segmentation.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/libs/postprocesses/semantic_segmentation/semantic_segmentation.cpp#L10>`_
      with your new paremeters, then recompile to create ``libsemantic_segmentation.so``