=========================
Compiling the Application
=========================

The application and all required peripherial files (.HEFs, .JSONs, etc...) should already come pre-loaded any image built with the TAPPAS yocto layer. If for some reason
you would like to re-compile the application, you can do so by following the instructions below.

This application serves as a stand-alone example that depends on TAPPAS packages along with other libraries and tools
from the Hailo-15 software suite. The application uses the `Meson build system <https://mesonbuild.com/>`_ to compile the source code, and requires a 
toolchain if it is intended to be cross compiled.

A script is provided to help you build this project, all it requires is to have the Hailo-15 SDK installed on your machine. To run the script, execute the following command:

.. code-block:: sh

    $ ./tools/cross_compiler/cross_compile_native_apps.py hailo15 release <path-to-your-toolchain>

Flags
-----

.. code-block:: sh

    $ ./cross_compile_native_apps.py --help
        usage: cross_compile_native_apps.py [-h] [--remote-machine-ip REMOTE_MACHINE_IP] [--clean-build-dir] [--install-to-rootfs] {hailo15} {debug,release} toolchain_dir_path

        Cross-compile TAPPAS

        positional arguments:
        {hailo15}             Target platform to compile to
        {debug,release}       Build and compilation type
        toolchain_dir_path    Toolchain directory path

        options:
        -h, --help            show this help message and exit
        --remote-machine-ip REMOTE_MACHINE_IP
                                remote machine ip
        --clean-build-dir     Delete previous build cache (default false)
        --install-to-rootfs   Install to rootfs (default false)
    
Toolchain
---------

For more details on how to obtain a toolchain, see the documentation in `here <./../../../../../tools/cross_compiler/README.rst>`_.