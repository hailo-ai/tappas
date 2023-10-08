
Sanity Pipeline
===============

Overview
--------

Sanity apps purpose is to help verify that all the required components have been installed successfully.

First of all, ``sanity_gstreamer.sh`` needs to be run to ensure that the image presented looks like the one that will be presented later.

Sanity GStreamer
^^^^^^^^^^^^^^^^

This app should launch first.


.. note::
    Open the source code in your preferred editor to see how simple this app is.


In order to run the app just ``cd`` to the ``sanity_pipeline`` directory and launch the app

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/general/sanity_pipeline
   ./sanity_gstreamer.sh

The display should look like the image below:


.. image:: readme_resources/sanity_gstreamer.png
   :width: 300px
   :height: 250px


If the output is similar to the image shown above, continue to the next verification phase.
