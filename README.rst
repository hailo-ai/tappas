Hailo TAPPAS - Optimized Execution of Video-Processing Pipelines
================================================================

.. |gstreamer| image:: https://img.shields.io/badge/gstreamer-1.16%20%7C%201.18%20%7C%201.20-blue
   :target: https://gstreamer.freedesktop.org/
   :alt: Gstreamer 1.16 | 1.18 | 1.20
   :width: 150
   :height: 20

.. |hailort| image:: https://img.shields.io/badge/HailoRT-4.17.0-green
   :target: https://github.com/hailo-ai/hailort
   :alt: HailoRT
   :height: 20


.. |license| image:: https://img.shields.io/badge/License-LGPLv2.1-green
   :target: https://github.com/hailo-ai/tappas/blob/master/LICENSE
   :alt: License: LGPL v2.1
   :height: 20

.. |check_mark| image:: ./resources/check_mark.png
  :width: 20
  :align: middle

.. image:: ./resources/github_Tappas_Mar24.jpg
  :height: 300
  :width: 600
  :align: center


.. raw:: html

   <div align="center">
      <img src="./apps/h8/gstreamer/general/cascading_networks/readme_resources/cascading_app.gif"/>
   </div>

|gstreamer| |hailort| |license|

----

Overview
--------

TAPPAS is Hailo's set of full application examples, implementing pipeline elements and
pre-trained AI tasks.

Demonstrating Hailo's system integration scenario of specific use cases on predefined systems
(software and Hardware platforms). It can be used for evaluations, reference code and demos:

* Accelerating time to market by reducing development time and deployment effort
* Simplifying integration with Hailo’s runtime SW stack
* Providing a starting point for customers to fine-tune their applications

.. image:: ./resources/HAILO_TAPPAS_SW_STACK.svg


----

Getting Started
---------------

Prerequisites
^^^^^^^^^^^^^


* Hailo-8 device
* HailoRT PCIe driver installed
* At least 6GB's of free disk space


.. note::
    This version is compatible with HailoRT v4.17.


Installation
^^^^^^^^^^^^

.. list-table::
   :header-rows: 1

   * - Option
     - Instructions
     - Supported OS
   * - **Hailo SW Suite***
     - `SW Suite Install guide <docs/installation/sw-suite-install.rst>`_
     - Ubuntu x86 20.04, Ubuntu x86 22.04
   * - Pre-built Docker image
     - `Docker install guide <docs/installation/docker-install.rst>`_
     - Ubuntu x86 20.04, Ubuntu x86 22.04, Ubuntu aarch64 20.04 (64-bit)
   * - Manual install
     - `Manual install guide <docs/installation/manual-install.rst>`_
     - Ubuntu x86 20.04, Ubuntu x86 22.04, Ubuntu aarch64 20.04
   * - Yocto installation
     - `Read more about Yocto installation <docs/installation/yocto.rst>`_
     - Yocto supported BSP's
   * - Raspberry pi installation
     - `Read more about Raspberry pi installation <docs/installation/raspberry-pi-install.rst>`_
     - Raspberry Pi OS, Ubuntu 20.04



``* It is recommended to start your development journey by first installing the Hailo SW Suite``

Documentation
^^^^^^^^^^^^^

* `Framework architecture and elements documentation <docs/TAPPAS_architecture.rst>`_
* `Guide to writing your own C++ postprocess element <docs/write_your_own_application/write-your-own-postprocess.rst>`_
* `Guide to writing your own Python postprocess element <docs/write_your_own_application/write-your-own-python-postprocess.rst>`_
* `Debugging and profiling performance <docs/write_your_own_application/debugging.rst>`_
* `Cross compile <tools/cross_compiler/README.rst>`_ - A guide for cross-compiling

----

Example Applications Built with TAPPAS
--------------------------------------

TAPPAS comes with a rich set of pre-configured pipelines optimized for different common hosts.


.. important:: 
    * All example applications utilize both the host (for non-neural tasks) and the Neural-Network Core
      (for neural-networks inference), therefore performance results are affected by the host.
    * General application examples do not include any architecture-specific accelerator usage,
      and therefore will provide the easiest way to run an application, but with sub-optimal performance.
    * Architecture-specific application examples (i.MX, Raspberry PI, etc.) use platform-specific
      hardware accelerators and are not compatible with different architectures.

.. note::
    All i.MX example application are validated on i.MX8 and i.MX6 platforms and are compatible with the architectures.

