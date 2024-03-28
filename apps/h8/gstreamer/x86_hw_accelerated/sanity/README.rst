
Sanity pipeline
===============

Overview
--------

Sanity apps purpose is to help you verify that all the required components have been installed successfully.
This sanity app helps you to verify that the x86 accelerators are installed correctly.

First of all, you would need to run ``sanity.sh`` and make sure that the image presented looks like the one that would be presented later.

Sanity GStreamer
^^^^^^^^^^^^^^^^

This app should launch first.


.. note::
    Open the source code in your preferred editor to see how simple this app is.


In order to run the app just ``cd`` to the ``sanity`` directory and launch the app

.. code-block:: sh

   cd $TAPPAS_WORKSPACE/apps/h8/gstreamer/x86_hw_accelerated/sanity
   ./sanity.sh

The output should look like:


.. image:: readme_resources/sanity.png
   :width: 300px
   :height: 250px


If the output is similar to the image shown above, you are good to go to the next verification phase!
