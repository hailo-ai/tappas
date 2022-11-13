===================
Compiling Your Code
===================

When building the ``TAPPAS`` `Docker <../installation/docker-install.rst>`_ (or `installing natively <../installation/manual-install.rst>`_\ ), all of the ``TAPPAS`` source files are compiled and ready to use. If however you want to add your own additions (\ `for example, a new postprocess <write-your-own-postprocess.rst>`_\ ) or make other changes, then you will need to recompile the sources. This guide covers the build system used in ``TAPPAS`` and how you can compile your project.

The Meson Build System
----------------------

`Meson <https://mesonbuild.com/>`_ is an open source build system that puts an emphasis on speed and ease of use. `GStreamer uses meson <https://gstreamer.freedesktop.org/documentation/installing/building-from-source-using-meson.html?gi-language=c>`_ for all subprojects to generate build instructions to be executed by `ninja <https://ninja-build.org/>`_\ , another build system focused soley on speed that requires a higher level build system (ie: meson) to generate its input files. \
Like GStreamer, ``TAPPAS`` also uses meson, and compiling new projects requires adjusting the ``meson.build`` files.

How to Compile
--------------

| To help streamline this process we have gone ahead and provided a script that handles most of the work. You can find this script at `scripts/gstreamer/install_hailo_gstreamer.sh <../../scripts/gstreamer/install_hailo_gstreamer.sh>`_.
| The following arguments are available:  


* | ``--build-dir``   Path to the build directory. Defaults to ``core/hailo``.
* | ``--build-mode`` Build mode, debug/release, default is release.
* | ``--skip-hailort``  Skip compiling HailoRT. 
* | ``--python-version`` specify what Python version to use.
* | ``--compile-libgsthailo`` Compile libgsthailo instead of copying it from the release.
  | From the Tappas home directory folder you can run:

.. code-block:: sh

   ./scripts/gstreamer/install_hailo_gstreamer.sh

.. image:: ../resources/compiling.png
