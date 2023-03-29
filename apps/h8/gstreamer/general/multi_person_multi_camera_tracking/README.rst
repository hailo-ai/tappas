Multi Person Multi Camera Tracking Application
==============================================

Overview
--------

``multi_person_multi_camera_tracking.sh`` demonstrates network switching in a complex pipeline with inference based decision-making. 
The overall task is to ``track people across multiple cameras`` in the pipeline with multiple streams. 
The pipeline is comprised of two general networks: Person/Face Detection, and RE-ID network.
The networks run on the Hailo device in parallel where the HailoRT model scheduler handles which network gets running time.
When newly tracked persons are detected (single class yolov5s) in, Each where a RE-ID network is run for each person and than a comparison is made across prior persons to RE-ID the person.
Once a Person is RE-ID'd, the pipeline updates the ``hailotracker`` JDE Tracking element upstream with the new global ID for the corresponding person.
From there the person is tracked along with its global id, and is re-inferred on new frames.

Options
-------

.. code-block:: sh

   ./multi_person_multi_camera_tracking.sh [--show-fps]


* ``--show-fps``  is an optional flag that enables printing FPS on screen.
* ``--num-of-sources`` sets the number of video sources to use by given input. the default, recommended and maximal value in this pipeline is 4 sources"

Run
---

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/multi_person_multi_camera_tracking
   ./multi_person_multi_camera_tracking.sh

Expected output:


.. raw:: html

   <div align="center">
       <img src="readme_resources/re_id.gif"/>
   </div>

Models
------


* ``yolov5s_personface``: yolov5s pre-trained on Hailo's dataset - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5s_personface.yaml
* ``repvgg_a0_person_reid_2048``: repvgg_a0_person_reid_2048 pre-trained on Hailo's dataset - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/repvgg_a0_person_reid_2048.yaml

How the application works
-------------------------

The app is based on the `Cascaded Networks Structure  <../../../../../docs/pipelines/cascaded_nets.rst>`
In addition it uses the `HailoTracker <../../../../../docs/elements/hailo_tracker.rst>` and the new `HailoGallery Element <../../../../../docs/elements/hailo_gallery.rst>` gstreamer elements.
These elements manage to track the persons over the same stream, and across the multiple streams respectively.
Note that we are not using the regular `HailoOverlay  <../../../../../docs/elements/hailo_overlay.rst>`,
Instead we are using `HailoFilter <../../../../../docs/elements/hailo_filter.rst>` to draw our results
since we want to also blur the faces of the persons in the picture and draw different color for each different person.

Pipeline diagram
----------------

.. image:: readme_resources/re_id_pipeline.png

How to use Retraining to replace models
---------------------------------------

.. note:: It is recommended to first read the `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_ page. 

You can use Retraining Dockers (available on Hailo Model Zoo), to replace the following models with ones
that are trained on your own dataset:

- ``yolov5s_personface``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_models/personface_detection/docs/TRAINING_GUIDE.rst>`_
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update ``resources/configs/yolov5_personface.json`` with your new post-processing parameters (NMS)
- ``repvgg_a0_person_reid_2048``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_models/reid/docs/TRAINING_GUIDE.rst>`_
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update `re_id.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/libs/postprocesses/re_id/re_id.cpp#L32>`_
      with your new paremeters, then recompile to create ``libre_id.so``