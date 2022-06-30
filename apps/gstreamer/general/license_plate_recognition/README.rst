License Plate Recognition
=========================

Overview
--------

``license_plate_recognition.sh`` demonstrates model scheduling in a complex pipeline with inference based decision making. The overall task is to ``detect and track vehicles`` in the pipeline and then ``detect/extract license plate numbers`` from newly tracked instances.

Options
-------

.. code-block:: sh

   ./license_plate_recognition.sh [--show-fps]


* ``--show-fps``  is an optional flag that enables printing FPS on screen.

Configuration
-------------

The yolo post processes parameters can be configured by a json file located in $TAPPAS_WORKSPACE/apps/gstreamer/general/license_plate_recognition/resources/configs


Run
---

Exporting ``TAPPAS_WORKSPACE`` environment variable is a must before running the app.

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/gstreamer/general/license_plate_recognition/license_plate_recognition.sh

The output should look like:


.. raw:: html

   <div align="center">
       <img src="readme_resources/lpr_pipeline.gif"/>
   </div>


How the application works
-------------------------

This app uses HailoRT Model Scheduler, read more about HailoRT Model Scheduler GStreamer integration at `HailoNet  <../../../../docs/elements/hailo_net.rst>`_