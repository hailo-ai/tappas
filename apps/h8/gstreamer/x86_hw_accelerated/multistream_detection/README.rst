
x86 Accelerated Multi-Stream Pipeline
======================================

Overview
--------

This GStreamer pipeline demonstrates Person+Face detection on multiple video streams.

All the streams are processed in parallel through the decode and scale phases, and enter the Hailo device frame by frame.

Afterwards postprocess and drawing phases add the classified object and bounding boxes to each frame.
The last step is to match each frame back to its respective stream and output all of them to the display.

In this pipeline the decoding phase is accelerated by `VA-API <../README.rst>`_.

Prerequisites
-------------

* RSC101
* Ubuntu 22.04 (or docker with ubuntu 22.04)
* Hailo-8 device connected via PCIe

Preparations
------------



Run the pipeline
----------------

.. code-block:: sh

   ./multistream_detection.sh


#. ``--show-fps`` Prints the fps to the output.
#. ``--num-of-sources`` Sets the number of sources to use by given input. The default and recommended value in this pipeline is 12 (for files) or 8 (for camera streams over RTSP protocol)"
#. ``--network`` If specified, set the network to use. Choose from [yolov5, yolox, yolov8], default is yolov5.
#. ``--device-count`` If specified, set the number of devices to use. Default (and maximum value) is the minimum between 4 and the number of devices on machine.

The output should look like:


.. image:: readme_resources/example.png
   :width: 300px 
   :height: 250px


Supported Networks
------------------

* 'yolov8m' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov8m.yaml
* 'yolov5m_wo_spp_60p' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolov5m_wo_spp_60p.yaml
* 'yolovx_l_leaky' - https://github.com/hailo-ai/hailo_model_zoo/blob/master/hailo_model_zoo/cfg/networks/yolovx_l_leaky.yaml

Overview of the pipeline
------------------------

These apps are based on our `multi stream pipeline template <../../../../../docs/pipelines/multi_stream.rst>`_

Example of HailoRT Stream Multiplexer 
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


* This application shows the usage of the HailoRT Stream Multiplexer. This feature controls the time shared on the Hailo device between all streams. The Stream Multiplexer is enabled by the ``Hailonet`` scheduling-algorithm property when in use in multiple ``Hailonet`` elements that run the same HEF file. When the Stream Multiplexer is in use, there is no need to use ``funnel`` and ``streamiddemux`` like elements because the logic is handled internally.

Using Retraining to Replace Models
----------------------------------

.. note:: It is recommended to first read the `Retraining TAPPAS Models <../../../../../docs/write_your_own_application/retraining-tappas-models.rst>`_ page. 

Retraining Dockers (available on Hailo Model Zoo), can be used to replace the following models with ones
that are trained on the users own dataset:

- ``yolov8m``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/yolov8>`_

    - For optimum compatibility and performance with TAPPAS, use for compilation the corresponding YAML file from above.
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file

- ``yolov5m``
  
  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/yolov5>`_

    - For optimum compatibility and performance with TAPPAS, use for compilation the corresponding YAML file from above.
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file

- ``yolox_l_leaky``

  - `Retraining docker <https://github.com/hailo-ai/hailo_model_zoo/tree/master/training/yolovx>`_

   - For optimum compatibility and performance with TAPPAS, use for compilation the corresponding YAML file from above.
  - TAPPAS changes to replace model:

    - Update HEF_PATH on the .sh file