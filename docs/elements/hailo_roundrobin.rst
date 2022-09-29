
Hailo RoundRobin
================

Overview
--------

``HailoRoundRobin`` is an element that provides muxing functionality.
It receives input from one or more sink pads and forwards them into a single src pad in round-robin method.

It also adds metadata to each buffer with the input pad name it was received on,
The metadata's pupose is to be able to de-mux it easily later on by `hailostreamrouter <hailo_stream_router.rst>`_ .

Example
-------

Here's an example of a pesudo pipeline muxing 4 streams into one detection pipeline,
and then de-muxing them into 2 separate pipelines - one for person attributes and one for face attributes.

.. image:: ../resources/stream_router_example2.png

.. code-block::

    for ((n = 0; n < 4; n++)); do
        filesrc location=video_$n ! decodebin ! roundrobin.sink_$n
    hailoroundrobin name=roundrobin funnel-mode=false !
    <Rest of the pipeline>

Hierarchy
---------

.. code-block::

GObject
 +----GInitiallyUnowned
       +----GstObject
             +----GstElement
                   +----GstHailoRoundRobin

Pad Templates:
  SINK template: 'sink_%u'
    Availability: On request
    Capabilities:
      ANY
  
  SRC template: 'src'
    Availability: Always
    Capabilities:
      ANY

Element has no clocking capabilities.
Element has no URI handling capabilities.

Pads:
  SRC: 'src'
    Pad Template: 'src'

Element Properties:
  name                : The name of the object
                        flags: readable, writable
                        String. Default: "hailoroundrobin0"
  parent              : The parent of the object
                        flags: readable, writable
                        Object of type "GstObject"
  funnel-mode         : Disables the round robin logic and pushes all buffers when available
                        flags: readable, writable, controllable
                        Boolean. Default: false
