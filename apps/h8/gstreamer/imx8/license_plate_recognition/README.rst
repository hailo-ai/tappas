
License Plate Recognition
=========================

Overview
--------

``license_plate_recognition.sh`` demonstrates model scheduling in a complex pipeline with inference based decision making. The overall task is to ``detect and track vehicles`` in the pipeline and then ``detect/extract license plate numbers`` from newly tracked instances.

Configuration
-------------

The yolo post processes parameters can be configured by a json file located in $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/license_plate_recognition/resources/configs

Configuration
-------------

The yolo post processes parameters can be configured by a json file located in $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/license_plate_recognition/resources/configs

Run
---

.. code-block:: sh

   ./apps/license_plate_recognition/license_plate_recognition.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/lpr_pipeline.gif"/>
   </div>

Models
------


* ``yolov5m_vehicles_yuy2``: yolov5m pre-trained on Hailo's dataset - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m_vehicles_yuy2.yaml
* ``tiny_yolov4_license_plates_yuy2``: tiny_yolov4 pre-trained on Hailo's dataset - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/tiny_yolov4_license_plates_yuy2.yaml
* ``lprnet_yuy2``: lprnet pre-trained on Hailo's dataset - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/lprnet_yuy2.yaml


How the application works
-------------------------

This app uses HailoRT Model Scheduler, read more about HailoRT Model Scheduler GStreamer integration at `HailoNet  <../../../../../docs/elements/hailo_net.rst>`_

How to use Retraining to replace models
---------------------------------------

.. note:: It is recommended to first read the `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_ page. 

You can use Retraining Dockers (available on Hailo Model Zoo), to replace the following models with ones
that are trained on your own dataset:

- ``yolov5m_vehicles``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_models/vehicle_detection/docs/TRAINING_GUIDE.rst>`_

  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update ``configs/yolov5_vehicle_detection.json`` with your new post-processing parameters (NMS)
- ``tiny_yolov4_license_plates``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_models/license_plate_detection/docs/TRAINING_GUIDE.rst>`_

  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update ``configs/yolov4_licence_plate.json`` with your new post-processing parameters (NMS)
- ``lprnet``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_models/license_plate_recognition/docs/TRAINING_GUIDE.rst>`_

  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update `ocr_postprocess.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/libs/postprocesses/ocr/ocr_postprocess.cpp#L20>`_
      with your new paremeters, then recompile to create ``libocr_post.so``