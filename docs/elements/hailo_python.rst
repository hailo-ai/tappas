
Hailo Python
==============

Overview
--------

HailoPython is an element which enables the user to apply processing operations to an image via python.
It provides an entry point for a python module that the user writes, inside of which they will have access to the Hailo raw-output (output tensors) and postprocessed-outputs (detections, classifications etc..) as well as the gstreamer buffer. The python function will be called for each buffer going through the hailopython element.

Parameters
^^^^^^^^^^

The two parameters that define the function to call are ``module`` and ``function`` for the module path and function name respectively.
In addition, as a member of the GstVideoFilter hierarchy, the hailofilter element supports qos (\ `Quality of Service <https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/qos.html?gi-language=c>`_\ ). Although qos typically tries to guarantee some level of performance, it can lead to frames dropping. For this reason it is advised to always set ``qos=false`` to avoid either tensors being dropped or not drawn.

Hierarchy
---------

.. code-block::

   GObject
    +----GInitiallyUnowned
          +----GstObject
                +----GstElement
                      +----GstBaseTransform
                            +----GstVideoFilter
                                  +----GstHailoPython

   Pad Templates:
     SRC template: 'src'
       Availability: Always
       Capabilities:
         video/x-raw
                    format: { (string)RGB, (string)YUY2 }
                     width: [ 1, 2147483647 ]
                    height: [ 1, 2147483647 ]
                 framerate: [ 0/1, 2147483647/1 ]

     SINK template: 'sink'
       Availability: Always
       Capabilities:
         video/x-raw
                    format: { (string)RGB, (string)YUY2 }
                     width: [ 1, 2147483647 ]
                    height: [ 1, 2147483647 ]
                 framerate: [ 0/1, 2147483647/1 ]

   Element has no clocking capabilities.
   Element has no URI handling capabilities.

   Pads:
     SINK: 'sink'
       Pad Template: 'sink'
     SRC: 'src'
       Pad Template: 'src'

   Element Properties:
     name                : The name of the object
                           flags: readable, writable
                           String. Default: "hailopython0"
     parent              : The parent of the object
                           flags: readable, writable
                           Object of type "GstObject"
     qos                 : Handle Quality-of-Service events
                           flags: readable, writable
                           Boolean. Default: true
     module              : Python module name
                           flags: readable, writable
                           String. Default: "/local/workspace/tappas/processor.py"
     function            : Python function name
                           flags: readable, writable
                           String. Default: "run"
