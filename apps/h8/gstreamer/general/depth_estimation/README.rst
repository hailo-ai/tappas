
Depth Estimation Pipelines
==========================

Depth Estimation
----------------

``depth_estimation.sh`` demonstrates depth estimation on one video file source.
This is done by running a ``single-stream object depth estimation pipeline`` on top of GStreamer using the Hailo-8 device.

Options
-------

.. code-block:: sh

   ./depth_estimation.sh [--video-src FILL-ME]


* ``-i --input`` is an optional flag, a path to the video displayed.
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it
* ``--show-fps`` is an optional flag that enables printing FPS on screen

Run
---

.. code-block:: sh

   cd /local/workspace/tappas/apps/h8/gstreamer/general/depth_estimation
   ./depth_estimation.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/depth_estimation_run.gif" width="600px" height="250px"/> 
   </div>


Model
-----


* ``fast_depth`` in resolution of 224X224X3: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/fast_depth.yaml

How it works
------------

This app is based on our `single network pipeline template <../../../../../docs/pipelines/single_network.rst>`_

With two modifications to the template:


#. We use ``tee`` to show two screens, one with the depth estimation mask applied and one without
#. The network expect no borders, so an ``aspectratiocrop`` mechanism is needed ``aspectratiocrop aspect-ratio=1/1``
