
Hailo Overlay
==============

Overview
--------

HailoOverlay is a drawing element that can draw postprocessed results on an incoming video frame.
This element supports the following results:


* Detection - Draws the rectengle over the frame, with the label and confidence (rounded).
* Classification - Draws a classification over the frame, at the top left corner of the frame.
* Landmarks - Draws a set of points on the given frame at the wanted coordintates.
* Tiles - Can draw tiles as a thin rectengle.

Parameters
^^^^^^^^^^

As a member of the GstBaseTransform hierarchy, the hailooverlay element supports qos (\ `Quality of Service <https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/qos.html?gi-language=c>`_\ ). Although qos typically tries to guarantee some level of performance, it can lead to frames dropping. For this reason it is advised to always set ``qos=false`` to avoid either tensors being dropped or not drawn.

Hierarchy
---------

.. code-block::

   GObject
    +----GInitiallyUnowned
          +----GstObject
                +----GstElement
                      +----GstBaseTransform
                            +----GstHailoOverlay

   Pad Templates:
     SINK template: 'sink'
       Availability: Always
       Capabilities:
         video/x-raw
                    format: { (string)RGB }
                     width: [ 1, 2147483647 ]
                    height: [ 1, 2147483647 ]
                 framerate: [ 0/1, 2147483647/1 ]

     SRC template: 'src'
       Availability: Always
       Capabilities:
         video/x-raw
                    format: { (string)RGB }
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
                           String. Default: "hailooverlay0"
     parent              : The parent of the object
                           flags: readable, writable
                           Object of type "GstObject"
     qos                 : Handle Quality-of-Service events
                           flags: readable, writable
                           Boolean. Default: false
