
Face Detection and Facial Landmarks Pipeline
============================================

Face Detection and Facial Landmarks
-----------------------------------

``face_detection_and_landmarks.sh`` demonstrates face detection and    facial landmarking on one video file source.
 This is done by running a face detection pipeline (infer + postprocessing), cropping and scaling all detected faces, and sending them into z 2nd network of facial landmarking. All resulting detections and landmarks are then aggregated and drawn on the original frame. The two networks are running using one Hailo-8 device with two ``hailonet`` elements.

Options
-------

.. code-block:: sh

   ./face_detection_and_landmarks.sh [OPTIONS] [-i INPUT_PATH]


* ``-i --input`` is an optional flag, a path to the video displayed.
* ``--print-gst-launch`` prints the ready gst-launch command without running it
* ``--show-fps``  optional - enables printing FPS on screen

Run
---

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/raspberrypi/cascading_networks
   ./face_detection_and_landmarks.sh

Model
-----

Joined together:

* ``lightface_slim`` in resolution of 320X240X3 - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/lightface_slim.yaml
* ``tddfa_mobilenet_v1`` in resolution of 120X120X3 - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/tddfa_mobilenet_v1.yaml

How it works
------------

This app is based on our `cascaded networks pipeline template <../../../../../docs/pipelines/cascaded_nets.rst>`_

With small modifications:


#. Use decode elements instead of ``decodebin``
#. Increase the number of threads on the ``videoconvert``
