#!/usr/bin/env python
import argparse
import logging
import os
import shutil
from pathlib import Path

import sys

from common import working_directory, ShellRunner, install_compilers_apt_packages, extract_and_install_toolchain, Arch

POSSIBLE_BUILD_TYPES = ['debug', 'release']

TAPPAS_WORKSPACE = Path(os.environ['TAPPAS_WORKSPACE']).resolve()
FOLDER_NAME = Path(__file__).resolve().parent
GSTREAMER_ROOT = f'{TAPPAS_WORKSPACE}/core/hailo/gstreamer'


class GstreamerInstall:
    INCLUDES = [
        '/usr/include/hailo/',
        '/usr/include/gstreamer-1.0/gst/hailo/',
    ]
    LIBARGS_TEMPLATE = "{}-std=c++17"

    def __init__(self, arch, build_type, toolchain_tar_path, yocto_distribution='poky'):
        self._logger = logging.getLogger(__file__)
        self._arch = arch
        self._build_type = build_type
        self._build_dir = FOLDER_NAME / f'{self._arch.value}-gsthailotools-build-{self._build_type}'
        self._runner = ShellRunner()

        self._toolchain_tar_path = Path(toolchain_tar_path).absolute().resolve()
        self._unpacked_toolchain_dir = FOLDER_NAME / f"unpacked-{self._toolchain_tar_path.stem}"
        self._toolchain_image_path = self._unpacked_toolchain_dir / "sysroots" / f"{self._arch.value}-{yocto_distribution}-linux"

        self._cross_file = FOLDER_NAME / "cross_files" / f"{arch}_cross_file.txt"
        self._hailort_cross_compiled_output_dir = FOLDER_NAME / f"build.linux.{self._arch.value}.{self._build_type}"

        self._initialize_toolchain()

    def _initialize_toolchain(self):
        if (self._unpacked_toolchain_dir / "sysroots").is_dir():
            self._logger.info('Toolchain has been already unpacked and installed successfully. Skipping')
        else:
            self._logger.info('Starting the extraction and install of toolchain')
            extract_and_install_toolchain(tar_path=self._toolchain_tar_path, logger=self._logger,
                                          dir_to_install_toolchain_in=self._unpacked_toolchain_dir)
            self._logger.info('Toolchain ready to use ({})'.format(self._unpacked_toolchain_dir))

    def get_image_user_path(self):
        usr_path = os.path.join(self._toolchain_image_path, 'usr')
        return usr_path

    def get_libargs_line(self):
        def get_includes(includes):
            include_string = ''
            for inc in includes:
                include_string += "-I{},".format(inc)
            return include_string

        return self.LIBARGS_TEMPLATE.format(get_includes(self.INCLUDES))

    def run_meson_build_command(self, env=None):
        self._logger.info("Running Meson build.")
        with working_directory(GSTREAMER_ROOT):
            usr_path = os.path.join(self._toolchain_image_path, 'usr')

            if self._build_dir.is_dir():
                shutil.rmtree(self._build_dir)

            build_cmd = ['meson', self._build_dir, '--buildtype', self._build_type,
                         '-Dlibargs={}'.format(self.get_libargs_line()),
                         '-Dprefix={}'.format(usr_path),
                         '-Dinclude_blas=false',
                         '-Dinclude_network_switch_app=false',
                         '-Dinclude_unit_tests=false',
                         '--cross-file={}'.format(self._cross_file)]

            self._runner.run(build_cmd, env=env)
            self._logger.info('Done running Meson command')

    def run_ninja_build_command(self, env=None):
        self._logger.info("Running Ninja command.")

        with working_directory(GSTREAMER_ROOT):
            ninja_cmd = ['ninja', '-C', self._build_dir]
            self._runner.run(ninja_cmd, env)
            self._logger.info('Done running Ninja command')

    def get_env_variables_from_source_file(self, file_to_source):
        env = dict()

        command = f"env --ignore-environment bash -c 'source {file_to_source} && env'"
        process = self._runner.run(shell_cmd=command, shell=True)

        for line in process.stdout.strip().split('\n'):
            (key, _, value) = line.partition("=")
            env[key] = value

        return env

    def get_custom_environment(self):
        env_setup_file = next(Path(self._unpacked_toolchain_dir).glob('*environment-setup*')).absolute().resolve()
        env_from_environ_setup = self.get_env_variables_from_source_file(env_setup_file)

        return env_from_environ_setup

    def build(self):
        self._logger.info("Building gsthailotools plugins and post processes")
        env = self.get_custom_environment()

        self.run_meson_build_command(env)
        self.run_ninja_build_command(env)

        self._logger.info(f"Build done. Outputs could be found in {self._build_dir}")


def parse_args():
    parser = argparse.ArgumentParser(description='Cross-compile gst-hailo.')
    parser.add_argument('arch', type=Arch, choices=list(Arch), help='Arch to compile to')
    parser.add_argument('build_type', choices=POSSIBLE_BUILD_TYPES, help='Build and compilation type')
    parser.add_argument('toolchain_tar_path', help='Toolchain TAR path')
    parser.add_argument('--yocto-distribution', help='The name of the Yocto distribution to use', default='poky')

    return parser.parse_args()


if __name__ == '__main__':
    args = parse_args()
    logging.basicConfig(stream=sys.stdout, level=logging.INFO)

    install_compilers_apt_packages(args.arch)

    gst_installer = GstreamerInstall(arch=args.arch, build_type=args.build_type,
                                     toolchain_tar_path=args.toolchain_tar_path,
                                     yocto_distribution=args.yocto_distribution)
    gst_installer.build()
