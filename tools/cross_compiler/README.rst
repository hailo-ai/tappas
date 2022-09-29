
Cross-compile Hailo's GStreamer plugins
=======================================

Overview
--------

Hailo recommended method at the moment for cross-compilation is using Yocto SDK (aka Toolchain). We provide wrapper scripts whose only requirement is a Yocto toolchain to make this as easy as possible.

Preparations
------------

In order to cross-compile you need to run ``TAPPAS`` container on a X86 machine and copy the Yocto toolchain to the container.

Toolchain
^^^^^^^^^

What is Toolchain?
~~~~~~~~~~~~~~~~~~

A standard Toolchain consists of the following:


* | Cross-Development Toolchain: This toolchain contains a compiler, debugger, and various miscellaneous tools.

* | Libraries, Headers, and Symbols: The libraries, headers, and symbols are specific to the image (i.e. they match the image).

* | Environment Setup Script: This \*.sh file, once run, sets up the cross-development environment by defining variables and preparing for Toolchain use.

| You can use the standard Toolchain to independently develop and test code that is destined to run on some target machine.

What should I add to my image?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For this example we would add the recipes to a NXP-IMX based image

Must add
~~~~~~~~

.. code-block:: sh

   # GStreamer plugins
   IMAGE_INSTALL_append += "                    \
       imx-gst1.0-plugin                        \
       gstreamer1.0-plugins-bad-videoparsersbad \
       gstreamer1.0-plugins-good-video4linux2   \
       gstreamer1.0-plugins-base                \
   "

   # Opencv requirements for the hailo gstreamer plugin’s postprocess
   CORE_IMAGE_EXTRA_INSTALL += "    \
         libopencv-core-dev         \
         libopencv-highgui-dev      \
   "

Nice to add
~~~~~~~~~~~

.. code-block:: sh

   # GStreamer plugins
   IMAGE_INSTALL_append += "                    \
       gstreamer1.0-python                      \
       gst-shark                                \
       gst-instruments                          \
   "

   # Enable trace hooks for GStreamer
   PACKAGECONFIG_append_pn-gstreamer1.0 += " gst-tracer-hooks"

Prepare the Toolchain
~~~~~~~~~~~~~~~~~~~~~

In order to generate a Yocto toolchain, use this following command

.. code-block:: sh

   # Generate the Toolchain
   bitbake -c do_populate_sdk <image name>

Compress the toolchain to tar.gz

.. code-block:: sh

   cd <BUILD_DIR>/tmp/deploy/sdk
   touch toolchain.tar.gz
   tar -czf toolchain.tar.gz --exclude=toolchain.tar.gz .

Copy the ``toolchain.tar.gz`` to the container using ``docker cp``

.. code-block:: sh

   docker cp toolchain.tar.gz hailo_tappas_container:/local/workspace/tappas

Components
----------

GstHailo
^^^^^^^^

Compiling the ``gst-hailo`` component.
This script, firstly unpack and installs the toolchain (If not installed already), and only after that, cross-compiles.

Flags
~~~~~

.. code-block:: sh

   $ ./cross_compile_gsthailo.py  --help
   usage: cross_compile_gsthailo.py [-h]
                                    {aarch64,armv7l} {debug,release}
                                    toolchain_tar_path

   Cross-compile gst-hailo.

   positional arguments:
     {aarch64,armv7l}    Arch to compile to
     {debug,release}     Build and compilation type
     toolchain_tar_path  Toolchain TAR path

   optional arguments:
     -h, --help          show this help message and exit

Example
~~~~~~~

An example for executing the script:



.. note::
    In this example we assume that the toolchain is located under toolchain-raw/hailo-dartmx8m-zeus-aarch64-toolchain.tar.gz


