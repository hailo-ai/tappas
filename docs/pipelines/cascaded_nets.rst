
Cascaded Networks Structure
===========================


.. image:: ../../resources/cascaded_nets_pipeline.png


This section provides a drill-down into the template of our cascaded network pipelines with a focus on explaining the ``GStreamer`` pipeline.

Example Pipeline
----------------

First, it is necessary to declare two sub-pipelines, these pipelines are derived from our `Single network template <single_network.rst>`_

.. code-block:: sh

   NETWORK_ONE_PIPELINE="videoscale qos=false ! \
       queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
       hailonet net-name=$NETWORK_ONE_NAME \
       hef-path=$hef_path is-active=true qos=false ! \
       queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
       hailofilter so-path=$NETWORK_ONE_SO function-name=$NETWORK_ONE_FUNC_NAME qos=false ! \
       queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0"

   NETWORK_TWO_PIPELINE="queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
       hailonet net-name=$NETWORK_TWO_NAME \
       hef-path=$hef_path is-active=true qos=false ! \
       queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
       hailofilter so-path=$NETWORK_TWO_SO function-name=$NETWORK_TWO_FUNC_NAME qos=false ! \
       queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0"

Next, insert them into the full pipeline:

.. code-block:: sh

   gst-launch-1.0 \
       $source_element ! \
       tee name=t \
       \
       hailomuxer name=hmux \
       \
       t. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! hmux. \
       t. ! $NETWORK_ONE_PIPELINE ! hmux. \
       hmux. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
       hailocropper internal-offset=$internal_offset name=cropper \
       \
       hailoaggregator name=agg \
       \
       cropper. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! agg. \
       \
       cropper. ! $NETWORK_TWO_PIPELINE ! agg. \
       agg. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
       hailooverlay ! \
       queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! videoconvert ! \
       fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=false text-overlay=false ${additional_parameters}

To clarify the user's understanding pipeline is described section by section:

.. code-block:: sh

       $source_element ! \
       tee name=t \
       \
       hailomuxer name=hmux \
       \
       t. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! hmux. \
       t. ! $NETWORK_ONE_PIPELINE ! hmux. \
       hmux. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \

This section of the pipeline is taken from our: `Single network template <single_network.rst#example-pipeline-with-resolution-preservation>`_. in brief: run an inference while keeping the original image resolution.

.. code-block:: sh

      hailocropper internal-offset=$internal_offset name=cropper hailoaggregator name=agg

`Hailocropper <../elements/hailo_cropper.rst>`_ Hailocropper splits the pipeline into 2 branches: the first passes the original frame while the other passes crops from that original frame one at a time. The hailocropper chooses what crops to take from the original frame based on an so file that you can provide (typically a set of detections from a prior hailofilter postprocess). The hailocropper also scales all crops based on caps negotiation. This way if a hailonet is placed after the cropper (such as in this example), then the hailocropper will scale all crops to that hailonet’s input network size. The hailoaggregator gets the original frame and then knows to wait for all related cropped buffers: aggregating all metadata to the original frame, then sending it forward.

.. code-block:: sh

      cropper. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! agg. \

The first part of the cascading network pipeline, passes the original frame on the bypass pads to hailoaggregator.

.. code-block:: sh

      cropper. ! $NETWORK_TWO_PIPELINE ! agg.

   The second part of the cascading network pipeline, performs a second network on all objects, which are cropped and scaled to the needed resolution by the HEF in the hailonet.

.. code-block:: sh

       agg. ! queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! \
       hailooverlay ! \

Aggregates all objects to the original frame, and draws them over the frame using the `hailooverlay <../elements/hailo_overlay.rst>`_ with specific drawing function.

.. code-block:: sh

      queue leaky=no max-size-buffers=3 max-size-bytes=0 max-size-time=0 ! videoconvert ! \
      fpsdisplaysink video-sink=xvimagesink name=hailo_display sync=false text-overlay=false

Display the final image using ``fpsdisplaysink``.
