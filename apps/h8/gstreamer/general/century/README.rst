Century Pipeline
================

Overview
--------

| ``century.sh`` demonstrates detection on one video file source over multiple Hailo-8 devices, either using the `Century platform <https://hailo.ai/product-hailo/hailo-8-century-evaluation-platform/>`_\ , or other multi device configurations (i.e., multiple M.2 modules connected directly to the same host). While this application defaults to 4 devices, any number of Hailo-8 devices are supported.
| This pipeline runs the detection network Yolov5.

Options
-------

.. code-block:: sh

   ./century.sh [--input FILL-ME]


* ``--input`` is an optional flag, a path to the video displayed (default is detection.mp4).
* ``--show-fps``  is an optional flag that enables printing FPS to the console.
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it
* ``--device-count`` is an optional flag that sets the number of devices to use (default 4)

Configuration
-------------

The app post process parameters can be configured by a json file located in $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/century/resources/configs/yolov5.json


Supported Networks
------------------

* 'yolov5m_wo_spp_60p' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m_wo_spp_60p.yaml
* 'yolox_l_leaky' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m_wo_spp_60p.yaml

Run
---

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/century
   ./century.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/pipeline_run.gif" width="600px" height="500px"/>
   </div>


How it works
------------

This app is based on our `multi device pipeline template <../../../../../docs/pipelines/multi_device.rst>`_.

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

- ``yolox_l_leaky``

  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/yolovx>`_

   - For best compatibility and performance with TAPPAS, use for compilation the corresponsing YAML file from above.
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update ``resources/configs/yolox.json`` with your new post-processing parameters (NMS)
