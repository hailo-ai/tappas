Classification Pipeline
=======================

Classification
--------------

The purpose of ``classification.sh`` is to demonstrate classification on one video file source.
 This is done by running a ``single-stream object classification pipeline`` on top of GStreamer using the Hailo-8 device.

.. raw:: html
  
  <div align="center"><img src="readme_resources/pipeline_run.gif"/></div>


Options
-------

.. code-block:: sh

   ./classification.sh [--input FILL-ME]


* ``--input`` is an optional flag, a path to the video displayed (default is classification_movie.mp4).
* ``--show-fps`` is a flag that prints the pipeline's fps to the screen.
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it.

Supported Networks
------------------


* 'resnet_v1_50' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/resnet_v1_50.yaml

Run
---

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/classification
   ./classification.sh


.. raw:: html
   
  <h2>The output should look like:</h2>

  <div align="center"><img src="readme_resources/pipeline_run.gif" width="600px" height="500px"/></div>


How does it work?
-----------------

This app is based on our `single network with resolution preservation pipeline template <../../../../../docs/pipelines/single_network.rst#example-pipeline-with-resolution-preservation>`_
