Face Recognition Pipeline
=========================

Overview
--------
``face_recognition.sh`` demonstrates face recognition pipeline on a video stream.

Options
-------

.. code-block:: sh

   ./face_recognition.sh


* ``-i --input`` is an optional flag, a path to an input source (video file / camera device)
* ``--print-gst-launch`` prints the ready gst-launch command without running it
* ``--show-fps`` is an optional flag that enables printing FPS on screen
* ``--network``  to set which network to use. choose from [scrfd_10g, scrfd_2.5g], default is scrfd_10g"

Models
------
* ``scrfd_10g``: scrfd pre-trained on Hailo's dataset - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/scrfd_10g.yaml
* ``scrfd_2.5g``: scrfd pre-trained on Hailo's dataset - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/scrfd_2.5g.yaml
* ``arcface_mobilefacenet``: https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/arcface_mobilefacenet.yaml

Run
---

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/face_recognition/
   ./face_recognition.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/face_recognition.gif" width="640px" height="360px"/>
   </div>


How does it work?
-----------------

The pipeline is devided to 4 steps:

1. Face detection and landmarks:
    Detect faces and predict the locations of key facial landmarks (such as eyes, nose, and mouth), in the video stream.
    The scrfd_10g network is more accurate but computationally heavier, while the scrfd_2.5g network is less accurate but more lightweight, providing better performance.

2. Face alignment:
    This step involves using the detected landmarks and the original video frame to compute an affine transformation that aligns the face with a predefined destination matrix.
    This ensures that the face is consistently positioned in the same way for the next step in the pipeline.

3. Embedding matrix:
    Run Arcface network to generate an embedding matrix for each aligned face. 
    An embedding is a compact representation of the face that captures its unique characteristics. This embedding can then be compared to other embeddings to determine the similarity between faces.

4. Gallery:
    Use the generated embeddings to find the closest matching face in the local database (named "Local Gallery" and stored in a JSON file). This allows the application to identify the person in the video stream.

How to save faces to the local gallery?
---------------------------------------
The local gallery file ``face_recognition_local_gallery.json`` is stored under ``apps/h8/gstreamer/general/face_recognition/resources/gallery`` directory.
It contains the embeddings of the faces.

To add faces to the gallery, you can use the ``save_face.sh`` script.

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/face_recognition/
   ./save_faces.sh

Options:

* ``--network``  to set which network to use. choose from [scrfd_10g, scrfd_2.5g], default is scrfd_10g
* ``--clean``    to clean the local gallery file enitrely

The script goes over the ``.png`` files in ``resources/faces`` directory, and saves each face into the gallery.
The name of the face is determined by the file name.

To use your own video sources and faces, add your images to the ``resources/faces`` directory and remove the original ones.
Make sure to use ``.png`` format image files and a file name including the name of the person.
Also use --clean option to order the script to clean the gallery file before saving the new faces.

How to use Retraining to replace models
---------------------------------------

.. note:: It is recommended to first read the `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_ page. 

You can use Retraining Dockers (available on Hailo Model Zoo), to replace the following models with ones
that are trained on your own dataset:

- ``scrfd_10g``
  
  - No retraining docker is available.
  - Post process CPP file edit update post-processing:

    - Update `face_detection.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/libs/postprocesses/detection/face_detection.cpp#L609>`_
      (``scrfd()`` fucttion) with your new paremeters, then recompile to create ``libface_detection_post.so``
- ``scrfd_2.5g``
  
  - No retraining docker is available.
  - Post process CPP file edit update post-processing:

    - Update `face_detection.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/libs/postprocesses/detection/face_detection.cpp#L609>`_
      (``scrfd()`` fucttion) with your new paremeters, then recompile to create ``libface_detection_post.so``
- ``arcface_mobilefacenet``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/arcface>`_
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file
    - Update `arcface.cpp <https://github.com/hailo-ai/tappas/blob/master/core/hailo/apps/x86/vms/postprocesses/arcface.cpp#L19>`_
      with your new paremeters, then recompile to create ``libface_recognition_post.so``
