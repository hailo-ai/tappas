Hailo Tracker
==============

Overview
--------

| HailoTracker is an element which enables the user to track `HailoDetection <../write_your_own_application/hailo-objects-api.rst#hailodetection>`_ objects using the Joint-Detection-and-Embedding (JDE) Tracking algorithm. JDE is a fast and high-performance multiple-object tracking algorithm that associates between detections in consecutive frames based on size/location/movement. Associations are made by linear assignment via a Kalman Filter.
| For more information on `JDE Tracking and Kalman Filtering read here <https://github.com/Zhongdao/Towards-Realtime-MOT>`_.

Parameters
^^^^^^^^^^

The hailotracker element provides a series of properties that allow you to adjust the tracking algorithm. The most important property to set is ``class-id``\ : this determines if the tracker will track all `HailoDetection <../write_your_own_application/hailo-objects-api.rst#hailodetection>`_ objects indiscriminately of class or focus only on detections of a specific class id (the default behavior is to track across-classes). 

Hierarchy
---------

.. code-block::

   GObject
    +----GInitiallyUnowned
          +----GstObject
                +----GstElement
                      +----GstBaseTransform
                            +----GstVideoFilter
                                  +----GstHailoTracker

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
                           String. Default: "hailotracker0"
     parent              : The parent of the object
                           flags: readable, writable
                           Object of type "GstObject"
     qos                 : Handle Quality-of-Service events
                           flags: readable, writable
                           Boolean. Default: true
     debug               : Enable debug mode.
                           flags: readable, writable, controllable
                           Boolean. Default: false
     class-id            : The class id of the class to track. Default -1 crosses classes.
                           flags: readable, writable, changeable only in NULL or READY state
                           Integer. Range: -2147483648 - 2147483647 Default: -1 
     kalman-dist-thr     : Threshold used in Kalman filter to compare Mahalanobis cost matrix. Closer to 1.0 is looser.
                           flags: readable, writable, controllable
                           Float. Range:               0 -               1 Default:             0.7 
     iou-thr             : Threshold used in Kalman filter to compare IOU cost matrix. Closer to 1.0 is looser.
                           flags: readable, writable, controllable
                           Float. Range:               0 -               1 Default:             0.8 
     init-iou-thr        : Threshold used in Kalman filter to compare IOU cost matrix of newly found instances. Closer to 1.0 is looser.
                           flags: readable, writable, controllable
                           Float. Range:               0 -               1 Default:             0.9 
     keep-tracked-frames : Number of frames to keep without a successful match before a 'tracked' instance is considered 'lost'.
                           flags: readable, writable, controllable
                           Integer. Range: 0 - 2147483647 Default: 2 
     keep-new-frames     : Number of frames to keep without a successful match before a 'new' instance is removed from the tracking record.
                           flags: readable, writable, controllable
                           Integer. Range: 0 - 2147483647 Default: 2 
     keep-lost-frames    : Number of frames to keep without a successful match before a 'lost' instance is removed from the tracking record.
                           flags: readable, writable, controllable
                           Integer. Range: 0 - 2147483647 Default: 2
