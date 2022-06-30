=========
Debugging
=========

GstShark
--------


.. image:: ../resources/example.png


GstShark is an open-source project from RidgeRun that provides benchmarks and profiling tools for GStreamer 1.7.1 (and above). It includes tracers for generating debug information plus some tools to analyze the debug information. GstShark provides easy to use and useful tracers, paired with analysis tools to enable straightforward optimizations.

GstShark leverages GStreamer's tracing hooks and open-source and standard tracing and plotting tools to simplify the process of understanding the bottlenecks in your pipeline.

The profiling tool provides 3 general features that can be used to debug the pipeline:


* 
  Console printouts - At the most basic level, you should get printouts from the traces about the different measurements made. If you know what you are looking for, you may see it here at runtime.

* 
  Graphic visualization - Shown above, gst-shark can generate a pipeline graph that shows how elements are connected and what caps were negotiated between them. This is a very convenient feature to look at the pipeline in a more comfortable way. The graph is generated at runtime so it is a great way to see and debug how elements were actually connected and what formats the data ended up in.

* 
  Gst-plot - A suite of graph generating scripts are included in gst-shark that will plot different graphs for each tracer metric enabled. This is a powerful tool to visualize each metric that can be used for deeper debugging.

Install
-------

Our docker image already contains GstShark! If you decide to not use our Docker image, our suggestion is to follow RidgeRun tutorial: `GstShark <https://developer.ridgerun.com/wiki/index.php?title=GstShark>`_

Bash shortcuts
--------------

As part of our creation of the Docker image, we copy some convinet shortcuts to GstShark:

.. code-block:: sh

   vim ~/.bashrc

.. code-block:: sh

   # set gstreamer debug
   gst_set_debug() {
     export GST_SHARK_LOCATION=/tmp/profile
     export GST_DEBUG_DUMP_DOT_DIR=<PATH YOU WANT DUMP FILES>
     export GST_DEBUG="GST_TRACER:7"
     export GST_TRACERS="cpuusage;proctime;interlatency;scheduletime;buffer;bitrate;framerate;queuelevel;graphic"
     echo 'export GST_TRACERS="cpuusage;proctime;interlatency;scheduletime;buffer;bitrate;framerate;queuelevel;graphic"'
   }

   # set gstreamer to only show graphic
   gst_set_graphic() {
     export GST_SHARK_LOCATION=/tmp/profile
     export GST_DEBUG_DUMP_DOT_DIR=<PATH YOU WANT DUMP FILES>
     export GST_DEBUG="GST_TRACER:7"
     export GST_TRACERS="graphic"
     echo 'export GST_TRACERS="cpuusage;proctime;interlatency;scheduletime;buffer;bitrate;framerate;queuelevel;graphic"'
   }

   # unset gstreamer debug
   gst_unset_debug() {
     unset GST_TRACERS
   }

   # run gst-plot
   gst_plot_debug() {
     cd <PATH TO GST-SHARK REPO FOLDER: gst-shark/scripts/graphics>
     ./gstshark-plot $GST_SHARK_LOCATION -p
     cd -
   }

Note that we added 4 functions: two sets, an unset, and a plot function. The set functions enable gst-shark by setting environment variables, the chief of which is GST_TRACERS. This enables the different trace hooks in the pipeline. The available tracers are listed in the echo command at the end of each set. You can enable any combination of the available tracers, just chain them together with a ';' (notice that the difference between gst_set_debug and gst_set_graphic is that gst_set_debug enables all tracers whereas gst_set_graphic only enables the graphic tracer that draws the pipeline graph). GST_SHARK_LOCATION and GST_DEBUG_DUMP_DOT_DIR set locations where the dump files are stored, the first sets where the tracer dumps are (used for gst-plot), and the latter where the dot file is saved (the graphic pipeline graph). Unset disables all tracers, and gst_plot_debug runs gst-plot.

Using GstShark
--------------

Let’s say you have a gstreamer app you want to profile. Start by enabling gst-shark:


