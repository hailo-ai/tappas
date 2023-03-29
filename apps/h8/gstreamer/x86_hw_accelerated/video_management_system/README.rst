Video Management System
=========================

 .. note::

  The Video Managment Systems (VMS) pipeline is currntely optimized and supported only for Ubuntu 22.04 OS (GStreamer 1.20), on previous Ubuntu releases you will encounter some failures or performance issues, and thefore it is required to run this pipeline from either an Ubuntu 22.04 host, or from the Ubuntu 22.04 TAPPAS Docker.

Overview
--------

``video_management_system.sh`` demonstrates model scheduling between 5 different networks, while performing different tasks:
The first task is to detect and track People (yolov5s_personface) and Faces (scrfd_2.5g) across multiple streams,
then the streams are divided into 3 groups:

- Person Attributes: for the detected persons, a network switch is made to a resnet_v1_18 based network that classifies person attributes.
- Face Attributes: for the detected faces, a network switch is made to a resnet_v1_18 based network that classifies face attributes.
- Face Recognition: for the detected faces, a network switch is made to an arcface based network that performs face recognition.

Once the person/face attributes are classified and and a match is found, the pipeline updates the corresponding ``hailotracker`` JDE Tracking element upstream with the attributes for the corresponding Person/Face.
From there the persons / faces are tracked along with thier attributes/name, and is omitted from being re-inferred on new frames (The face is re-inferred every 60 frames in order to get more accurate attributes).
The logic for the network switching is handled by the hailonet elements behind the scenes.

Options
-------

.. code-block:: sh

   ./video_management_system.sh [--show-fps] [--num-of-sources NUM]


* ``--show-fps``  is an optional flag that enables printing FPS on screen.
* ``--num-of-sources`` sets the number of video sources to use by given input. the default, recommended value is 4 and maximal value in this pipeline is 8 sources
* ``--face-attr-streams``        list of streames to perform face attributes on (default is 'sink_1,sink_4')
* ``--person-attr-streams``      list of streames to perform person attributes on (default is 'sink_0,sink_3,sink_6,sink_7')
* ``--face-recognition-streams`` list of streames to perform face recognition on (default is 'sink_2,sink_5')

Configuration
-------------

The yolo and scrfd post processes parameters can be configured by a json file located in $TAPPAS_WORKSPACE/apps/h8/gstreamer/x86_hw_accelerated/video_management_system/resources/configs


Run
---

Exporting ``TAPPAS_WORKSPACE`` environment variable is a must before running the app.

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/x86_hw_accelerated/video_management_system/video_management_system.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/vms_pipeline.gif"/>
   </div>

Models
------

* ``yolov5s_personface_rgbx``: yolov5s pre-trained on Hailo's dataset - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5s_personface.yaml
* ``scrfd_2.5g``: scrfd pre-trained on Hailo's dataset - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/scrfd_2.5g.yaml
* ``arcface_mobilefacenet_rgbx``: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/arcface_mobilefacenet.yaml
* ``face_attr_resnet_v1_18_rgbx``: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/face_attr_resnet_v1_18.yaml
* ``person_attr_resnet_v1_18_rgbx``: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/person_attr_resnet_v1_18.yaml

.. note::
   The networks that are used on TAPPAS differ from the Model-Zoo model:
   - They have an additional RGBX->RGB layer
   - More information on the retraining section

How the application works
-------------------------

This section explains the network switching.

The app builds a gstreamer pipeline (that is explained below) and utilises the ``scheduling-algorithm`` property of its hailonet elements. This lets the hailonet elements know that we wish to switch networks on the same device.
The hailonets perform network switching by blocking their sink pads when it is time to switch: turning off one hailonet and turning on the other. Before turning a hailonet element on, it has to flush the buffers out of the element, this is all handled internaly. `read more about hailonet <../../../../../docs/elements/hailo_net.rst>`_

How the pipeline works
----------------------

