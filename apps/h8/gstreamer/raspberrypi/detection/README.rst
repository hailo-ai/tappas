
Detection Pipeline
==================

Overview
--------

``detection.sh`` demonstrates detection on one video file source and verifies Hailo’s configuration.
 This is done by running a ``single-stream object detection pipeline`` on top of GStreamer using the Hailo-8 device.

Options
-------

.. code-block:: sh

   ./detection.sh [--input FILL-ME]


* ``--input`` is an optional flag, a path to the video file displayed (default is detection.mp4).
* ``--network``   is a flag that sets which network to use. choose from [yolov5, mobilenet_ssd], default is yolov5.
  this will set the hef file to use, the ``hailofilter`` function to use and the scales of the frame to match the width and heigh input dimensions of the network.
* ``--show-fps``  is an optional flag that enables printing FPS on screen.
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it
* ``--print-device-stats`` Print the power and temperature measured

Configuration
-------------
In case the selected network is yolo, the app post process parameters can be configured by a json file located in $TAPPAS_WORKSPACE/apps/h8/gstreamer/raspberrypi/detection/resources/configs/yolov5.json


Supported Networks
------------------


* 'yolov5m_wo_spp_60p' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m_wo_spp_60p.yaml
* 'mobilenet_ssd' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/ssd_mobilenet_v1.yaml

Run
---

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/raspberrypi/detection
   ./detection.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/pipeline_run.gif" width="600px" height="500px"/>
   </div>


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

- ``yolov5m``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/yolov5>`_

    - For best compatibility and performance with TAPPAS, use for compilation the corresponsing YAML file from above.
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update ``resources/configs/yolov5.json`` with your new post-processing parameters (NMS)
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update ``resources/configs/yolov5.json`` with your new post-processing parameters (NMS)
- ``mobilenet_ssd``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/ssd>`_
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update `mobilenet_ssd.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/libs/postprocesses/detection/mobilenet_ssd.cpp#L141>`_
      with your new paremeters, then recompile to create ``libmobilenet_ssd_post.so``