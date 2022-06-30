
Hailo DeviceStats
==================

Overview
--------

Hailodevicestats is an element that samples power and temperature. It doesn't have any pads, it just has to be part of the pipeline. An example for using this element could be found under the ``detection`` / ``multistream_multidevice`` app.

Parameters
^^^^^^^^^^

Determine the time period between samples with the ``interval`` property.

Choose device with the ``device-id`` property.

Hierarchy
---------

.. code-block::

   GObject
    +----GInitiallyUnowned
          +----GstObject
                +----GstElement
                      +----GstHailoDeviceStats


   Factory Details:
     Rank                     primary (256)
     Long-name                hailodevicestats element
     Klass                    Hailo/Device
     Description              Log Hailo-8 device statistics
     Author                   Omer Salem <omers@hailo.ai>


   Pad Templates:

     none


   Element has no clocking capabilities.


   Element has no URI handling capabilities.


   Pads:

     none


   Properties:

     name                : The name of the object
                           flags: readable, writable
                           String. Default: "hailodevicestats0"

     parent              : The parent of the object
                           flags: readable, writable
                           Object of type "GstObject"

     interval            : Time period between samples, in seconds
                           flags: readable, writable
                           Unsigned Integer. Range: 0 - 4294967295 Default: 1 

     device-id           : Device ID ([<domain>]:<bus>:<device>.<func>, same as in lspci command)
                           flags: readable, writable
                           String. Default: null

     silent              : Should print statistics
                           flags: readable, writable
                           Boolean. Default: false

     power-measurement   : Current power measurement of device
                           flags: readable
                           Float. Range:               0 -    3.402823e+38 Default:               0 

     temperature         : Current temperature of device
                           flags: readable
                           Float. Range:               0 -    3.402823e+38 Default:               0
