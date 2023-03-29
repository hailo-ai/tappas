
Tiling Pipeline
===============

Single Scale Tiling
-------------------

Single scale tiling FHD Gstreamer pipeline demonstrates splitting each frame into several tiles which are processed independently by ``hailonet`` element.
This method is especially effective for detecting small objects in high-resolution frames.

Model
^^^^^


* ``ssd_mobilenet_v1_visdrone`` in resolution of 300X300: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/ssd_mobilenet_v1_visdrone.yaml

The VisDrone dataset consists of only small objects which we can assume are always confined within an single tile. As such it is better suited for running single-scale tiling with little overlap and without additional filtering.

Options
^^^^^^^

.. code-block:: sh

   ./tiling.sh [OPTIONS] [-i INPUT_PATH]


* ``-i --input`` is an optional flag, a path to the video file displayed.
* ``--print-gst-launch`` prints the ready gst-launch command without running it
* ``--show-fps``  optional - enables printing FPS on screen
* ``--tiles-x-axis`` optional - set number of tiles along x axis (columns)
* ``--tiles-y-axis`` optional - set number of tiles along y axis (rows)
* ``--overlap-x-axis`` optional - set overlap in percentage between tiles along x axis (columns)
* ``--overlap-y-axis`` optional - set overlap in percentage between tiles along y axis (rows)
* ``--iou-threshold`` optional - set iou threshold for NMS.
* ``--sync-pipeline`` optional - set pipeline to sync to video file timing.

Run
^^^

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/tiling
   ./tiling.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/pipeline_run.gif" width="1000px" height="600px"/>
   </div>


How it works
^^^^^^^^^^^^

This app is based on our `tiling pipeline template <../../../../../docs/pipelines/single_network.rst#example-pipeline-single-network-with-tiling>`_

Multi Scale Tiling
------------------

Multi-scale tiling FHD Gstreamer pipeline demonstrates a case where the video and the training dataset includes objects in different sizes. Dividing the frame to small tiles might miss large objects or “cut" them to small objects.
The solution is to split each frame into number of scales (layers) each includes several tiles.

Multi-scale tiling strategy also allows us to filter the correct detection over several scales.
For example we use 3 sets of tiles at 3 different scales:


* Large scale, one tile to cover the entire frame (1x1)
* Medium scale dividing the frame to 2x2 tiles.
* Small scale dividing the frame to 3x3 tiles.

In this mode we use 1 + 4 + 9 = 14 tiles for each frame.
We can simplify the process by highliting the main tasks:
crop -> inference -> post-process -> aggregate → remove exceeded boxes → remove large landscape → perform NMS

Model
^^^^^


* ``mobilenet_ssd`` in resolution of 300X300X3: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/ssd_mobilenet_v1.yaml

Options
^^^^^^^

.. code-block:: sh

   ./multi_scale_tiling.sh [OPTIONS] [-i INPUT_PATH]


* ``-i --input`` is an optional flag, a path to the video file displayed.
* ``--print-gst-launch`` prints the ready gst-launch command without running it
* ``--show-fps``  optional - enables printing FPS on screen
* ``--tiles-x-axis`` optional - set number of tiles along x axis (columns)
* ``--tiles-y-axis`` optional - set number of tiles along y axis (rows)
* ``--overlap-x-axis`` optional - set overlap in percentage between tiles along x axis (columns)
* ``--overlap-y-axis`` optional - set overlap in percentage between tiles along y axis (rows)
* ``--iou-threshold`` optional - set iou threshold for NMS.
* ``--border-threshold`` optional - set border threshold to Remove tile's exceeded objects.
* ``--scale-level`` optional - set scales (layers of tiles) in addition to the main layer. 1: [(1 X 1)] 2: [(1 X 1), (2 X 2)] 3: [(1 X 1), (2 X 2), (3 X 3)]]'

Run
^^^

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/tiling
   ./multi_scale_tiling.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/multi_scale_tiling.gif" width="1000px" height="600px"/>
   </div>


How it works
^^^^^^^^^^^^

As multi scale tiling is almost equal to single scale i will mention the differences:

.. code-block:: sh

   TILE_CROPPER_ELEMENT="hailotilecropper internal-offset=$internal_offset name=cropper tiling-mode=1 scale-level=$scale_level

``hailotilecropper`` sets ``tiling-mode`` to 1 (0 - single-scale, 1 - multi-scale) and ``scale-level`` to define what is the structure of scales/layers in addition to the main scale.

``hailonet`` hef-path is ``mobilenet_ssd`` which is training dataset includes objects in different sizes.

.. code-block:: sh

    hailotileaggregator flatten-detections=true iou-threshold=$iou_threshold border-threshold=$border_threshold name=agg

 ``hailotileaggregator`` sets ``border-threshold`` used in remove tile's exceeded objects process.

How to use Retraining to replace models
---------------------------------------

.. note:: It is recommended to first read the `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_ page. 

You can use Retraining Dockers (available on Hailo Model Zoo), to replace the following models with ones
that are trained on your own dataset:

- ``mobilenet_ssd``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/ssd>`_
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update `mobilenet_ssd.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/libs/postprocesses/detection/mobilenet_ssd.cpp#L141>`_
      with your new paremeters, then recompile to create ``libmobilenet_ssd_post.so``