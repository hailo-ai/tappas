==============
Benchmark Tool
==============

The benchmark tool is a new script that can help the user identify their platform's limitations when trying to build an application using TAPPAS baseline.
This tool can be used to measure the FPS of different pipelines according to a variety of parameters.
The pipeline that will run will be without any neural network and is able to demonstrate the performance the platform is able to provide under the defined conditions.


Benchmark Usage
---------------

Options:
  --help                              Show this help

  --video-prefix-path PATH            Video prefix path

  --min-num-of-sources NUM            Setting number of sources to given input (Default is 1)
  --max-num-of-sources NUM            Setting number of sources to given input (Default is 4)
  --num-of-buffers NUM                Number of buffers for each stream (Default is 500)

  --use-vaapi                         Whether to use vaapi decoding or not (Default is no vaapi)
  --use-display                       Whether to use display or not (Default is no display)

  --format FORMAT                     Required format
  --display-resolution RESOLUTION     Scale width and height of each stream in WxH mode (e.g. 640x480)


Usage Notes
-----------

#. The format should be written according to the GStreamer conventions, e.g. RGBA instead of rgba.
#. If the min and max number of sources are the same only 1 pipeline will run, otherwise several pipelines will be running consecutively.
#. If you want to use VA-API make sure you download VA-API.

Videos For Benchmark Tool
-------------------------

Before using the benchmark tool videos must be prepared for it to run. The videos must be in format mp4 .
A working script has been prepared `script <../../tools/benchmark/create_soft_links.sh>`_
which takes one video and creates as many soft links to this video as the user requires, numbered from 0 to n-1 for the benchmark tool.
When providing the prefix after --video-prefix-path in the benchmark tool, write a path with the name of the videos until the numbering.
Example: If the videos are located at /tmp/tappas/ and their names are tap0.mp4 to tap15.mp4, the prefix will be "/tmp/tappas/tap".



Benchmark Example
-----------------

.. code-block:: sh
    TAPPAS_WORKSPACE=/local/workspace/tappas
    INPUT_PATH="$TAPPAS_WORKSPACE/apps/h8/gstreamer/general/license_plate_detection/resources/lpr_ayalon.mp4"
    OUTPUT_PATH="$TAPPAS_WORKSPACE/tap_video"
    ./tools/benchmark/create_soft_links.sh --input $INPUT_VIDEO --num-of-copies 16 --output-prefix $OUTPUT_PATH
    ./tools/benchmark/benchmark.sh --video-prefix-path $OUTPUT_PATH --min-num-of-sources 16 --max-num-of-sources 16 --display-resolution 640x480
