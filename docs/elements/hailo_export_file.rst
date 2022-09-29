Hailo Export File
==================

Overview
--------

| HailoExportFile is an element which provides an access point in the pipeline to export `HailoObjects meta <../write_your_own_application/hailo-objects-api.rst>`_ to a JSON file.
| The meta itself is not changed or removed, and buffers continue onwards in the pipeline unchanged.

Parameters
^^^^^^^^^^

The HailoExportFile element allows the user to change the output file name/path. The default is hailo_meta.json

Hierarchy
---------

.. code-block::

    GObject
    +----GInitiallyUnowned
          +----GstObject
                +----GstElement
                      +----GstBaseTransform
                            +----GstHailoExportFile

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
                            String. Default: "hailoexportfile0"
      parent              : The parent of the object
                            flags: readable, writable
                            Object of type "GstObject"
      qos                 : Handle Quality-of-Service events
                            flags: readable, writable
                            Boolean. Default: false
      location            : Location of the JSON file to save
                            flags: readable, writable, changeable only in NULL or READY state
                            String. Default: "hailo_meta.json"
