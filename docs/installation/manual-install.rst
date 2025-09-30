
Manual Installation
===================

The manual installation of TAPPAS requires preparation, Hailo's recommended method is to begin with ``Hailo SW Suite``.
This guide will instruct how to install the required components manually.

.. note::
    Only Ubuntu 20.04 and 22.04 are supported


Hailort Installation
--------------------

First `HailoRT <https://github.com/hailo-ai/hailort>`_ + `HailoRT PCIe driver <https://github.com/hailo-ai/hailort-drivers>`_\ , needs to be installed. Follow the HailoRT installation guide for further instructions.
After the installation, `confirm that HailoRT is working correctly <./verify_hailoRT.rst>`_.

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
   Change the directory to TAPPAS, and create a directory named ``hailort`` and clone ``HailoRT`` sources

   .. code-block:: sh

       cd `tappas_VERSION`
       mkdir hailort
       git clone https://github.com/hailo-ai/hailort.git hailort/sources

.. note::
  In some cases, HailoRT (which is installed via TAPPAS) is not the latest version, while the driver,
  which was installed before HailoRT and TAPPAS - has a different version.
  In these cases, this error will occur:

    "Could not find a configuration file for package "HailoRT" that exactly
    matches requested version."

  Meaning, HailoRT library and driver installed versions are mismatched - while
  they are required to be fully identical.

  In such a case - re-install the driver version which is compatible to your installed HailoRT version.

Required Packages
-----------------

The following APT packages need to be installed, using the command below:


* ffmpeg
* x11-utils
* python3 (pip and setuptools).
* python3-virtualenv
* python-gi-dev
* libgirepository1.0-dev
* cmake
* libzmq3-dev
* git
* rsync
* gcc (>= gcc-9)
* g++ (>= g++-9)


To install the above packages, run the following command:

.. code-block:: sh
    
    sudo apt-get install -y rsync ffmpeg x11-utils python3-dev python3-pip python3-setuptools python3-virtualenv python-gi-dev libgirepository1.0-dev gcc-12 g++-12 cmake git libzmq3-dev

.. note::
    The gcc and g++ version depends on the OS support. The versions shown in the example (gcc-12, g++-12) are recommended, but ensure the version is at least gcc-9 and g++-9 or higher.

The following packages are required as well, and their installation instructions can be viewed from the links below:

* `OpenCV installation`_.
* `GStreamer installation`_.
* `PyGobject installation`_.

In case any requirements are missing, a requirements table will be printed when calling manual installation.

.. _OpenCV4 installation:

OpenCV Installation
-------------------
To install OpenCV, run the following commands:

.. code-block:: sh
    
    sudo apt-get install -y libopencv-dev python3-opencv

To check your OpenCV version, run the following command:

.. code-block:: sh

    # To check the OpenCV version installed 
    pkg-config --modversion opencv4

.. tip::

    If you are running on an old OS the apt-get version might be too old (You will be notified on the next steps), you can install OpenCV manually as shown below.

Opencv compilation from source
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
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

GStreamer Installation
----------------------

Run the following command to install GStreamer:

.. code-block:: sh

    sudo apt-get install -y libcairo2-dev libgirepository1.0-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio gcc-12 g++-12 python-gi-dev

Please refer to: `GStreamer official installation guide <https://gstreamer.freedesktop.org/documentation/installing/on-linux.html?gi-language=c#install-gstreamer-on-ubuntu-or-debian>`_ for more details

.. _PyGobject installation:

PyGobject Installation
----------------------

Run the following command to install PyGobject:

.. code-block:: sh

    sudo apt install python3-gi python3-gi-cairo gir1.2-gtk-3.0

Please refer to: `PyGobject official installation guide <https://pygobject.readthedocs.io/en/latest/getting_started.html#ubuntu-getting-started>`_ for more details

.. _TAPPAS installation section:

TAPPAS Installation
-------------------

On most platforms (such as x86-based platforms), run:

.. code-block:: sh

    ./install.sh --skip-hailort

and then, `Make sure that HailoRT works <./verify_hailoRT.rst>`_

On Rockchip, run: 

.. code-block:: sh

    ./install.sh --skip-hailort --target-platform rockchip

and then, `return to the Rockchip section <./rockchip.rst>`_.

Upgrade TAPPAS
--------------

To Upgrade TAPPAS, first clean the GStreamer cache

.. code-block:: sh
    
    rm -rf ~/.cache/gstreamer-1.0/

Remove old ``libgsthailotools.so``

.. code-block:: sh

   rm /usr/lib/$(uname -m)-linux-gnu/gstreamer-1.0/libgsthailotools.so

and then, `TAPPAS installation section`_

Troubleshooting
---------------

Cannot allocate memory in static TLS block
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In some scenarios (especially ``aarch64``), this warning might occur:

.. code-block:: sh

    (gst-plugin-scanner:15): GStreamer-WARNING **: 13:58:20.557: Failed to load plugin '/usr/lib/aarch64-linux-gnu/gstreamer-1.0/libgstlibav.so': /lib/aarch64-linux-gnu/libgomp.so.1: cannot allocate memory in static TLS block 

The solution is to export an environment variable:

.. code-block:: sh

    export LD_PRELOAD=/usr/lib/aarch64-linux-gnu/libgomp.so.1

PCIe descriptor page size error
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
If you encounter the following error: (actual page size might vary)

.. code-block:: sh

    [HailoRT] [error] CHECK_AS_EXPECTED failed - max_desc_page_size given 16384 is bigger than hw max desc page size 4096"

Some hosts doesn't support certain PCIe descriptor page size.
in order to overcome this issue add the text below to /etc/modprobe.d/hailo_pci.conf (create the file if it doesn't exist)

.. code-block:: sh

    options hailo_pci force_desc_page_size=4096
    # you can do this by running the following command:
    echo 'options hailo_pci force_desc_page_size=4096' >> /etc/modprobe.d/hailo_pci.conf

Reboot the machine for this change to take effect. You can also reload the driver without rebooting by running the following commands:

.. code-block:: sh

    modprobe -r hailo_pci
    modprobe hailo_pci
