
Hailo Tile Aggregator
======================

Overview
--------

``HailoTileAggregator`` is a derived element of `hailoAggregator <hailo_aggregator.rst>`_.
A complement to the `HailoTileCropper <hailo_tile_cropper.rst>`_\ , the two elements work together to form a versatile tiling apps.

The element extends two methods of the parent element:


* ``handle_sub_frame_roi``\ : Functionality to perform for each incoming sub frame.
  .. code-block::

                           Performs ``remove_exceeded_bboxes`` (remove boxes close to boundary - using given border_threshold) and then parent element performs flatten detections.

* ``post_aggregation``\ : Functionality to perform after all frames are aggregated succesfully.
  .. code-block::

                       Performs ``remove_large_landscape`` and ``NMS``.

Example
-------


.. image:: ../resources/tiling_pipeline.png

Hierarchy
---------

.. code-block:: sh

   GObject
    +----GInitiallyUnowned
          +----GstObject
                +----GstElement
                      +----GstHailoAggregator
                            +----GstHailoTileAggregator

   Pad Templates:
     SRC template: 'src'
       Availability: Always
       Capabilities:
         ANY

     SINK template: 'sink'
       Availability: Always
       Capabilities:
         ANY

   Element has no clocking capabilities.
   Element has no URI handling capabilities.

   Pads:
     SINK: 'sink_0'
       Pad Template: 'sink'
     SINK: 'sink_1'
       Pad Template: 'sink'
     SRC: 'src'
       Pad Template: 'src'

   Element Properties:
     name                : The name of the object
                           flags: readable, writable
                           String. Default: "hailotileaggregator0"
     parent              : The parent of the object
                           flags: readable, writable
                           Object of type "GstObject"
     flatten-detections  : perform a 'flattening' functionality on the detection metadata when receiving each frame
                           flags: readable, writable, changeable only in NULL or READY state
                           Boolean. Default: false
     iou-threshold       : threshold
                           flags: readable, writable, changeable only in NULL or READY state
                           Float. Range:               0 -               1 Default:             0.3
     border-threshold    : border threshold
                           flags: readable, writable, changeable only in NULL or READY state
                           Float. Range:               0 -               1 Default:             0.1
     remove-large-landscape: remove large landscape objects when running in multi-scale mode
                           flags: readable, writable, changeable only in NULL or READY state