.. note::
    Running application examples requires a direct connection to a monitor.

Basic Single Network Pipelines
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Pipelines that run a single network. The diagram below shows the pipeline data-flow.


.. image:: resources/single_net_pipeline.jpg


The following table details the currently available examples.

.. list-table::
   :header-rows: 1
   :stub-columns: 1
   :widths: 40 12 12 12 12 12
   :align: center

   * - 
     - `General <apps/h8/gstreamer/general/README.rst>`_
     - `i.MX8 <apps/h8/gstreamer/imx8/README.rst>`_
     - `RPi4 <apps/h8/gstreamer/raspberrypi/README.rst>`_
     - `x86 Hardware Accelerated <apps/h8/gstreamer/x86_hw_accelerated/README.rst>`_
     - `Rockchip <apps/h8/gstreamer/rockchip/README.rst>`_
   * - Object Detection
     - |check_mark|
     - |check_mark|
     - |check_mark|
     - 
     - |check_mark|
   * - Depth Estimation
     - |check_mark|
     - |check_mark|
     - |check_mark|
     - 
     - 
   * - Instance segmentation
     - |check_mark|
     - 
     - 
     - 
     - 
   * - Classification with Python Postprocessing
     - |check_mark|
     - 
     - 
     - 
     - 
   * - Object Detection Multiple Devices (Century)
     - |check_mark|
     - 
     - 
     - |check_mark|
     - 
   * - Face Recognition
     - |check_mark|
     - 
     - 
     - 
     - 


Two Network Pipelines
^^^^^^^^^^^^^^^^^^^^^

Examples of basic pipelines running two networks.
The parallel networks pipeline is an simple extension of the single network pipeline and is shown in the following diagram:

.. image:: resources/parallel_nets_pipeline.png


The cascaded (serial) flow shows two networks running in series. This example pipeline is of the popular configuration where the first network is a detector which finds some Region-of-Interest (ROI) in the input image and the second network processes the cropped ROI (a face-detection-and-landmarking use case of this pipeline is shown at the top of this guide). The pipeline is shown in the following diagram:


.. image:: resources/cascaded_nets_pipeline.png


.. list-table::
   :header-rows: 1
   :stub-columns: 1
   :widths: 40 12 12 12 12 12
   :align: center

   * - 
     - `General <apps/h8/gstreamer/general/README.rst>`_
     - `i.MX8 <apps/h8/gstreamer/imx8/README.rst>`_
     - `RPi4 <apps/h8/gstreamer/raspberrypi/README.rst>`_
     - `x86 Hardware Accelerated <apps/h8/gstreamer/x86_hw_accelerated/README.rst>`_
     - `Rockchip <apps/h8/gstreamer/rockchip/README.rst>`_
   * - Cascaded - Face Detection & Landmarks
     - |check_mark|
     - 
     - |check_mark|
     - 
     - 
   * - Cascaded - Person Det & Single Person Pose Estimation
     - |check_mark|
     - |check_mark|
     - 
     - 
     - 
   * - Cascaded - Face Detection & Recognition
     - |check_mark|
     - 
     - 
     - 
     - 


Multi-Stream Pipelines
^^^^^^^^^^^^^^^^^^^^^^

.. image:: docs/resources/one_network_multi_stream.png


.. list-table::
   :header-rows: 1
   :stub-columns: 1
   :widths: 40 12 12 12 12 12 
   :align: center

   * - 
     - `General <apps/h8/gstreamer/general/README.rst>`_
     - `i.MX8 <apps/h8/gstreamer/imx8/README.rst>`_
     - `RPi4 <apps/h8/gstreamer/raspberrypi/README.rst>`_
     - `x86 Hardware Accelerated <apps/h8/gstreamer/x86_hw_accelerated/README.rst>`_
     - `Rockchip <apps/h8/gstreamer/rockchip/README.rst>`_
   * - Multi-stream Object Detection
     - |check_mark|
     -
     - 
     - |check_mark|
     - |check_mark|
   * - Multi-stream Multi-Device Object Detection
     - |check_mark|
     - 
     - 
     - 
     - 
     


Pipelines for High-Resolution Processing Via Tiling
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. image:: docs/resources/tiling-example.png


