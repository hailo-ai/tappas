
Detection and Depth Estimation Pipeline
=======================================

Detection and Depth Estimation
------------------------------

``detection_and_depth_estimation.sh`` demonstrates depth estimation and detection on one video file source.
This is done by running two streams on top of GStreamer using one Hailo-8 device with using two ``hailonet`` elements.

Options
-------

.. code-block:: sh

   ./detection_and_depth_estimation.sh [--video-src FILL-ME]


* ``-i --input`` is an optional flag, a path to the video displayed.
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it
* ``--show-fps`` is an optional flag that enables printing FPS on screen

Run
---

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/raspberrypi/multinetworks_parallel/
   ./detection_and_depth_estimation.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/pipeline_run.gif" width="600px" height="250px"/> 
   </div>


Models
------

Joined together:

* ``fast_depth`` in resolution of 224X224X3: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/fast_depth.yaml
* ``mobilenet_ssd`` in resolution of 300X300X3: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/ssd_mobilenet_v1_no_alls.yaml

How it works
------------

This app is based on our `parallel networks pipeline template <../../../../../docs/pipelines/parallel_networks.rst>`_

With small modifications:


#. Use decode elements instead of ``decodebin``
#. Increase the number of threads on the ``videoconvert``
