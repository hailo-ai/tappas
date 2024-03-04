#!/usr/bin/env python
import argparse
import logging
import os
import sys
from pathlib import Path

from common import (FOLDER_NAME, Arch, MesonInstaller, Target,
                    install_compilers_apt_packages)

POSSIBLE_BUILD_TYPES = ['debug', 'release']
POSSIBLE_BUILD_LIBS = ['all', 'apps', 'plugins', 'libs', 'tracers']
TAPPAS_WORKSPACE = FOLDER_NAME.parent.parent.resolve()


class TappasInstaller(MesonInstaller):
    INCLUDES = [
        '/usr/include/hailort',
        '/usr/include/gst-hailo/metadata',
    ]
    LIBARGS_TEMPLATE = "{}-std=c++17"

    def __init__(self, arch, target, build_type, toolchain_dir_path, build_lib='all',
                 install_to_rootfs=False, remote_machine_ip=None, clean_build_dir=False):
        super().__init__(arch=arch, build_type=build_type, src_build_dir=TAPPAS_WORKSPACE / "core/hailo",
                         toolchain_dir_path=toolchain_dir_path, remote_machine_ip=remote_machine_ip,
                         clean_build_dir=clean_build_dir, install_to_toolchain_rootfs=install_to_rootfs)
        self._target_platform = target
        self._build_lib = build_lib
        self._open_source_root = f'{TAPPAS_WORKSPACE}/core/open_source'
        self._hailort_cross_compiled_output_dir = FOLDER_NAME / f"build.linux.{self._arch.value}.{self._build_type}"

    def get_libargs_line(self, rootfs_base_path):
        def get_includes(includes):
            include_string = ''
            for inc in includes:
                include_string += "-I{}{},".format(rootfs_base_path, inc)
            return include_string

        return self.LIBARGS_TEMPLATE.format(get_includes(self.INCLUDES))

    def get_meson_build_command(self):
        usr_path = os.path.join(self._toolchain_rootfs_base_path, 'usr')

        rapidjson_root = f'{self._open_source_root}/rapidjson'
        cxxopts_root = f'{self._open_source_root}/cxxopts'

        xtensor_root = f'{self._open_source_root}/xtensor_stack'
        xtensor_blas_root = f'{xtensor_root}/blas'
        xtensor_base_root = f'{xtensor_root}/base'

        required_sources = [xtensor_blas_root, xtensor_base_root, rapidjson_root, cxxopts_root]

        if any(not Path(source).is_dir() for source in required_sources):
            raise FileNotFoundError(f"One or more of the external packages are missing. Please run {TAPPAS_WORKSPACE}/scripts/build_scripts/clone_external_packages.sh")

        build_cmd = ['meson', str(self._output_build_dir), '--buildtype', self._build_type,
                     '-Dlibargs={}'.format(self.get_libargs_line(self._toolchain_rootfs_base_path)),
                     '-Dprefix={}'.format(usr_path),
                     '-Dinclude_blas=false',
                     '-Dtarget_platform={}'.format(self._target_platform),
                     '-Dtarget={}'.format(self._build_lib),
                     '-Dlibxtensor={}'.format(xtensor_base_root),
                     '-Dlibblas={}'.format(xtensor_blas_root),
                     '-Dlibcxxopts={}'.format(cxxopts_root),
                     '-Dlibrapidjson={}'.format(rapidjson_root),
                     '-Dinclude_unit_tests=false']

        return build_cmd


def parse_args():
    parser = argparse.ArgumentParser(description='Cross-compile TAPPAS')
    parser.add_argument('arch', type=Arch, choices=list(Arch), help='Arch to compile to')
    parser.add_argument('target', type=Target, choices=list(Target), help='Target platform to compile to')
    parser.add_argument('build_type', choices=POSSIBLE_BUILD_TYPES, help='Build and compilation type')
    parser.add_argument('toolchain_dir_path', help='Toolchain directory path')
    parser.add_argument('--remote-machine-ip', help='remote machine ip')
    parser.add_argument('--build-lib', help='Build a specific tappas lib target (default all)', choices=POSSIBLE_BUILD_LIBS, default='all')
    parser.add_argument('--clean-build-dir', action='store_true', help='Delete previous build cache (default false)', default=False)
    parser.add_argument('--install-to-rootfs', action='store_true', help='Install to rootfs (default false)', default=False)

    return parser.parse_args()


if __name__ == '__main__':
    args = parse_args()
    logging.basicConfig(stream=sys.stdout, level=logging.INFO)

    install_compilers_apt_packages(args.arch)

    gst_installer = TappasInstaller(arch=args.arch, target=args.target, build_type=args.build_type,
                                    toolchain_dir_path=args.toolchain_dir_path,
                                    build_lib=args.build_lib,
                                    remote_machine_ip=args.remote_machine_ip,
                                    clean_build_dir=args.clean_build_dir,
                                    install_to_rootfs=args.install_to_rootfs)
    gst_installer.build()
