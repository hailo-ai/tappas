#!/usr/bin/env python
import argparse
import logging
import os
import shutil
from pathlib import Path

import sys

from common import working_directory, ShellRunner, install_compilers_apt_packages, extract_and_install_toolchain, Arch, Target

POSSIBLE_BUILD_TYPES = ['debug', 'release']
POSSIBLE_BUILD_LIBS = ['all', 'apps', 'plugins', 'libs', 'tracers']

TAPPAS_WORKSPACE = Path(os.environ['TAPPAS_WORKSPACE']).resolve()
FOLDER_NAME = Path(__file__).resolve().parent

class GstreamerInstall:
    INCLUDES = [
        '/usr/include/hailort',
        '/usr/include/gst-hailo/metadata',
    ]
    LIBARGS_TEMPLATE = "{}-std=c++17"

    def __init__(self, arch, target, build_type, toolchain_tar_path, yocto_distribution='poky', build_lib='all', remove_cache=False, install_to_rootfs=False):
        self._logger = logging.getLogger(__file__)
        self._arch = arch
        self._target_platform = target
        self._build_type = build_type
        self._build_dir = FOLDER_NAME / f'{self._arch.value}-gsthailotools-build-{self._build_type}'
        self._remove_cache = remove_cache
        self.install_to_rootfs = install_to_rootfs
        self._build_lib = build_lib
        self._runner = ShellRunner()

        self._toolchain_tar_path = Path(toolchain_tar_path).absolute().resolve()
        self._unpacked_toolchain_dir = FOLDER_NAME / f"unpacked-{self._toolchain_tar_path.stem}"
        self._toolchain_rootfs_base_path = self._unpacked_toolchain_dir / "sysroots" / f"{self._arch.value}-{yocto_distribution}-linux"
        self._open_source_root = f'{TAPPAS_WORKSPACE}/core/open_source'
        self._tappas_sources_dir = f'{TAPPAS_WORKSPACE}/core/hailo'

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

    def get_libargs_line(self, rootfs_base_path):
        def get_includes(includes):
            include_string = ''
            for inc in includes:
                include_string += "-I{}{},".format(rootfs_base_path, inc)
            return include_string

        return self.LIBARGS_TEMPLATE.format(get_includes(self.INCLUDES))

    def run_meson_build_command(self, env=None):
        self._logger.info("Running Meson build.")
        with working_directory(self._tappas_sources_dir):
            usr_path = os.path.join(self._toolchain_rootfs_base_path, 'usr')
            if self._build_dir.is_dir() and self._remove_cache:
                shutil.rmtree(self._build_dir)

            rapidjson_root = f'{self._open_source_root}/rapidjson'
            cxxopts_root = f'{self._open_source_root}/cxxopts'

            xtensor_root = f'{self._open_source_root}/xtensor_stack'
            xtensor_blas_root = f'{xtensor_root}/blas'
            xtensor_base_root = f'{xtensor_root}/base'

            # check if xtensor dir exists
            required_sources = [xtensor_blas_root, xtensor_base_root, rapidjson_root, cxxopts_root]

            if any(not Path(source).is_dir() for source in required_sources):
                raise FileNotFoundError(f"One or more of the external packages are missing. Please run {TAPPAS_WORKSPACE}/scripts/build_scripts/clone_external_packages.sh")

            build_cmd = ['meson', self._build_dir, '--buildtype', self._build_type,
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

            self._runner.run(build_cmd, env=env, print_output=True)
            self._logger.info('Done running Meson command')

    def run_ninja_install_command(self, env=None):
        self._logger.info("Running Ninja install command.")

        with working_directory(self._tappas_sources_dir):
            ninja_cmd = ['ninja', 'install', '-C', self._build_dir]
            self._runner.run(ninja_cmd, env, print_output=True)
            self._logger.info('Done running Ninja install')

    def run_ninja_build_command(self, env=None):
        self._logger.info("Running Ninja command.")

        with working_directory(self._tappas_sources_dir):
            ninja_cmd = ['ninja', '-C', self._build_dir]
            self._runner.run(ninja_cmd, env, print_output=True)
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
        if self.install_to_rootfs:
            self.run_ninja_install_command(env)

        self._logger.info(f"Build done. Outputs could be found in {self._build_dir}")


def parse_args():
    parser = argparse.ArgumentParser(description='Cross-compile gst-hailo.')
    parser.add_argument('arch', type=Arch, choices=list(Arch), help='Arch to compile to')
    parser.add_argument('target', type=Target, choices=list(Target), help='Target platform to compile to')
    parser.add_argument('build_type', choices=POSSIBLE_BUILD_TYPES, help='Build and compilation type')
    parser.add_argument('toolchain_tar_path', help='Toolchain TAR path')
    parser.add_argument('--yocto-distribution', help='The name of the Yocto distribution to use (default poky)', default='poky')
    parser.add_argument('--build-lib', help='Build a specific tappas lib target (default all)', choices=POSSIBLE_BUILD_LIBS, default='all')
    parser.add_argument('--remove-cache', action='store_true', help='Delete previous build cache (default false)', default=False)
    parser.add_argument('--install-to-rootfs', action='store_true', help='Install to rootfs (default false)', default=False)

    return parser.parse_args()


if __name__ == '__main__':
    args = parse_args()
    logging.basicConfig(stream=sys.stdout, level=logging.INFO)

    install_compilers_apt_packages(args.arch)

    gst_installer = GstreamerInstall(arch=args.arch, target=args.target, build_type=args.build_type,
                                     toolchain_tar_path=args.toolchain_tar_path,
                                     yocto_distribution=args.yocto_distribution,
                                     build_lib=args.build_lib,
                                     remove_cache=args.remove_cache,
                                     install_to_rootfs=args.install_to_rootfs)
    gst_installer.build()
