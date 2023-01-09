Hailo Export ZMQ
==================

Overview
--------

| HailoExportZMQ is an element which provides an access point in the pipeline to export `HailoObjects meta <../write_your_own_application/hailo-objects-api.rst>`_ to a `ZMQ  <https://zeromq.org/>`_ socket.
| The meta itself is not changed or removed, and buffers continue onwards in the pipeline unchanged.

Parameters
^^^^^^^^^^

The HailoExportZMQ element allows the user to change the output port/protocol. The default is `tcp://*:5555`. 
Currently only PUB behvaior (`PUB/SUB <https://zeromq.org/socket-api/#publish-subscribe-pattern>`_) is supported.

Hierarchy
---------

.. code-block::

    GObject
    +----GInitiallyUnowned
          +----GstObject
                +----GstElement
                      +----GstBaseTransform
                            +----GstHailoExportZMQ

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
                            String. Default: "hailoexportzmq0"
      parent              : The parent of the object
                            flags: readable, writable
                            Object of type "GstObject"
      qos                 : Handle Quality-of-Service events
                            flags: readable, writable
                            Boolean. Default: false
      address             : Address to bind the socket to.
                            flags: readable, writable, changeable only in NULL or READY state
                            String. Default: "tcp://*:5555"