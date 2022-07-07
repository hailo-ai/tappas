
Yocto
=====

This section will guide through the integration of Hailo's Yocto layer's into your own Yocto
environment.

Two layers are provided by Hailo, the first one is ``meta-hailo`` which compiles the ``HailoRT`` sources, and the second one is ``meta-hailo-tappas`` which compiles the ``TAPPAS`` sources.

``meta-hailo-tappas`` is a layer that based un-top of ``meta-hailo`` that adds ``TAPPAS`` recipes.

The layers are stored in `Meta-Hailo Github <https://github.com/hailo-ai/meta-hailo.git>`_\ , with branch for each supported yocto release:


* Zeus (kernel 5.4.24)
* Dunfell (kernel 5.4.85)
* Gatesgarth (kernel 5.10.9)
* Hardknott (kernel  5.10.72)
* Honister (kernel  5.14)

 **NOTE:** Zeus and Gatesgarth will not be supported by TAPPAS from version 3.21.0

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

Tappas
^^^^^^

Add the following to your image in your ``conf/bblayers.conf``\ :

.. code-block:: sh

   BBLAYERS += "${BSPDIR}/sources/meta-hailo/meta-hailo-tappas"

Add the following to your image in your ``conf/local.conf``\ :

.. code-block:: sh

   IMAGE_INSTALL_append = "libgsthailotools tappas-apps hailo-post-processes"

Build your image
----------------

Run bitbake and build your image. After the build successfully finished, burn the Image to your embedded device.

   **NOTE:** building on non-IMX devices:
             To increase the performance of our applications, we patched imx gstreamer-plugins-base.
             In non-IMX devices you may encounter an error indicating that recipes under ``meta-hailo-tappas/recipes-multimedia/gstreamer/`` cannot be parsed.
             In this case remove this directory under the meta-hailo-tappas layer, and re-build the image.

             .. code-block:: sh

               rm -rf meta-hailo/meta-hailo-tappas/recipes-multimedia/gstreamer/



Copy the IMX apps
^^^^^^^^^^^^^^^^^

From within the x86 container, copy the ``imx`` apps into the embedded device using your preferred way of copying

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
the source files located in the TAPPAS release under ``core/hailo/gstreamer``.
The recipe compiles with meson and copies the ``libgsthailotools.so`` file to ``/usr/lib/gstreamer-1.0`` 
on the target device's root file system.

tappas-apps
^^^^^^^^^^^

Hailo's TAPPAS embedded application recipe, including GStreamer apps for embedded.
The recipe copies the app script, the hef and media files to /home/root/apps/.
Depends on GStreamer, opencv, cxxopts, xtensor and xtl.

hailo-post-processes
^^^^^^^^^^^^^^^^^^^^

the recipe compiles and copies the post processes to ``/usr/lib/hailo-post-processes``.
Deppends on opencv, xtensor, xtl, rapidjson and cxxopts.