This section is optional and provides a drill-down into the implementation of the ``Video Management System` app with a focus on explaining the ``GStreamer`` pipeline.

Pipeline diagram
----------------


.. image:: readme_resources/vms_pipeline.png


The following elements are the structure of the pipeline:


* | ``Pre-Models (Detectors and Trackers)``

  * | ``filesrc`` Reads data from a file in the local file system.
  * | ``qtdemux`` Demuxes the sources and extracts the video.
  * | ``vaapidecodebin`` Decodes the video using VA-API.
  * | ``hailoroundrobin`` Aggregates the streams into 1 stream using roundrobin method.

  * | ``Model 1`` - Face Detection and Tracking.

    * | ``hailocropper`` Filters face configured streams and bypass the FHD.
    * | ``videoscale``  Scales the picture to the detector resolution.
    * | ``hailonet``  Performs the inference on the Hailo-8 device.
      | This intance of hailonet performs scrfd_2.5g network inference for face detection and landmarks.
      | `read more about hailonet <../../../../../docs/elements/hailo_net.rst>`_ 
    * | ``hailofilter`` Performs the given postprocess, chosen with the ``so-path`` property. This instance is in charge of face detection and landmarks processing.
    * | ``hailoaggregator`` waits for all crops belonging to the original frame to arrive and merges all metas into their original frame. So, for example, if the upstream ``hailocropper`` cropped 4 faces from the original frame, then this ``hailoaggregator`` will wait to recieve 4 buffers along with he original frame.
      | `read more about hailoaggregator <../../../../../docs/elements/hailo_aggregator.rst>`_
    * | ``hailotracker`` Performs JDE Tracking using a kalman filter, applying a unique id to tracked persons.
      | This element also receives updates of person/face attributes and associates them to their corresponding tracked person/face.
      | `read more about hailotracker <../../../../../docs/elements/hailo_tracker.rst>`_ 

  * | ``Model 2`` - Person Detection and Tracking.

    * | ``hailocropper`` Filters person configured streams and bypass the FHD.
    * | ``videoscale``  Scales the picture to the detector resolution.
    * | ``hailonet``  This intance of hailonet performs yolov5s network inference for person/face detection.
    * | ``hailofilter`` Performs the given postprocess (yolo detction).
    * | ``hailoaggregator`` waits for all crops belonging to the original frame
    * | ``hailotracker`` Performs JDE Tracking using a kalman filter, applying a unique id to tracked face.
  
  * | ``hailogallery`` - Enables the user to save and compare embeddings(HailoMatrix) that represents recogintion, in order to track objects across multiple streams.
    | In this case, the gallery is used to track pre-saved faces.
    | `read more about hailogallery <../../../../../docs/elements/hailo_gallery.rst>`_ 
  * | ``tee`` - Splits the piepline into two branches. While one buffer continues the drawing and displaying, the other continues to person/face attributes and face recognition.
  
* | ``Display branch``

  * | ``videoscale`` Scales the picture to the compositing resolution.
  * | ``hailostreamrouter`` Deaggregated streams into mutliple streams.
  * | ``hailooverlay`` draws the postprocess results on each frame.
  * | ``videoconvert`` Converts the format of the image.
  * | ``compositor`` Composites multiple streams into one big picture containing an image from each stream.
  * | ``fpsdisplaysink`` Outputs video onto the screen, and displays the current and average framerate.

* | ``Model 3`` - Person Attributes

  * | ``hailocropper`` Crops person detections from the original full HD image and resizes them to the input size of the following ``hailonet`` (Person Attributes). Extra classifications are applied to only pass persons that have not had classified person attributes yet. 
    | `read more about hailocropper <../../../../../docs/elements/hailo_cropper.rst>`_
  * | ``hailonet`` This intance of hailonet performs resnet_v1_18 network inference for Person Attributes classification.
  * | ``hailofilter`` This instance of hailofilter is in charge of Person attributes post processing. The so in this filter is also in charge of updating the tracker with the post-processed classifications of person attributes.
  * | ``hailoaggregator`` waits for all crops belonging to the original frame to arrive and merges all metas into their original frame. So, for example, if the upstream ``hailocropper`` cropped 4 persons from the original frame, then this ``hailoaggregator`` will wait to recieve 4 buffers along with he original frame.
    | `read more about hailoaggregator <../../../../../docs/elements/hailo_aggregator.rst>`_
  * | ``fakesink`` Redirects the image to a fake sink since this image is no longer needed.

* | ``Model 4`` - Face Attributes

  * | ``hailocropper`` Crops Face detections from the original full HD image and resizes them to the input size of the following ``hailonet`` (Face Attributes). Extra classifications are applied to only pass faces that have not had classified Face Attributes yet. 
    | `read more about hailocropper <../../../../../docs/elements/hailo_cropper.rst>`_
  * | ``hailonet`` This intance of hailonet performs resnet_v1_18 network inference for Face Attributes classification.
  * | ``hailofilter`` This instance of hailofilter is in charge of Face attributes post processing. The so in this filter is also in charge of updating the tracker with the post-processed classifications of face attributes.
  * | ``hailoaggregator`` waits for all crops belonging to the original frame to arrive and merges all metas into their original frame. So, for example, if the upstream ``hailocropper`` cropped 4 faces from the original frame, then this ``hailoaggregator`` will wait to recieve 4 buffers along with he original frame.
    | `read more about hailoaggregator <../../../../../docs/elements/hailo_aggregator.rst>`_
  * | ``fakesink`` Redirects the image to a fake sink since this image is no longer needed.

* | ``Model 5`` - Face Recognition

  * | ``hailocropper`` Crops Face detections from the original full HD image and resizes them.
  * | ``hailofilter`` Performs face alignment that ensures that the face is consistently positioned in the same way.
  * | ``hailonet`` This intance of hailonet performs arcface network inference to generate an embedding matrix for each aligned face.
  * | ``hailofilter`` This instance of hailofilter is in charge of arcface face embedding post-process.

  * | ``hailoaggregator`` waits for all crops belonging to the original frame to arrive and merges all metas into their original frame. So, for example, if the upstream ``hailocropper`` cropped 4 faces from the original frame, then this ``hailoaggregator`` will wait to recieve 4 buffers along with he original frame.
  * | ``fakesink`` Redirects the image to a fake sink since this image is no longer needed.

  `read more about Face Recogntion pipeline <../../general/face_recognition/README.rst>`_

Use your own videos and faces in Face Recognition
-------------------------------------------------
To use your own video sources and faces, use ``Face Recgonition Pipeline`` ``- save_faces.sh`` script.
For further instructions see `Face Recogntion pipeline documentation <../../general/face_recognition/README.rst>`_.

Replace the ``resources/face_recognition_local_gallery.json`` file with your own face gallery file.

you can copy the new file in face_recognition app to the following path like this:

.. code-block:: sh

  cp apps/h8/gstreamer/general/face_recognition/resources/gallery/face_recognition_local_gallery.json apps/h8/gstreamer/x86_hw_accelerated/video_management_system/resources/gallery/face_recognition_local_gallery.json

How to use Retraining to replace models
---------------------------------------

.. note:: It is recommended to first read the `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_ page. 

