
Detection Pipeline
==================

Overview
--------

Our requirement from this pipeline is a real-time high-accuracy object detection to run on a single video stream using an embedded host. The required input video resolution was HD (high definition, 720p).

The chosen platform for this project is based on NXP’s i.MX8M ARM processor. The Hailo-8TM AI processor is connected to it as an AI accelerator.


.. image:: readme_resources/overview.png


Drill Down
----------

Although the i.MX8M is a capable host, processing and decoding real-time HD video is bound to utilize a lot of the CPU’s resources, which may eventually reduce performance. To solve this problem, most of the vision pre-processing pipeline has been offloaded to the Hailo-8 device in our application.

The camera sends the raw video stream, encoded in YUV color format using the YUY2 layout. The data passes through Hailo’s runtime software library, called HailoRT, and through Hailo’s PCIe driver. The data’s format is kept unmodified, and it is sent to the Hailo-8 device as is.

Hailo-8’s NN core handles the data preprocessing, which includes decoding the YUY2 scheme, converting from the YUV color space to RGB and, finally, resizing the frames into the resolution expected by the deep learning detection model.

The Hailo Dataflow Compiler supports adding these pre-processing stages to any model when compiling it. In this case, they are added before the YOLOv5m detection model.


.. image:: readme_resources/full_pipeline.jpg


Options
-------

.. code-block:: sh

   ./detection.sh [--input FILL-ME]


* ``--input`` is an optional flag, path to the video camera used (default is /dev/video2).
* ``--show-fps``  is an optional flag that enables printing FPS on screen.
* ``--print-gst-launch`` is a flag that prints the ready gst-launch command without running it"

Configuration
-------------

The app post process parameters can be configured by a json file located in $TAPPAS_WORKSPACE/apps/gstreamer/imx/detection/resources/configs/yolov5.json


Run
---

.. code-block:: sh

   ./detection.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/pipeline_run.gif" width="600px" height="500px"/>
   </div>


How it works
------------

This app is based on our `single network pipeline template <../../../../docs/pipelines/single_network.rst>`_

Links
-----

`Blog post about this setup <https://hailo.ai/blog/customer-case-study-developing-a-high-performance-application-on-an-embedded-edge-ai-device/>`_
