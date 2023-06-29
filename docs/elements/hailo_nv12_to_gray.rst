
Hailo NV12 to Gray
==================

Overview
--------

Hailonv12togray is an element that performs conversion of NV12 format to GRAY8 format, while preserving the initial NV12 buffer as the metadata of the resulting GRAY8 buffer. It is a mirror to the hailograytonv12 element.\
It is worth noting that this process does not involve any buffer copies, enabling high performance.\
This element is particularly useful for NV12 pipelines that want to use a GRAY8 formatted HEF.\
In such cases, the hailonv12togray element should be placed before the hailonet element, and the hailograytonv12 element should be placed after the hailonet element, to retrieve the original NV12 buffer.

Hierarchy
---------

.. code-block::

  GObject
  +----GInitiallyUnowned
        +----GstObject
              +----GstElement
                    +----GstBaseTransform
                          +----GstHailonv12togray

  Pad Templates:
    SRC template: 'src'
      Availability: Always
      Capabilities:
        video/x-raw
                  format: { (string)GRAY8 }
                   width: [ 1, 2147483647 ]
                  height: [ 1, 2147483647 ]
               framerate: [ 0/1, 2147483647/1 ]
    
    SINK template: 'sink'
      Availability: Always
      Capabilities:
        video/x-raw
                  format: { (string)NV12 }
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
                          String. Default: "hailonv12togray0"
    parent              : The parent of the object
                          flags: readable, writable
                          Object of type "GstObject"
    qos                 : Handle Quality-of-Service events
                          flags: readable, writable
                          Boolean. Default: false