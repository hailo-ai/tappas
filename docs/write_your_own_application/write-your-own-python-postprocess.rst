===================================
Writing Your Own Python Postprocess
===================================

Overview
--------

To add a network to the TAPPAS that is not already supported, requires the implementation of a new post-process. Fortunately with the use of the `hailopython <../elements/hailo_python.rst>`_\ , there is no need to create any new GStreamer elements, only to provide a Python module that applies your post-processing! \
This section will review how to create a python module and what mechanisms/structures are available for creating a post-process.

Getting Started
---------------

hailopython requires a module and a Python function.

Python Module Template
^^^^^^^^^^^^^^^^^^^^^^

Below is a template for a Python module for the hailopython element.

.. code-block:: py

    import hailo

    # Importing VideoFrame before importing GST is must
    from gsthailo import VideoFrame
    from gi.repository import Gst

    # Create 'run' function, that accepts one parameter - VideoFrame, more about VideoFrame later.
    # `run` is default function name if no name is provided
    def run(video_frame: VideoFrame):
        print("My first Python postprocess!")

        return Gst.FlowReturn.OK

To call it, create a pipeline with hailopython:

.. code-block::

   gst-launch-1.0 videotestsrc ! hailopython module=$PATH_TO_MODULE/my_module.py ! autovideosink

Extracting the Tensors
^^^^^^^^^^^^^^^^^^^^^^

One of the first steps in each post-process is to get the output tensors.
This can be done by one of two methods ``video_frame.roi.get_tensor(tensor_name)`` or ``video_frame.roi.get_tensors()`` 

.. code-block:: py

   def run(video_frame: VideoFrame):
       for tensor in video_frame.roi.get_tensors():
           print(tensor.name())
       my_tensor = roi.get_tensor("output_layer_name")
       print(f"shape is {my_tensor.height()}X{my_tensor.width()}X{my_tensor.features()})

After doing this it is possible to convert this object of type HailoTensor to a numpy array on which perform post-processing operations can be performed more conveniently.
This is a fairly simple step, just use np.array on a given HailoTensor.

..

   Notice that np.array has a parameter that determines whether to copy the memory or use the original buffer.


.. code-block:: py

   def run(video_frame: VideoFrame):
       my_tensor = roi.get_tensor("output_layer_name")
       # To create a numpy array with new memory
       my_array = np.array(my_tensor)
       # To create a numpy array with original memory
       my_array = np.array(my_tensor, copy=False)

There are some other methods in HailoTensor, that can be performed ``dir(my_tensor)`` or ``help(my_tensor)``.

Adding Results
^^^^^^^^^^^^^^

After processing  the net results and obtaining the post-processed results, they can be used as preferred.
Below demonstrates how to add them to the original image in order to draw them later with the hailooverlay element.
In order to add the post-processed result to the original image - use the ``video_frame.roi.add_object`` method.
This method adds a HailoObject object to the image. There are several types of objects that are currently supported:
hailo.HailoClassification - Classification of the image.
hailo.HailoDetection - Detection in the image.
hailo.HailoLandmarks - Landmarks in the image.

It is possible to create one of these objects and then add it with the roi.add_object method.

.. code-block:: py

   def run(video_frame: VideoFrame):
       classification = hailo.HailoClassification(type='animal', index=1, label='horse', confidence=0.67)

       # You can also create a classification without class id (index).
       classification = hailo.HailoClassification(type='animal', label='horse', confidence=0.67)
       
       video_frame.roi.add_object(classification)

One can also add objects to detections:

.. code-block:: py

   def run(video_frame: VideoFrame):
       # Adds a person detection in the bottom right quarter of the image. (normalized only)
       person_bbox = hailo.HailoBBox(xmin=0.5, ymin=0.5, width=0.5, height=0.5)
       person = hailo.HailoDetection(bbox=person_bbox, label='person', confidence=0.97)
       video_frame.roi.add_object(person)
       
       # Now, Adds a face to the person, at the top of the person. (normalized only)
       face_bbox  = hailo.HailoBBox(xmin=0.0, ymin=0.0, width=1, height=0.2)
       face = hailo.HailoDetection(bbox=face_bbox, label='face', confidence=0.84)
       person.add_object(face)
       # No need to add the face to the roi because it is already in the person that is in the roi.

Next Steps
----------

Drawing
^^^^^^^

In order to draw the post-processed results on the original image use the hailooverlay element.
It is already familiar with our HailoObject types and knows how to draw classifications, detections, and landmarks onto the image.

.. code-block:: sh

   gst-launch-1.0 filesrc location=$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/detection/detection.mp4 name=src_0 ! decodebin \
   ! videoscale ! video/x-raw, pixel-aspect-ratio=1/1 ! videoconvert ! queue leaky=no max-size-buffers=30 \
   max-size-bytes=0 max-size-time=0 ! hailonet hef-path=$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/detection/yolov5m_wo_spp_60p.hef \
   is-active=true ! queue leaky=no max-size-buffers=30 max-size-bytes=0 \
   max-size-time=0 ! hailopython module=$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/detection/my_module.py qos=false ! queue \
   leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! hailooverlay qos=false ! videoconvert ! \
   fpsdisplaysink video-sink=ximagesink name=hailo_display sync=true text-overlay=false

..

   This is the standard detection pipeline with a python module for post-processing.


Multiple Functions in One Python Module
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

 There is an option to write several post-process functions in the same module.
 In order to run each of them just add the ``function`` property to the ``hailopython`` element:

.. code-block:: py

   import hailo

   # Importing VideoFrame before importing GST is must
   from gsthailo import VideoFrame
   from gi.repository import Gst


   def post_process_function(video_frame: VideoFrame):
       print("My first Python postprocess!")

   def other_post_function(video_frame: VideoFrame):
       print("Other Python postprocess!")

.. code-block::

   gst-launch-1.0 videotestsrc ! hailopython module=$PATH_TO_MODULE/my_module.py function=other_post_function ! autovideosink


VideoFrame Class
^^^^^^^^^^^^^^^^

In addition to providing ``buffer`` and ``HailoROI`` access functions, the ``VideoFrame`` module provides helper functions for accessing the buffer through NumPy 


List all Available Methods and Members
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Running the following command would display a list of methods and members available:

.. code-block:: bash

    python3 -c "import hailo; help(hailo)"