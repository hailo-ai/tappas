# HailoFilter

- [HailoFilter](#hailofilter)
  - [Overview](#overview)
    - [Parameters](#parameters)
  - [Hierarchy](#hierarchy)

## Overview

Hailofilter is an element which enables the user to apply a postprocess or drawing operation to a frame and its tensors. It provides an entry point for a compiled .so file that the user writes, inside of which they will have access to the original image frame, the tensors output by the network for that frame, and any metadata attached. At first the hailofilter will read the buffer from the sink pad, then apply the filter defined in the provided .so, until finally sending the filtered buffer along the source pad to continue down the pipeline.

### Parameters
The most important parameter here is the `so-path`. Here you provide the path to your compiled .so that applies your wanted filter. \
By default, the hailofilter will call on a filter() function within the .so as the entry point. If your .so has multiple entry points, for example in the case of slightly different network flavors, then you can chose which specific filter function to apply via the `function-name` parameter. \
As a member of the GstVideoFilter hierarchy, the hailofilter element supports qos ([Quality of Service](https://gstreamer.freedesktop.org/documentation/plugin-development/advanced/qos.html?gi-language=c)). Although qos typically tries to garuantee some level of performance, it can lead to frames dropping. For this reason it is `advised to always set qos=false` to avoid either tensors being dropped or not drawn.

## Hierarchy
```
GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstBaseTransform
                         +----GstVideoFilter
                               +----GstHailoFilter

Pad Templates:
  SINK template: 'sink'
    Availability: Always
    Capabilities:
      video/x-raw
                 format: { (string)RGB, (string)YUY2 }
                  width: [ 1, 2147483647 ]
                 height: [ 1, 2147483647 ]
              framerate: [ 0/1, 2147483647/1 ]
  
  SRC template: 'src'
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
                        String. Default: "hailofilter0"
  parent              : The parent of the object
                        flags: readable, writable
                        Object of type "GstObject"
  qos                 : Handle Quality-of-Service events
                        flags: readable, writable
                        Boolean. Default: true
  debug               : debug
                        flags: readable, writable, controllable
                        Boolean. Default: false
  so-path             : Location of the so file to load
                        flags: readable, writable, changeable only in NULL or READY state
                        String. Default: null
  function-name       : function-name
                        flags: readable, writable, changeable only in NULL or READY state
                        String. Default: "filter"
```
