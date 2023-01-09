==============
Benchmark Tool
==============

The benchmark tool is a new script that can help you figure out what are your computer's limitations when trying to build an application using Tappas baseline.
With this tool you can measure the FPS of different pipelines according to a variety of parameters.
The pipeline that will run will be without any neural network and is able to demonstrate the performence your computer is able to provide under your conditions.


Benchmark Usage
---------------

Options:
  --help                              Show this help

  --video-prefix-path PATH            Video prefix path

  --min-num-of-sources NUM            Setting number of sources to given input (Default is 1)
  --max-num-of-sources NUM            Setting number of sources to given input (Default is 4)
  --num-of-buffers NUM                Number of buffers for each stream (Default is 500)

  --use-vaapi                         Whther to use vaapi decodeing or not (Default is no vaapi)
  --use-display                       Whther to use display or not (Default is no display)

  --format FORMAT                     Required format
  --display-resolution RESOLUTION     Scale width and height of each stream in WxH mode (e.g. 640x480)


Usage Notes
-----------

#. The format should be written according to the gstreamer conventions, e.g. RGBA instead of rgba.
#. If the min and max number of sources are the same only 1 pipeline will run, otherwise several pipelines will be running consecutively.
#. If you want to use VA-API make sure you download VA-API via our `VA-API Install Manual <../../apps/gstreamer/x86_hw_accelerated/README.rst>`_


Videos For Benchmark Tool
-------------------------

Before using the benchmark tool you must prepare videos for it to run. The videos must be mp4 videos.
We prepared a `script <../../tools/benchmark/create_soft_links.sh>`_
The script takes one video and create as many soft links to this video as you request, numbered from 0 to n-1 for the benchmark tool.
When you are giving the prefix after --video-prefix-path in the benchmark tool, give a path with the name of the videos until the numbering.
Example: If my videos are at /tmp/tappas/ and their names are tap0.mp4 to tap15.mp4, the prefix will be "/tmp/tappas/tap".



Benchmark Example
-----------------

.. code-block:: sh
    TAPPAS_WORKSPACE=/local/workspace/tappas
    INPUT_PATH="$TAPPAS_WORKSPACE/apps/gstreamer/general/license_plate_detection/resources/lpr_ayalon.mp4"
    OUTPUT_PATH="$TAPPAS_WORKSPACE/tap_video"
    ./tools/benchmark/create_soft_links.sh --input $INPUT_VIDEO --num-of-copies 16 --output-prefix $OUTPUT_PATH
    ./tools/benchmark/benchmark.sh --video-prefix-path $OUTPUT_PATH --min-num-of-sources 16 --max-num-of-sources 16 --display-resolution 640x480