.. raw:: html

   <div align="left"><img src="../resources/using.gif"/></div>


Then just run your app. You will start seeing all kinds of tracer prints, and when the pipeline starts playing you should see the graphic plot load.



   **NOTE:** : The graph will stay open as long as the pipeline runs. However if you have GST_DEBUG_DUMP_DOT_DIR set then afterwards a .dot file will be saved. Click this file to reopen the graph.



.. raw:: html

   <div align="left"><img src="../resources/full.gif"/></div>


After you’ve run a gstreamer pipeline with tracers enabled, you can plot them using gst-plot (gst-plot will open an octave window which will runt he appropriate script to plot each tracer. Depending on how much data you have to plot this can take a while:


.. raw:: html

   <div align="left"><img src="../resources/plot_debug.gif"/></div>
   <div align="left"><img src="../resources/queues.gif"/></div>


Each graph inspects a different metric of the pipeline, it is recommended to read more about what each one represents here:


* CPU Usage (cpuusage) - Measures the CPU usage every second. In multiprocessor systems this measurements are presented per core.
* Processing Time (proctime) - Measures the time an element takes to produce an output given the corresponding input.
* InterLatency (interlatency) - Measures the latency time at different points in the pipeline.
* Schedule Time (scheduling) - Measures the amount of time between two consecutive buffers in a sink pad.
* Buffer (buffer) - Prints information of every buffer that passes through every sink pad in the pipeline.
* Bitrate (bitrate) - Measures the current stream bitrate in bits per second.
* Framerate (framerate) - Measures the amount of frames that go through a src pad every second.
* Queue Level (queuelevel) - Measures the amount of data queued in every queue element in the pipeline
* Graphic (graphics) - Records a graphical representation of the current pipeline

Modify Buffering Mode and Size
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: sh

   $ export GST_SHARK_FILE_BUFFERING=0

With the no buffering mode every I/O operation is written as soon as possible.

The following command is an example of how to define the environment variable that will change the buffering mode to full buffering and the buffering size, this command uses a positive integer value for the size:

.. code-block:: sh

   $ export GST_SHARK_FILE_BUFFERING=1024

Individual Element Tracing (filter)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The individual element tracing, or filter parameter, allows the user to choose which elements get included in the tracing. The value to be set in the filter is a Glib Compatible Regular Expression, meaning that elements to be traced can be grouped by using a regex that matches with their name.

The filtering applies to the element name, NOT the factory. This is, if your element is specified as "identity name=myelem", you should refer to "myelem" and not to "identity"

Print the amount of frames that flow every 5 seconds through the different src pads in the pipe:

.. code-block:: sh

   GST_TRACERS="framerate(period=5)" GST_DEBUG=GST_TRACER:7

Print the amount of bits that flow every 3 seconds through the different src pads in the pipe:

.. code-block:: sh

   GST_TRACERS="bitrate(period=3)" GST_DEBUG=GST_TRACER:7

Print the amount of frames that flow every 5 seconds and bits that flow every 3 seconds through the different src pads in the pipe:

.. code-block:: sh

   GST_TRACERS="framerate(period=5);bitrate(period=3)" GST_DEBUG=GST_TRACER:7

Print the amount of frames that flow every 5 through the identity:

.. code-block:: sh

   GST_TRACERS="framerate(period=5,filter=identity);bitrate(period=3)" GST_DEBUG=GST_TRACER:7

Good luck, happy hunting.

Using gst-instruments
---------------------

gst-instruments is a set of performance profiling and data flow inspection tools for GStreamer pipelines.


* 
  ``gst-top-1.0`` at the start of the pipeline will analyze and profile the run. (gst-top-1.0 gst-launch-1.0 ! audiotestsrc ! autovideosink)

* 
  ``gst-report-1.0`` - generates performance report for input trace file.

* 
  ``gst-report-1.0 --dot gst-top.gsttracee | dot -Tsvg > perf.svg`` - generates performance graph in DOT format.

`Read more in gst-instruments github page <https://github.com/kirushyk/gst-instruments>`_
