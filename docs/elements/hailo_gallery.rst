Hailo Gallery
==============

Overview
--------

HailoGallery is an element which enables the user to save and compare embeddings(HailoMatrix) that represents recogintion, in order to track objects across multiple streams.
It is also enables saving and loading these embeddings into a local JSON file (database like), in order to track pre-saved objects.

Parameters
^^^^^^^^^^

The hailogallery element provides a series of properties that allow you to adjust the gallery comparison algorithm. The most important property to set is ``class-id``\ : this determines if the gallery will track all `HailoDetection <../write_your_own_application/hailo-objects-api.rst#hailodetection>`_ objects indiscriminately of class or focus only on detections of a specific class id (the default behavior is to track across-classes).

Hierarchy
---------

.. code-block::

  GObject
  +----GInitiallyUnowned
        +----GstObject
              +----GstElement
                    +----GstBaseTransform
                          +----GstHailoGallery

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
                          String. Default: "hailogallery0"
    parent              : The parent of the object
                          flags: readable, writable
                          Object of type "GstObject"
    qos                 : Handle Quality-of-Service events
                          flags: readable, writable
                          Boolean. Default: false
    class-id            : The class id of the class to update into the gallery. Default -1 crosses classes.
                          flags: readable, writable, changeable only in NULL or READY state
                          Integer. Range: -2147483648 - 2147483647 Default: -1 
    similarity-thr      : Similarity threshold used in Gallery to find New ID's. Closer to 1.0 is less similar.
                          flags: readable, writable, controllable
                          Float. Range:               0 -               1 Default:            0.15 
    gallery-queue-size  : Number of Matrixes to save for each global ID
                          flags: readable, writable, controllable
                          Integer. Range: 0 - 2147483647 Default: 100 
    load-local-gallery  : Load Gallery from JSON file
                          flags: readable, writable, controllable
                          Boolean. Default: false
    save-local-gallery  : Save Gallery to JSON file
                          flags: readable, writable, controllable
                          Boolean. Default: false
    gallery-file-path   : Gallery JSON file path to load
                          flags: readable, writable, controllable
                          String. Default: null