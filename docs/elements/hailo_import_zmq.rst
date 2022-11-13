Hailo Import ZMQ
==================

Overview
--------

| HailoImportZMQ is an element which provides an access point in the pipeline to import `HailoObjects meta <../write_your_own_application/hailo-objects-api.rst>`_ from a `ZMQ  <https://zeromq.org/>`_ socket.
| The meta is added to any pre-existing ROI in the buffer, and then the buffer continues onwards in the pipeline.

Parameters
^^^^^^^^^^

The HailoImportZMQ element allows the user to change the input port/protocol. The default is `tcp://localhost:5555`. 
Currently only SUB behvaior (`PUB/SUB <https://zeromq.org/socket-api/#publish-subscribe-pattern>`_) is supported.

Hierarchy
---------

.. code-block::

    GObject
    +----GInitiallyUnowned
          +----GstObject
                +----GstElement
                      +----GstBaseTransform
                            +----GstHailoImportZMQ

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
      SINK: 'sink'
        Pad Template: 'sink'
      SRC: 'src'
        Pad Template: 'src'

    Element Properties:
      name                : The name of the object
                            flags: readable, writable
                            String. Default: "hailoimportzmq0"
      parent              : The parent of the object
                            flags: readable, writable
                            Object of type "GstObject"
      qos                 : Handle Quality-of-Service events
                            flags: readable, writable
                            Boolean. Default: false
      address             : Address to bind the socket to.
                            flags: readable, writable, changeable only in NULL or READY state
                            String. Default: "tcp://localhost:5555"