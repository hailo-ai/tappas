
Manual installation
===================

Manual installing TAPPAS requires preparations, our recommended method is to begin with ``Hailo SW Suite`` or ``Pre-built Docker image``.
In this guide we instruct how to install our required components manually.

.. note::
    Only Ubuntu 20.04 and 22.04 are supported


Hailort installation
--------------------

First you will need to install `HailoRT <https://github.com/hailo-ai/hailort>`_ + `HailoRT PCIe driver <https://github.com/hailo-ai/hailort-drivers>`_\ , follow HailoRT installation guide for further instructions.
And then `Make sure that HailoRT works <./verify_hailoRT.rst>`_

Preparations
------------

Download from Hailo developer zone ``tappas_VERSION_linux_installer.zip``.

x86
^^^

Unzip ``tappas_VERSION_linux_installer.zip``

.. code-block:: sh

   unzip tappas_VERSION_linux_installer.zip

Non-x86
^^^^^^^


#. 
   Unzip TAPPAS and clone HailoRT sources

   .. code-block:: sh

       unzip tappas_VERSION_linux_installer.zip

#. 
   change directory to TAPPAS, make directory named ``hailort`` and clone ``HailoRT`` sources

   .. code-block:: sh

       cd `tappas_VERSION`
       mkdir hailort
       git clone https://github.com/hailo-ai/hailort.git hailort/sources

Required Packages
-----------------

Install the following APT packages:


* ffmpeg
* x11-utils
* python3 (pip and setuptools)

.. warning::
    Python 3.6 will be deprecated in TAPPAS future version

* python3-virtualenv
* python-gi-dev
* libgirepository1.0-dev
* gcc-9 and g++-9
* cmake
* libzmq3-dev
* git

To install the above packages, run the following command:

.. code-block:: sh
    
    sudo apt-get install -y ffmpeg x11-utils python3-dev python3-pip python3-setuptools python3-virtualenv python-gi-dev libgirepository1.0-dev gcc-9 g++-9 cmake git libzmq3-dev

The following packages are required as well, and see their installation instructions below:

* `OpenCV installation`_.
* `GStreamer installation`_.
* `PyGobject installation`_.

In case any requirements are missing, a requirements table will be printed when calling manual installation.

.. _OpenCV4 installation:

OpenCV installation
-------------------

.. code-block:: sh

    # Download Opencv and unzip
    wget https://github.com/opencv/opencv/archive/4.5.2.zip 
    unzip 4.5.2.zip 

    # cd and make build dir
    cd opencv-4.5.2 
    mkdir build  
    cd build 

    # Make and install
    cmake -DOPENCV_GENERATE_PKGCONFIG=ON \
        -DBUILD_LIST=core,imgproc,imgcodecs,calib3d,features2d,flann \
        -DCMAKE_BUILD_TYPE=RELEASE \
        -DWITH_PROTOBUF=OFF -DWITH_QUIRC=OFF \
        -DWITH_WEBP=OFF -DWITH_OPENJPEG=OFF \
        -DWITH_GSTREAMER=OFF -DWITH_GTK=OFF \
        -DOPENCV_DNN_OPENCL=OFF -DBUILD_opencv_python2=OFF \
        -DINSTALL_C_EXAMPLES=ON \
        -DINSTALL_PYTHON_EXAMPLES=ON \
        -DCMAKE_INSTALL_PREFIX=/usr/local  ..

    num_cores_to_use=$(($(nproc)/2))
    make -j$num_cores_to_use
    sudo make install

    # Update the linker
    sudo ldconfig

.. _GStreamer installation:

GStreamer installation
----------------------

Run the following command to install GStreamer:

.. code-block:: sh

    apt-get install -y libcairo2-dev libgirepository1.0-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio gcc-9 g++-9 python-gi-dev

Please refer to: `GStreamer offical installation guide <https://gstreamer.freedesktop.org/documentation/installing/on-linux.html?gi-language=c#install-gstreamer-on-ubuntu-or-debian>`_ for more details

.. _PyGobject installation:

PyGobject installation
----------------------

Run the following command to install PyGobject:

.. code-block:: sh

    sudo apt install python3-gi python3-gi-cairo gir1.2-gtk-3.0

Please refer to: `PyGobject offical installation guide <https://pygobject.readthedocs.io/en/latest/getting_started.html#ubuntu-getting-started>`_ for more details

.. _TAPPAS installation section:

TAPPAS installation
-------------------

On x86, run: 

.. code-block:: sh

    ./install.sh --skip-hailort

And then, `Make sure that HailoRT works <./verify_hailoRT.rst>`_

On Raspberry Pi, run: 

.. code-block:: sh

    ./install.sh --skip-hailort --target-platform rpi

And then, `Get back to Raspberry Pi section <./raspberry-pi-install.rst>`_


Upgrade TAPPAS
--------------

To Upgrade TAPPAS, first clean GStreamer cache

.. code-block:: sh
    
    rm -rf ~/.cache/gstreamer-1.0/

Remove old ``libgsthailotools.so``

.. code-block:: sh

   rm /usr/lib/$(uname -m)-linux-gnu/gstreamer-1.0/libgsthailotools.so

And then, `TAPPAS installation section`_

Troubleshooting
---------------

Cannot allocate memory in static TLS block
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In some sceneraios (especially aarch64), you might face the following error:

.. code-block:: sh

    (gst-plugin-scanner:15): GStreamer-WARNING **: 13:58:20.557: Failed to load plugin '/usr/lib/aarch64-linux-gnu/gstreamer-1.0/libgstlibav.so': /lib/aarch64-linux-gnu/libgomp.so.1: cannot allocate memory in static TLS block 

The solution is to export an enviroment variable:

.. code-block:: sh

    export LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libgomp.so.1
