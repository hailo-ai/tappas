
Native detection application
============================

Overview
--------

This example demonstrate the use of libhailort's C API as part of a detection application. The example uses the Yolov5m model, on top of the Hailo-8 device.
The application also requires opevCV.

Compiling with meson
^^^^^^^^^^^^^^^^^^^^

Run the following commands from the application's directory.

.. code-block:: sh

   CC=gcc-9 CXX=g++-9 meson build
   ninja -C build

Running the example
^^^^^^^^^^^^^^^^^^^

.. code-block::

   ./build/detection_app


Example details
^^^^^^^^^^^^^^^

The example demonstrates the use of libhailort's C API, all functions calls are based on the header provided in ``hailort/include/hailo/hailort.h``.
The input images are located in ``input_images/`` directory and the output images are written to ``output_images/`` directory.
The application works on bitmap images with the following properties:


* 24bits per pixel
* Image size: 640x640

Code structure
--------------


* Main function:
    The main function gets the input images and passes them to ``infer`` function.

* Infer function:
    First, the function is preparing the device for inference:

  * Device initialization

    .. code-block::

       Open the Hailo PCIe device.

    **Used APIs**\ : ``hailo_create_pcie_device``

  * Configure the device from an HEF

    .. code-block::

       The next step is to create an `hailo_hef` object, and use it to configure the device for inference. Then, init an `hailo_configure_params_t` object with default values, configure the device and receive an `hailo_configured_network_group` object.


    **Used APIs**\ : ``hailo_create_hef_file()``\ , ``hailo_init_configure_params``\ , ``hailo_configure_device()``

  * Build VStreams


    * Initialize VStream parameters (both input and output).
    * Create VStreams.

    **Used APIs:** ``hailo_make_input_vstream_params``\ , ``hailo_make_output_vstream_params``\ , ``hailo_create_input_vstreams``\ , ``hailo_create_output_vstreams``

  * Activating the network group before starting inference
    **Used APIs:** ``hailo_activate_network_group()``

    Afterwards, the infer function starts the inference threads:

    * One thread for writing the data to the device using the ``write_all`` function.
      **Used APIs:** ``hailo_vstream_write_raw_buffer``

    * Three threads for receiving data from the device using the ``read_all`` function.
      **Used APIs:** ``hailo_vstream_read_raw_buffer``

    * One thread for post-processing the data received from the device, drawing the detected objects and writing the output files to the output directory.
      FeatureData is an object used for gathering the information needed for the post-processing and is created for each feature in the model.