.. list-table::
   :header-rows: 1
   :stub-columns: 1
   :widths: 40 12 12 12 12 12
   :align: center

   * - 
     - `General <apps/h8/gstreamer/general/README.rst>`_
     - `i.MX8 <apps/h8/gstreamer/imx8/README.rst>`_
     - `RPi4 <apps/h8/gstreamer/raspberrypi/README.rst>`_
     - `x86 Hardware Accelerated <apps/h8/gstreamer/x86_hw_accelerated/README.rst>`_
     - `Rockchip <apps/h8/gstreamer/rockchip/README.rst>`_
   * - HD Object Detection
     - |check_mark|
     - 
     - 
     - 
     - |check_mark|


Example Use Case Pipelines
^^^^^^^^^^^^^^^^^^^^^^^^^^

Our LPR reference application demonstrates the use of 3 networks, with a database.
The pipeline demonstrates inference based decision making (Vehicle detection) for secondary inference tasks (License plate data extraction). This allows multiple networks to cooperate in the pipeline for reactive behavior.


.. image:: resources/lpr_pipeline.png

Our Multi-Person Multi-Camera Tracking reference application demonstrates person tracking across multiple streams using RE-ID tracking.
The pipeline demonstrates another method for inference based decision making that also connects between different video streams.


.. image:: resources/re_id_pipeline.png

Our VMS reference application demonstrates the use of 5 networks over multiple streams, and is a reference for video management system applications.


.. image:: resources/vms_pipeline.png


.. list-table::
   :header-rows: 1
   :stub-columns: 1
   :widths: 40 12 12 12 12 12
   :align: center

   * - 
     - `General <apps/h8/gstreamer/general/README.rst>`_
     - `i.MX8 <apps/h8/gstreamer/imx8/README.rst>`_
     - `RPi4 <apps/h8/gstreamer/raspberrypi/README.rst>`_
     - `x86 Hardware Accelerated <apps/h8/gstreamer/x86_hw_accelerated/README.rst>`_
     - `Rockchip <apps/h8/gstreamer/rockchip/README.rst>`_
   * - LPR
     - |check_mark|
     - |check_mark|
     - 
     - 
     - |check_mark|
   * - RE-ID
     - |check_mark|
     - 
     - 
     - 
     - 


----


Changelog
----------

**v3.28.0 (April 2024)**

* Tappas was updated in this version, with a revised list of supported platform and apps
* Added ``yolov8`` (as default) to Detection application examples
* Fixed various stability issues across apps and platforms
* This release is aligned to Hailo-15 Vision Processor Software Package 2024-04

**v3.27.2 (March 2024)**

* Aligned to Hailo-15 Vision Processor Software Package 2024-01.2
* Added a new example application - Frontend Update
* Updated the hailo OSD API
* Detection application now works with an updated ``hailonet`` element
* Various bugs fixes

**v3.27.1 (February 2024)**

* Aligned to Hailo-15 Vision Processor Software Package 2024-01.1
* Added a new Hailo-15 external host application that saves udp stream to file 

**v3.27.0 (January 2024)**

* Updated the Hailo-15 applications to use the updated Media Library implementation:

  * Basic Security Camera (streaming)
  * Detection
  * Single Stream OSD (On-Screen Display)

* Added a folder for external host scripts and added the UDP Stream Display script

.. note:: TAPPAS supports both Hailo-15 and Hailo-8. Temporarily, in this version, only the
  following Hailo-8 based example applications are supported:

    * Detection

      * yolov5
      * mobilenet_ssd

    * Multi-Stream Detection

      * Multi-Stream Detection
      * MultiStream Detection with Stream Multiplexer

    * License Plate Recognition

  These applications are supported under the general folder (x86-based platforms).

**v3.26.2 (December 2023)**

* Aligned to Hailo-15 Vision Processor Software Package 2023-10.2
* Add grayscale support for Media Library Front-end
* Various bug fixes for Hailo-15 pipelines

**v3.26.1 (November 2023)**

* Aligned to Hailo-15 Vision Processor Software Package 2023-10.1
* Updated OSD configuration to support new dynamic features and adjust to the updated Media Library implementation
* Added a script for displaying UDP streams
* Basic security camera (Media Library implementation) now support 5 outputs

**v3.26.0 (October 2023)**

* Added Hailo-15 supported application examples:

  * Detection
  * License Plate Recognition
  * A new Hailo-15 specific example application - Basic Security Camera (streaming)

