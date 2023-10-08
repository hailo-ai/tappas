
Multi Stream Pipeline Structure
===============================


.. image:: ../resources/one_network_multi_stream.png


This page provides a drill-down into the template of our multi stream pipelines with a focus on explaining the ``GStreamer`` pipeline.

Example Pipeline
----------------

The first stage is to create the pipeline sources.

.. code-block:: sh

   n=4
   sources=''

   for ((n = $start_index; n < $num_of_src; n++)); do
       sources+="$source_element ! \
               queue name=hailo_preprocess_q_$n leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! \
               videoconvert ! videoscale ! fun.sink_$n \
               sid.src_$n ! queue name=comp_q_$n leaky=downstream max-size-buffers=30 \
               max-size-bytes=0 max-size-time=0 ! comp.sink_$n "
   done

Each source is a sub-pipeline

.. code-block:: sh

   pipeline="gst-launch-1.0 \
           funnel name=fun ! \
           queue name=hailo_pre_infer_q_0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           hailonet hef-path=$HEF_PATH is-active=true ! \
           queue name=hailo_postprocess0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           hailofilter so-path=$POSTPROCESS_SO qos=false ! \
           queue name=hailo_draw0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           hailooverlay ! \
           streamiddemux name=sid \
           compositor name=comp start-time-selection=0 $compositor_locations ! \
           queue name=hailo_video_q_0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           videoconvert ! queue name=hailo_display_q_0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! \
           $video_sink_element name=hailo_display sync=false \
           $sources

They can then be combined together


* ``funnel`` takes multiple input sinks and outputs one source. an N-to-1 funnel that attaches a streamid to each stream, can later be used to demux back into separate streams. this lets you queue frames from multiple streams to send to the hailo device one at a time.
* ``streamiddemux`` a reverse to the funnel. It is a 1-to-N demuxer that splits a serialized stream based on stream id to multiple outputs.
* ``compositor`` composites pictures from multiple sources. handy for multi-stream/tiling like applications, as it lets you input many streams and draw them all together as a grid.
