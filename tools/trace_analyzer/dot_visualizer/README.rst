Dot Visulaizer
==============

How to use the tool?
--------------------

.. code:: bash

   # Run `gst_set_debug`
   $ gst_set_debug

   # Optional, adjust the TRACERS
   # Important: Keep the `graphic` tracer
   $ export GST_TRACERS="proctime;interlatency;framerate;queuelevel;graphic"

   # Run the application
   $ gst-launch-1.0 ...

   # Prepare the tracers dir 
   $ gst_prepare_tracers_dir
   # output should be in the following format:
   Splited tracers dir: /local/workspace/tappas/tappas_traces_<date>

   # Run the tool
   $ hailo_tappas_dot_visualizer /local/workspace/tappas/tappas_traces_<date>