.. code-block:: sh

   $ ./cross_compile_gsthailo.py aarch64 debug toolchain-raw/hailo-dartmx8m-zeus-aarch64-toolchain.tar.gz 
   INFO:cross_compile_gsthailo.py:Compiling gstreamer
   INFO:cross_compile_gsthailo.py:extracting toolchain
   INFO:cross_compile_gsthailo.py:installing toolchain
   INFO:cross_compile_gsthailo.py:installing /$TAPPAS_WORKSPACE/tools/cross-compiler/toolchain-raw/fsl-imx-xwayland-glibc-x86_64-fsl-image-gui-aarch64-imx8mq-var-dart-toolchain-5.4-zeus.sh
   INFO:cross_compile_gsthailo.py:toolchain ready to use (/local/workspace/tappas/tools/cross-compiler/toolchain)
   INFO:cross_compile_gsthailo.py:using environment setup found in /$TAPPAS_WORKSPACE/tools/cross-compiler/toolchain/environment-setup-aarch64-poky-linux
   INFO:cross_compile_gsthailo.py:Starting compilation...
   INFO:cross_compile_gsthailo.py:Compilation done

A good practice is to check the output using ``file`` command

.. code-block:: sh

   $ ls aarch64-gsthailo-build/
   CMakeCache.txt  CMakeFiles  Makefile  cmake_install.cmake  libgsthailo.so
   $ file aarch64-gsthailo-build/libgsthailo.so 
   aarch64-gsthailo-build/libgsthailo.so: ELF 64-bit LSB shared object, ARM aarch64, version 1 (SYSV), dynamically linked, BuildID[sha1]=e55c1655c113e99bb649dbb03c15b844142503ee, with debug_info, not stripped

As you can see, the file is compatible to ``aarch64`` like we wanted to

GstHailoTools
^^^^^^^^^^^^^

This script cross-compiles ``gst-hailo-tools``.
This script, firstly unpack and installs the toolchain (If not installed already), and only after that, cross-compiles.

Flags
~~~~~

.. code-block:: sh

   $ ./cross_compile_gsthailotools.py --help
   usage: cross_compile_gsthailotools.py [-h]
                                         [--yocto-distribution YOCTO_DISTRIBUTION]
                                         {aarch64,armv7l} {debug,release}
                                         toolchain_tar_path

   Cross-compile gst-hailo.

   positional arguments:
     {aarch64,armv7l}      Arch to compile to
     {debug,release}       Build and compilation type
     toolchain_tar_path    Toolchain TAR path

   optional arguments:
     -h, --help            show this help message and exit
     --yocto-distribution YOCTO_DISTRIBUTION
                           The name of the Yocto distribution to use

Example
~~~~~~~

Run the compilation script



.. note::
    In this example we assume that the toolchain is located under toolchain-raw/hailo-dartmx8m-zeus-aarch64-toolchain.tar.gz


.. code-block:: sh

   $ ./cross_compile_gsthailotools.py aarch64 debug toolchain
   INFO:./cross_compile_gsthailotools.py:Building hailofilter plugin and post processes
   INFO:./cross_compile_gsthailotools.py:Running Meson build.
   INFO:./cross_compile_gsthailotools.py:Running Ninja command.

Check the output directory

.. code-block:: sh

   $ ls aarch64-gsthailotools-build/
   build.ninja  compile_commands.json  config.h  libs  meson-info  meson-logs  meson-private  plugins

``libgsthailotools.so`` is stored under libs

.. code-block:: sh

   $ ls aarch64-gsthailotools-build/plugins/*.so
   libgsthailotools.so

And the post-processes are stored under plugins

.. code-block:: sh

   $ ls aarch64-gsthailotools-build/libs/*.so   
   libcenterpose_post.so  libmobilenet_ssd_post.so
   libclassification.so   libsegmentation_draw.so
   libdebug.so            libyolo_post.so
   libdetection_draw.so

Copy the cross-compiled files
-----------------------------

Find out where the ``GStreamer`` plugins are stored in your embedded device by running the following command:

.. code-block:: sh

   gst-inspect-1.0 filesrc | grep Filename | awk '{print $2}' | xargs dirname

Copy ``libgsthailo.so`` + ``libgsthailotools.so`` to the path found out above.
Copy the post-processes ``so`` files under ``libs`` to the embedded device under /usr/lib/hailo-post-processes (create the directory if it does not exist)

Run ``gst-inspect-1.0 hailo`` and ``gst-inspect-1.0 hailotools`` and make sure that no error raises  