You can use Retraining Dockers (available on Hailo Model Zoo), to replace the following models with ones
that are trained on your own dataset:

- ``yolov5s_personface_rgbx``

  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_models/vehicle_detection/docs/TRAINING_GUIDE.rst>`_

    - **Apply the changes** written on 'on-chip RGBX->RGB layers' section on `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update ``configs/yolov5_personface.json`` with your new post-processing parameters (NMS)

- ``scrfd_2.5g_rgbx``

  - No retraining docker is available.
  - Post process CPP file edit update post-processing:

    - Update `face_detection.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/libs/postprocesses/detection/face_detection.cpp#L609>`_
      (``scrfd()`` fucttion) with your new paremeters, then recompile to create ``libface_detection_post.so``

- ``arcface_mobilefacenet_rgbx``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/arcface>`_

    - **Apply the changes** written on 'on-chip RGBX->RGB layers' section on `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update `arcface.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/apps/x86/vms/postprocesses/arcface.cpp#L19>`_
      with your new paremeters, then recompile to create ``libface_recognition_post.so``


- ``face_attr_resnet_v1_18_rgbx``

  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_models/license_plate_detection/docs/TRAINING_GUIDE.rst>`_

    - **Apply the changes** written on 'on-chip RGBX->RGB layers' section on `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update `face_attributes.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/apps/x86/vms/postprocesses/face_attributes.cpp#L20>`_
      with your new paremeters, then recompile to create ``libface_attributes_post.so``

- ``person_attr_resnet_v1_18_rgbx``

  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_models/video_management_system/docs/TRAINING_GUIDE.rst>`_

    - **Apply the changes** written on 'on-chip RGBX->RGB layers' section on `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update `person_attributes.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/apps/x86/vms/postprocesses/person_attributes.cpp#L20>`_
      with your new paremeters, then recompile to create ``libperson_attributes_post.so``