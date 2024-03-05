
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

For this example we will add the recipes to an NXP-IMX based image

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

Copy the ``toolchain`` directory to the container using ``docker cp``

.. code-block:: sh

   docker cp toolchain hailo_tappas_container:/local/workspace/tappas

Components
----------

TAPPAS
^^^^^^^^^^^^^

This script cross-compiles ``TAPPAS`` components.
This script first unpack and installs the toolchain (if not already installed), and then cross-compiles TAPPAS core/.

Flags
~~~~~

.. code-block:: sh

   $ ./cross_compile_tappas.py --help
    usage: cross_compile_tappas.py [-h] [--remote-machine-ip REMOTE_MACHINE_IP] [--build-lib {all,apps,plugins,libs,tracers}] [--clean-build-dir]
                               [--install-to-rootfs]
                               {aarch64,armv7l,armv7lhf,armv8a} {imx6,imx8,hailo15} {debug,release} toolchain_dir_path

    Cross-compile TAPPAS

    positional arguments:
    {aarch64,armv7l,armv7lhf,armv8a}
                            Arch to compile to
    {imx6,imx8,hailo15}   Target platform to compile to
    {debug,release}       Build and compilation type
    toolchain_dir_path    Toolchain directory path

    optional arguments:
    -h, --help            show this help message and exit
    --remote-machine-ip REMOTE_MACHINE_IP
                            remote machine ip
    --build-lib {all,apps,plugins,libs,tracers}
                            Build a specific tappas lib target (default all)
    --clean-build-dir     Delete previous build cache (default false)
    --install-to-rootfs   Install to rootfs (default false)


Example
~~~~~~~

Run the compilation script

.. note::
    In this example we assume that the toolchain is located under toolchain-raw/hailo-dartmx8m-zeus-aarch64-toolchain


.. code-block:: sh

   $ ./cross_compile_tappas.py aarch64 imx8 debug toolchain
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
