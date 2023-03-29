License Plate Recognition
=========================

Overview
--------

``license_plate_recognition.sh`` demonstrates model scheduling between 3 different networks in a complex pipeline with inference based decision making.
The overall task is to ``detect and track vehicles`` in the pipeline and then ``detect/extract license plate numbers`` from newly tracked instances.
When enough newly tracked vehicles are detected (single class yolov5m), a network switch is made to a tiny-yolov4 based network that detects licence plates.
If enough license plates of good image quality (not blurred) are found, then the network is changed again to a thrid HEF - lprnet license plate text extraction (OCR). 
Once the license plate is detected and its text extracted, the pipeline updates the ``hailotracker`` JDE Tracking element upstream with the license plate for the corresponding vehicle.
From there the vehicle is tracked along with its license plate number, and is omitted from being re-inferred on new frames.
The logic for the network switching is handled by the hailonet elements behind the scenes.

Options
-------

.. code-block:: sh

   ./license_plate_recognition.sh [--show-fps]


* ``--show-fps``  is an optional flag that enables printing FPS on screen.

Configuration
-------------

The yolo post processes parameters can be configured by a json file located in $TAPPAS_WORKSPACE/apps/h8/gstreamer/rockchip/license_plate_recognition/resources/configs


Run
---

Exporting ``TAPPAS_WORKSPACE`` environment variable is a must before running the app.

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/rockchip/license_plate_recognition/license_plate_recognition.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/lpr_pipeline.gif"/>
   </div>

Models
------


* ``yolov5m_vehicles``: yolov5m pre-trained on Hailo's dataset - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m_vehicles.yaml
* ``tiny_yolov4_license_plates``: tiny_yolov4 pre-trained on Hailo's dataset - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/tiny_yolov4_license_plates.yaml
* ``lprnet``: lprnet pre-trained on Hailo's dataset - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/lprnet.yaml

How the application works
-------------------------

This section explains the network switching.

The app builds a gstreamer pipeline (that is explained below) and utilises the ``scheduling-algorithm`` property of its hailonet elements. This lets the hailonet elements know that we wish to switch networks on the same device.
The hailonets perform network switching by blocking their sink pads when it is time to switch: turning off one hailonet and turning on the other. Before turning a hailonet element on, it has to flush the buffers out of the element, this is all handled internaly. `read more about hailonet <../../../../../docs/elements/hailo_net.rst>`_

How the pipeline works
----------------------

This section is optional and provides a drill-down into the implementation of the ``License Plate Recognition`` app with a focus on explaining the ``GStreamer`` pipeline.

Pipeline diagram
----------------


.. image:: readme_resources/lpr_pipeline.png


The following elements are the structure of the pipeline:


* | ``Model 0`` - Vehicle Detection and Tracking

  * | ``filesrc`` reads data from a file in the local file system.
  * | ``decodebin``  constructs a decoding sub-pipeline using available decoders and demuxers 
  * | ``videoconvert`` converts the frame into network input format.
  * | ``hailonet``  Performs the inference on the Hailo-8 device.
    | This intance of hailonet performs yolov5m network inference for vehicle detection.
    | `read more about hailonet <../../../../../docs/elements/hailo_net.rst>`_ 
  * | ``hailofilter`` performs the given postprocess, chosen with the ``so-path`` property. This instance is in charge of yolo post processing.
  * | ``hailotracker`` performs JDE Tracking using a kalman filter, applying a unique id to tracked vehicles. This element also receives updates of license plate text and associates them to their corresponding tracked vehicle.
    | `read more about hailotracker <../../../../../docs/elements/hailo_tracker.rst>`_ 
  * | ``tee`` splits the piepline into two branches. While one buffer continues the drawing and displaying, the other continues to license plate detection and extraction.
  * | ``hailooverlay`` draws the postprocess results on the frame.
  * | ``fpsdisplaysink`` outputs video onto the screen, and displays the current and average framerate.


* | ``Model 1`` - License Plate Detection

  * | ``hailocropper`` crops vehicle detections from the original full HD image and resizes them to the input size of the following ``hailonet`` (licence plate detection). Extra decision making is applied to only pass vehicles that have not had license plate detected and text extracted yet. 
    | `read more about hailocropper <../../../../../docs/elements/hailo_cropper.rst>`_

    * ``hailonet`` this intance of hailonet performs tiny-yolov4 network inference for license plate detection. When initiallizing the pipeline this instance of hailonet is set to is-active=false.
    * ``hailofilter`` this instance of hailofilter is in charge of tiny-yolov4 post processing.

  * | ``hailoaggregator`` waits for all crops belonging to the original frame to arrive and merges all metas into their original frame. So, for example, if the upstream ``hailocropper`` cropped 4 vehicles from the original frame, then this ``hailoaggregator`` will wait to recieve 4 buffers along witht he original frame.
    | `read more about hailoaggregator <../../../../../docs/elements/hailo_aggregator.rst>`_


* | ``Model 2`` - License Plate Text Extraction (OCR)

  * | ``hailocropper`` another cropping element, this time the decision making is an image quality estimator - if the license plate detection is determined to be too blurry for OCR, then it is dropped. If the detection is not too blurry, then a crop of the license plate is taken from the original full HD image and sent to for OCR inference.

    * ``hailonet`` this intance of hailonet performs lprnet network inference for license plate text extraction. When initiallizing the pipeline this instance of hailonet is set to is-active=false.
    * ``hailofilter`` this instance of hailofilter is in charge of OCR post processing.

  * | ``hailoaggregator`` waits for all crops belonging to the original frame to arrive and merges all metas into their original frame.
  * | ``hailofilter`` captures incoming buffers. From these the ocr text is extracted and sent upstream behind the scenes. 
      These events contain both the OCR postprocess results and the unique tracking id of the vehicle they were extracted from. 
      The event is caught by the ``hailotracker`` element which updates the corresponding entry in its tracked vehicle database. 

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