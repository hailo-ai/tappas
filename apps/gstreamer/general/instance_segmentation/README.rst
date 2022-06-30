
Instance Segmentation Pipeline
==============================

Overview
--------

``instance_segmentation.sh`` demonstrates instance segmentation on one video file source and verifies Hailo’s configuration.
 This is done by running a ``single-stream instance segmentation pipeline`` on top of GStreamer using the Hailo-8 device.

Options
-------

.. code-block:: sh

   ./instance_segmentation.sh [--input FILL-ME]


* ``--input`` is an optional flag, a path to the video displayed (default is detection.mp4).
* ``--show-fps``  is an optional flag that enables printing FPS on screen.
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it"

Run
---

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/gstreamer/general/instance_segmentation
   ./instance_segmentation.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/instance_segmentation_run.gif" width="640px" height="360px"/>
   </div>


How it works?
-------------

This app is based on our `single network pipeline template <../../../../docs/pipelines/single_network.rst>`_
