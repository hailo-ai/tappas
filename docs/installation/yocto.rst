
Yocto
=====

This section guides users how to integrate Hailo's Yocto layers into their own Yocto environment.

Two layers are provided by Hailo, the first one is ``meta-hailo`` which compiles the ``HailoRT`` sources, and the second one is ``meta-hailo-tappas`` which compiles the ``TAPPAS`` sources.

``meta-hailo-tappas`` is a layer that is based on top of ``meta-hailo`` and adds ``TAPPAS`` recipes.

The layers are stored in `Meta-Hailo Github <https://github.com/hailo-ai/meta-hailo.git>`_\ , with a branch for each supported Yocto release:


* Kirkstone 4.0 (kernel 5.15)

Setup
-----

HailoRT
^^^^^^^

Add the following to your image in your ``conf/bblayers.conf``\ :

.. code-block:: sh

   BBLAYERS += " ${BSPDIR}/sources/meta-hailo/meta-hailo-accelerator \
                 ${BSPDIR}/sources/meta-hailo/meta-hailo-libhailort"

Add the recipes to your image in your ``conf/local.conf``\ :

.. code-block:: sh

   IMAGE_INSTALL:append = "hailo-firmware libhailort hailortcli hailo-pci libgsthailo"

TAPPAS
^^^^^^

Add the following to your image in your ``conf/bblayers.conf``\ :

.. code-block:: sh

   BBLAYERS += "${BSPDIR}/sources/meta-hailo/meta-hailo-tappas"

Add the following to your image in your ``conf/local.conf``\ :

.. code-block:: sh

   IMAGE_INSTALL:append = "libgsthailotools hailo-post-processes tappas-tracers"

Building the Image
----------------

Run bitbake and build the image. After the build has successfully finished, burn the image to the embedded device.

.. note::
    Building on non-IMX devices:
    To increase application performance, the imx gstreamer-plugins-base has been patched.
    In non-IMX devices an error may be encountered indicating that recipes under ``meta-hailo-tappas/recipes-gstreamer/gstreamer/`` cannot be parsed.
    In this case, remove this directory under the meta-hailo-tappas layer, and re-build the image.

    .. code-block:: sh

        rm -rf meta-hailo/meta-hailo-tappas/recipes-gstreamer/gstreamer/


Validating the Integration's Success
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Make sure that the following conditions have been met on the target device:


* 
  Running ``hailortcli fw-control identify`` prints the right configurations

* 
  Running ``gst-inspect-1.0 | grep hailo`` returns hailo elements:

  .. code-block:: sh

     hailo:  hailonet: hailonet element
     hailodevicestats: hailodevicestats element

* 
  Running ``gst-inspect-1.0 | grep hailotools`` returns hailotools elements:

  .. code-block:: sh

     hailotools: hailomuxer: Muxer pipe fitting
     hailotools: hailofilter: Hailo postprocessing and drawing element
     ...

* 
  post-processes shared object files exists at ``/usr/lib/hailo-post-processes``

Recipes
-------

libgsthailo
^^^^^^^^^^^

Hailo's GStreamer plugin for running inference on the hailo8 chip. Depends on ``libhailort`` and GStreamer.

The recipe compiles and copies the ``libgsthailo.so`` file to ``/usr/lib/gstreamer-1.0`` on the target device's
root file system, making it loadable by GStreamer as a plugin.

libgsthailotools
^^^^^^^^^^^^^^^^

Hailo's TAPPAS gstreamer elements. Depends on ``libgsthailo``, GStreamer, opencv, xtensor and xtl.
The source files are located in the TAPPAS release under ``core/hailo``.
The recipe compiles with meson and copies the ``libgsthailotools.so`` file to ``/usr/lib/gstreamer-1.0`` 
on the target device's root file system.

hailo-post-processes
^^^^^^^^^^^^^^^^^^^^

The recipe compiles and copies the post processes to ``/usr/lib/hailo-post-processes``.
Depends on opencv, xtensor, xtl, rapidjson and cxxopts.

tappas-tracers
^^^^^^^^^^^^^^
Hailo's TAPPAS gstreamer tracers. Depends on ``libgsthailo`` and GStreamer.
The source files are located in the TAPPAS release under ``core/hailo/tracers``.
The recipe compiles with meson and copies the ``libgsthailotracers.so`` file to ``/usr/lib/gstreamer-1.0`` 
on the target device's root file system.

For instructions on how to use the tracers on a yocto built machine, see `debugging <../write_your_own_application/debugging.rst>`_\ 


Troubleshooting
---------------

1. The device does not appear on lspci
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If the device does not appear after running lspci, there may be two possible reasons:

*
   Symptom:
   
   The device is not connected correctly

*
   Symptom:

   The u-boot device tree does not support pcie.

   Solution:

   To fix this, replace the ftd_file you are using on u-boot.

   .. code-block:: sh

      setenv fdt_file imx6q-sabresd-pcie.dtb


2. HDMI port is connected but there is no display
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Symptom:

On some imx devices you need to manually configure the u-boot to show video using HDMI port.

Solution:

To fix this issue you should set the u-boot to use HDMI port, defining the resolution, FPS and output format.
The configuration is "added" (do not override this) to the mmcargs:

For example on IMX6Q-Sabresd, this the default value of mmargs:

   .. code-block:: sh

      mmcargs="setenv bootargs console=${console},${baudrate} ${smp} root=${mmcroot}"

Use the following command to add the needed info to this variable:

   .. code-block:: sh
   
      setenv mmcargs "setenv bootargs console=${console},${baudrate} ${smp} root=${mmcroot} video=mxcfb0:dev=hdmi,1280x720M@30,if=RGB24"
