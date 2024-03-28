
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

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/rockchip/tiling
   ./tiling.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/pipeline_run.gif" width="1000px" height="600px"/>
   </div>


How it works
^^^^^^^^^^^^

This app is based on our `tiling pipeline template <../../../../../docs/pipelines/single_network.rst#example-pipeline-single-network-with-tiling>`_