* Removed Yolact models support from Instance Segmentation
* Various bug fixes:

  * Fixed the Detection application on i.MX6 platforms
  * Fixed an issue with Face Recognition which prevented faces to be recognized ins some scenarios
  * Fixed an issue which caused a warning when running some networks

**v3.25.0 (July 2023)**

* Improved Yolov5seg post-process performance
* Updated Yolo networks to use the HailoRT native post-process (selected models)
* Added "non-blocking mode" and "wait-time" properties to hailoroundrobin element

**v3.24.0 (March 2023)**

* Added support for `Rockchip RK3588 <apps/h8/gstreamer/rockchip/README.rst>`_ (validated on Firefly ITX-3588J platform)
* Video Management System now supports multi-device (Ubuntu 22.04 only)
* Video Management System (single device) now works on Ubuntu 20.04
* Added a new model to `Instance Segmentation Pipeline <apps/h8/gstreamer/general/instance_segmentation/README.rst>`_:

  * `yolov5seg` - which has improved performance compared to `yolact`

* New applications for `i.MX8 <apps/h8/gstreamer/imx8/README.rst>`_:

  * Object Detection and Pose Estimation (cascaded)
  * Multi-Stream Detection

* Added a TAPPAS Graphic User Interface to easily run selected general example applications (preview) on the TAPPAS Docker - to activate it, run `tappas-gui`
* Added back `yolox_l_leaky` to the `Century general application <apps/h8/gstreamer/general/century/README.rst>`_
* Reduced docker size


**v3.23.1 (February 2023)**

* Updated to HailoRT 4.12.1
* Fixed a documentation mistake in `Writing your own Python postprocess <docs/write_your_own_application/write-your-own-python-postprocess.rst>`_


**v3.23.0 (December 2022)**

* New Apps:

  * Added `x86_hw_accelerated <apps/h8/gstreamer/x86_hw_accelerated/README.rst>`_ example pipelines
    that use Video Acceleration API (VA-API) over Intel processors that support
    `Quick Sync <https://en.wikipedia.org/wiki/Intel_Quick_Sync_Video>`_:

    * Video Management System -
      a pipeline that demonstrates a VMS application which runs several streams and different tasks - Face Recognition,
      Face Attributes and Person Attributes. Currently this example pipeline is supported on Ubuntu 22.04 only
    * `Multi-stream detection <apps/h8/gstreamer/x86_hw_accelerated/multistream_detection/README.rst>`_
    * `Century <apps/h8/gstreamer/x86_hw_accelerated/century/README.rst>`_

  * Pose Estimation pipeline with two cascading networks - `Person detection and single person pose estimation <apps/h8/gstreamer/general/cascading_networks/README.rst>`_
  * `Face recognition <apps/h8/gstreamer/general/face_recognition/README.rst>`_
  * Updated i.MX6 Object Detection App - New network, updated the pipeline to include i.MX6 hardware acceleration

* Added new models to `Instance Segmentation Pipeline <apps/h8/gstreamer/general/instance_segmentation/README.rst>`_:

  * yolact_regnetx_1.6gf
  * yolact_regnetx_800mf (80 classes) 

* `Century app <apps/h8/gstreamer/general/century/README.rst>`_ now uses a new network (yolov5m)
* `Multi-Camera Multi-Person Tracking (RE-ID) <apps/h8/gstreamer/general/multi_person_multi_camera_tracking/README.rst>`_  -  Improved pipeline performance and accuracy
* Added support for Ubuntu 22.04 (release-grade)

**v3.22.0 (November 2022)**

* New element `hailoimportzmq` - provides an entry point for importing metadata exported by `hailoexportzmq` (HailoObjects) into the pipeline
* Added Depth Estimation, Object Detection and Classification pipelines for i.MX6 Pipelines
* Changed the debugging tracers to use an internal tracing mechanism  

**v3.21.0 (October 2022)**

* New Apps:
  
  * `Multi-stream detection that uses HailoRT Stream Multiplexer <apps/h8/gstreamer/general/multistream_detection/README.rst>`_ - Demonstrates the usage of HailoRT stream multiplexer (preview)

* New elements - `hailoexportfile` and `hailoexportmq` which provide an access point in the pipeline to export metadata (HailoObjects)
* Improved pipeline profiling by adding new tracers and replacing the GUI of `gst-shark <docs/write_your_own_application/debugging.rst>`_
* Ubuntu 22 is now supported (GStreamer 1.20, preview)
* Yocto Kirkstone is now supported (GStreamer 1.20)

