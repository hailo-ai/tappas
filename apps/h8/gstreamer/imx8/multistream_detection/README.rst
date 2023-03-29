
Multi-Stream object detection Pipeline
======================================

Overview
--------

This GStreamer pipeline demonstrates object detection on multiple files streams.

All the streams are processed in parallel through the decode and scale phases, and enter the Hailo device frame by frame.

Afterwards postprocess and drawing phases add the classified object and bounding boxes to each frame. 
The last step is to match each frame back to its respective stream and output all of them to the display.

All the elements in this pipeline operate in NV12 format, and the HEF file is also IN NV12 format, so no format conversion is performed.

Run the pipeline
----------------

.. code-block:: sh

   ./multi_stream_detection.sh


#. ``--show-fps`` Prints the fps to the output.
#. ``--num-of-sources`` Sets the number of sources to use by given input. The default and maximum value in this pipeline is 6
#. ``--fakesink``  Run the application without display
#. ``--print-gst-launch``  Print the ready gst-launch command without running it


Configuration
-------------

The app post process parameters can be configured by a json file located in $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/multistream_detection/resources/configs/yolov5_personface.json


Supported Networks
------------------

* 'yolov5m_wo_spp' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m_wo_spp_60p.yaml

Overview of the pipeline
------------------------

This app is based on our `multi stream pipeline template <../../../../../docs/pipelines/multi_stream.rst>`_

.. note::
    This pipeline only works properly with H265 encoded files due to the faulty behavior of the imx8 H264 decoder. 
    To ensure optimal behavior, please refrain from using H264 files as sources for this pipeline.