
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


* ``--network``   is an optional flag that sets which network to use. Choose from [yolov3, yolov4, yolov5, mobilenet_ssd], default is yolov5.
  This will set which ``hef file`` to use, the corresponding ``hailofilter`` function, and the scaling of the frame to match the width/height input dimensions of the network.
* ``--input`` is an optional flag, a path to the video displayed (default is detection.mp4).
* ``--show-fps``  is an optional flag that enables printing FPS on screen.
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it.
* ``--print-device-stats`` Print the power and temperature measured on the Hailo device.

Configuration
-------------

In case the selected network is yolo, the app post process parameters can be configured by a json file located in $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/detection/resources/configs

Supported Networks
------------------


* 'yolov5m_wo_spp_60p' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m_wo_spp_60p.yaml
* 'yolov4_leaky' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov4_leaky.yaml
* 'yolov3_gluon' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov3_gluon.yaml
* 'mobilenet_ssd' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/ssd_mobilenet_v1.yaml
* 'nanodet_repvgg' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/nanodet_repvgg.yaml

Run
---

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/detection
   ./detection.sh

The output should look like:


.. image:: readme_resources/detection_run.png
   :width: 600px
   :height: 500px


How does it work?
-----------------

This app is based on our `single network pipeline template <../../../../../docs/pipelines/single_network.rst>`_

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
- ``yolov4``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/yolov4>`_

    - For best compatibility and performance with TAPPAS, use for compilation the corresponsing YAML file from above.
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update ``resources/configs/yolov4.json`` with your new post-processing parameters (NMS)
- ``yolov3``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/yolov3>`_

    - For best compatibility and performance with TAPPAS, use for compilation the corresponsing YAML file from above.
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update ``resources/configs/yolov3.json`` with your new post-processing parameters (NMS)
- ``mobilenet_ssd``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/ssd>`_

    - For best compatibility and performance with TAPPAS, use for compilation the corresponsing YAML file from above.
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update `mobilenet_ssd.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/libs/postprocesses/detection/mobilenet_ssd.cpp#L141>`_
      with your new paremeters, then recompile to create ``libmobilenet_ssd_post.so``
- ``nanodet_repvgg``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/nanodet>`_
    
    - For best compatibility and performance with TAPPAS, use for compilation the corresponsing YAML file from above.
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update `nanodet.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/libs/postprocesses/detection/nanodet.cpp#L221>`_
      with your new paremeters, then recompile to create ``libnanodet_post.so``