**v3.20.0 (August 2022)**

* New Apps:
  
  * `Detection every X frames pipeline <apps/h8/gstreamer/general/detection/README.rst>`_ - Demonstrates the ability of skipping frames using a tracker

* Improvements to Multi-Camera Multi-Person Tracking (RE-ID) pipeline (released)

**v3.19.1 (July 2022)**

* New Apps:
  
  * Multi-Camera Multi-Person Tracking (RE-ID) pipeline `multi_person_multi_camera_tracking.sh <apps/h8/gstreamer/general/multi_person_multi_camera_tracking/README.rst>`_ (preview)

**v3.19.0 (June 2022)**

* New Apps:

  * Added Cascading networks, Depth Estimation, Pose Estimation and Semantic Segmentation pipelines for `i.MX Pipelines <apps/h8/gstreamer/imx8/README.rst>`_

* Added an option to control post-process parameters via a JSON configuration for the detection application
* Added support for Raspberry Pi Raspbian OS
* Native Application now uses TAPPAS post-process
* LPR (License Plate Recognition) pipeline is simplified to bash only
* New detection post-process - Nanodet

.. note::
    Ubuntu 18.04 will be deprecated in TAPPAS future version

.. note::
    Python 3.6 will be deprecated in TAPPAS future version

**v3.18.0 (April 2022)**

* New Apps:

  * LPR (License Plate Recognition) pipeline and facial landmark pipeline for `i.MX Pipelines <apps/h8/gstreamer/imx8/README.rst>`_

* Added the ability of compiling a specific TAPPAS target (post-processes, elements)
* Improved the performance of Raspberry Pi example applications


**v3.17.0 (March 2022)** 

* New Apps:

  * LPR (License Plate Recognition) pipeline for `General Pipelines <apps/h8/gstreamer/general/README.rst>`_ (preview)
  * Detection & pose estimation app
  * Detection (MobilenetSSD) - Multi scale tiling app

* Update infrastructure to use new HailoRT installation packages
* Code is now publicly available on `Github <https://github.com/hailo-ai/tappas>`_
   

**v3.16.0 (March 2022)** 
   
* New Apps:

  * Hailo `Century <https://hailo.ai/product-hailo/hailo-8-century-evaluation-platform/>`_ app - Demonstrates detection on one video file source over 6 different Hailo-8 devices
  * Python app - A classification app using a post-process written in Python

* New Elements:

  * Tracking element "HailoTracker" - Add tracking capabilities
  * Python element "HailoPyFilter" - Enables to write post-processes using Python

* Yocto Hardknott is now supported
* Raspberry Pi 4 Ubuntu dedicated apps
* HailoCropper cropping bug fixes
* HailoCropper now accepts cropping method as a shared object (.so)


**v3.14.1 (March 2022)** 

* Fix Yocto Gatesgarth compilation issue
* Added support for hosts without X-Video adapter


**v3.15.0 (February 2022)** 

* New Apps:

  * Detection and depth estimation - Networks switch app
  * Detection (MobilenetSSD) - Single scale tilling app


**v3.14.0 (January 2022)**

* New Apps:

  * Cascading apps - Face detection and then facial landmarking

* New Yocto layer - Meta-hailo-tappas
* Window enlargement is now supported
* Added the ability to run on multiple devices
* Improved latency on Multi-device RTSP app


**v3.13.0 (November 2021)**

* Context switch networks in multi-stream apps are now supported
* New Apps:

  * Yolact - Instance segmentation
  * FastDepth - Depth estimation
  * Two networks in parallel on the same device - FastDepth + Mobilenet SSD
  * Retinaface

* Control Element Integration - Displaying device stats inside a GStreamer pipeline (Power, Temperature)
* New Yocto recipes - Compiling our GStreamer plugins is now available as a Yocto recipe
* Added a C++ detection example (native C++ example for writing an app, without GStreamer)

   
**v3.12.0 (October 2021)** 

* Detection app - MobilenetSSD added
* NVR multi-stream multi device app (detection and pose estimation)
* Facial Landmarks app
* Segmentation app
* Classification app
* Face detection app
* Hailomuxer gstreamer element
* Postprocess implementations for various networks
* GStreamer infrastructure improvements
* Added ARM architecture support and documentation

  
**v3.11.0 (September 2021)**

* GStreamer based initial release
* NVR multi-stream detection app
* Detection app
* Hailofilter gstreamer element
* Pose Estimation app