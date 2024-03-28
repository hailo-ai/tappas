
Yocto
=====

This section will guide through the integration of Hailo's Yocto layer's into your own Yocto
environment.

Two layers are provided by Hailo, the first one is ``meta-hailo`` which compiles the ``HailoRT`` sources, and the second one is ``meta-hailo-tappas`` which compiles the ``TAPPAS`` sources.

``meta-hailo-tappas`` is a layer that based un-top of ``meta-hailo`` that adds ``TAPPAS`` recipes.

The layers are stored in `Meta-Hailo Github <https://github.com/hailo-ai/meta-hailo.git>`_\ , with branch for each supported yocto release:


* Dunfell 3.1 (kernel 5.4.85)
* Kirkstone 4.0 (kernel 5.15)

.. warning:: On i.MX8-based devices Kirkstone branch does not support OpenGL, therefore, the Kirkstone applications portfolio is reduced.

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

   IMAGE_INSTALL_append = "hailo-firmware libhailort hailortcli hailo-pci libgsthailo"

TAPPAS
^^^^^^

Add the following to your image in your ``conf/bblayers.conf``\ :

.. code-block:: sh

   BBLAYERS += "${BSPDIR}/sources/meta-hailo/meta-hailo-tappas"

Add the following to your image in your ``conf/local.conf``\ :

.. code-block:: sh

   IMAGE_INSTALL_append = "libgsthailotools tappas-apps hailo-post-processes tappas-tracers"

Build your image
----------------

Run bitbake and build your image. After the build successfully finished, burn the Image to your embedded device.

.. note::
    building on non-IMX devices:
    To increase the performance of our applications, we patched imx gstreamer-plugins-base.
    In non-IMX devices you may encounter an error indicating that recipes under ``meta-hailo-tappas/recipes-multimedia/gstreamer/`` cannot be parsed.
    In this case remove this directory under the meta-hailo-tappas layer, and re-build the image.

    .. code-block:: sh

        rm -rf meta-hailo/meta-hailo-tappas/recipes-multimedia/gstreamer/


Validating the integration's success
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
root file system, make it loadable by GStreamer as a plugin.

libgsthailotools
^^^^^^^^^^^^^^^^

Hailo's TAPPAS gstreamer elements. Depends on ``libgsthailo``, GStreamer, opencv, xtensor and xtl.
The source files located in the TAPPAS release under ``core/hailo``.
The recipe compiles with meson and copies the ``libgsthailotools.so`` file to ``/usr/lib/gstreamer-1.0`` 
on the target device's root file system.

tappas-apps
^^^^^^^^^^^

Hailo's TAPPAS embedded application recipe, including GStreamer apps for embedded.
The recipe copies the app script, the hef and media files to /home/root/apps/.
Depends on GStreamer, opencv, cxxopts, xtensor and xtl.

hailo-post-processes
^^^^^^^^^^^^^^^^^^^^

The recipe compiles and copies the post processes to ``/usr/lib/hailo-post-processes``.
Deppends on opencv, xtensor, xtl, rapidjson and cxxopts.

tappas-tracers
^^^^^^^^^^^^^^
Hailo's TAPPAS gstreamer tracers. Depends on ``libgsthailo`` and GStreamer.
The source files located in the TAPPAS release under ``core/hailo/tracers``.
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

Using this command we add the needed info to this variable:

   .. code-block:: sh
   
      setenv mmcargs "setenv bootargs console=${console},${baudrate} ${smp} root=${mmcroot} video=mxcfb0:dev=hdmi,1280x720M@30,if=RGB24"
