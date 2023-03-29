
Facial Landmarks Pipeline
=========================

Overview
--------

``facial_landmarks.sh`` demonstrates facial landmarking on one video file source and verifies Hailo’s configuration.
 This is done by running a ``single-stream facial landmarking pipeline`` on top of GStreamer using the Hailo-8 device.

Options
-------

.. code-block:: sh

   ./face_landmarks.sh


* ``--input`` is an optional flag, a path to the video displayed (default is faces_120_120.mp4).
* ``--show-fps``  is an optional flag that enables printing FPS on screen.
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it"

Run
---

.. code-block:: sh

   ./face_landmarks.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/face_landmarks_pipeline.gif" width="600px" height="500px"/>
   </div>


Model
-----


* ``tddfa_mobilenet_v1`` in resolution of 120X120X3 - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/tddfa_mobilenet_v1.yaml

How does it work?
-----------------

This app is based on our `single network pipeline template <../../../../../docs/pipelines/single_network.rst>`_
