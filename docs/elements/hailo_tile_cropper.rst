
Hailo Tile Cropper
===================

Overview
--------

``HailoTileCropper`` is a derived element of `hailoCropper <hailo_cropper.rst>`_.
It overrides the default ``prepare_crops`` behaviour to return a vector of tile regions of interest, and allows splitting the incoming frame into tiles by rows and columns.
Each tile stores their x, y, width, and height (with overlap between tiles included) in the full frame.
Just like the base HailoCropper, the full original frame is sent to the first src pad while all the cropped images are sent to the second.

`hailoaggregator <hailo_aggregator.rst>`_ wiil aggregate the cropped tiles and stitch them back to the original resolution.

Parameters
^^^^^^^^^^


* tiles-along-x-axis  : Number of tiles along x axis (columns) - default 2
* tiles-along-y-axis  : Number of tiles along x axis (rows) - default 2
* overlap-x-axis      : Overlap in percentage between tiles along x axis (columns) - default 0
* overlap-y-axis      : Overlap in percentage between tiles along y axis (rows) - default 0
* tiling-mode         : Tiling mode (0 - single-scale, 1 - multi-scale) - default 0
* scale-level         : Scales (layers of tiles) in addition to the main layer 1: [(1 X 1)] 2: [(1 X 1), (2 X 2)] 3: [(1 X 1), (2 X 2), (3 X 3)]] - default 2

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
                      +----GstHailoBaseCropper
                            +----GstHailoTileCropper
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
     SRC: 'src_0'
       Pad Template: 'src'
     SRC: 'src_1'
       Pad Template: 'src'

   Element Properties:
     name                : The name of the object
                           flags: readable, writable
                           String. Default: "hailotilecropper0"
     parent              : The parent of the object
                           flags: readable, writable
                           Object of type "GstObject"
     internal-offset     : 
                           Whether to use Gstreamer offset of internal offset.
                           NOTE: If using file sources, Gstreamer does not generate offsets for buffers, 
                           so this property should be set to true in such cases.
                           flags: readable, writable, controllable
                           Boolean. Default: false
     tiles-along-x-axis  : Number of tiles along x axis (columns)
                           flags: readable, writable, changeable only in NULL or READY state
                           Unsigned Integer. Range: 1 - 20 Default: 2
     tiles-along-y-axis  : Number of tiles along x axis (rows)
                           flags: readable, writable, changeable only in NULL or READY state
                           Unsigned Integer. Range: 1 - 20 Default: 2
     overlap-x-axis      : Overlap in percentage between tiles along x axis (columns)
                           flags: readable, writable, changeable only in NULL or READY state
                           Float. Range:               0 -               1 Default:               0
     overlap-y-axis      : Overlap in percentage between tiles along y axis (rows)
                           flags: readable, writable, changeable only in NULL or READY state
                           Float. Range:               0 -               1 Default:               0
     tiling-mode         : Tiling mode
                           flags: readable, writable
                           Enum "GstHailoTileCropperTilingMode" Default: 0, "single-scale"
                              (0): single-scale     - Single Scale
                              (1): multi-scale      - Multi Scale
     scale-level         : 1: [(1 X 1)] 2: [(1 X 1), (2 X 2)] 3: [(1 X 1), (2 X 2), (3 X 3)]]
                           flags: readable, writable, changeable only in NULL or READY state
                           Unsigned Integer. Range: 1 - 3 Default: 2
