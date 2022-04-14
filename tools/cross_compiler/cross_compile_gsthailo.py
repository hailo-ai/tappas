#!/usr/bin/env python
import argparse
import logging
import os
import re
import shutil
from pathlib import Path

import sys

from common import working_directory, run_subprocess, extract_and_install_toolchain, Arch

POSSIBLE_BUILD_TYPES = ['debug', 'release']

FOLDER_NAME = Path(__file__).resolve().parent
TAPPAS_WORKSPACE = Path(os.environ['TAPPAS_WORKSPACE'])
HAILORT_RELEASE_EXTRACTED_PATH = TAPPAS_WORKSPACE.resolve() / Path("tmp/platform")

PLATFORM_REPOSITORY_ROOT = HAILORT_RELEASE_EXTRACTED_PATH / "hailort/gstreamer"
HAILORT_INCLUDES = HAILORT_RELEASE_EXTRACTED_PATH / "hailort/include"
HAILORT_LIB_TEMPLATE = "hailort/lib/{arch}"
COMPILE_WITH_TOOLCHAIN_SCRIPT = FOLDER_NAME / "scripts" / "compile_gst_with_toolchain.sh"


class HailoRTBuild:
    PERCENT_RE = re.compile(r"^\[ +(?P<percent>\d+)%\].*$")
    MAX_PBAR_VALUE = 100
    CMAKE_TEMPLATE = '{script_path} --compile-from {compile_from} --env-setup-path {env_setup_path} ' \
                     '--build-directory {build_dir} --build-type {build_type} ' \
                     '--hailort-include-dir {hailort_include_dir} ' \
                     '--hailort-lib {hailort_lib} --cmake'
    MAKE_TEMPLATE = '{script_path} --compile-from {compile_from} --env-setup-path {env_setup_path} ' \
                    '--build-directory {build_dir} --make-target {make_target}'

    def __init__(self, arch, build_type, toolchain_tar_path):
        self._logger = logging.getLogger(__file__)
        self._arch = arch
        self._build_type = build_type
        self._toolchain_tar_path = Path(toolchain_tar_path).absolute().resolve()
        self._unpacked_toolchain_dir = FOLDER_NAME / f"unpacked-{self._toolchain_tar_path.stem}"
        self._build_dir = FOLDER_NAME / f'{self._arch.value}-gsthailo-build-{self._build_type}'

        self._initialize_toolchain()

    def _initialize_toolchain(self):
        if (self._unpacked_toolchain_dir / "sysroots").is_dir():
            self._logger.info('Toolchain has been already unpacked and installed successfully. Skipping')
        else:
            self._logger.info('Starting the extraction and install of toolchain')
            extract_and_install_toolchain(tar_path=self._toolchain_tar_path, logger=self._logger,
                                          dir_to_install_toolchain_in=self._unpacked_toolchain_dir)
            self._logger.info('Toolchain ready to use ({})'.format(self._unpacked_toolchain_dir))

    def build(self):
        if os.path.exists(self._build_dir):
            shutil.rmtree(self._build_dir)

        self._build_dir.mkdir(parents=True, exist_ok=True)

        self._logger.info("Compiling gstreamer")
        self._compile_gstreamer_plugins_with_toolchain()

    def _compile_gstreamer_plugins_with_toolchain(self):
        # after yocto toolchain extraction process done, we can enter the toolchain directory
        # that includes 'environment-setup' script and all the root file system of the embedded device
        for env_setup_path in Path(self._unpacked_toolchain_dir).glob('*environment-setup*'):
            self._logger.info('using environment setup found in {}'.format(str(env_setup_path)))
            # setting arguments for compilation - the compile_with_toolchain bash script path,
            # the 'environment-setup' script path and the make_target
            make_target = 'gsthailo'

            self._logger.info('Starting compilation...')
            with working_directory(self._build_dir):
                # running compile_with_toolchain bash script:
                # will source the 'environment-setup' - which sets all the arguments for compilation process
                # and point to the proper paths in the embedded device file system.
                # will call the compilation command (cmake and make) with the make_target
                # (gsthailo in this case), in the requested build directory.
                hailort_libs_folder = HAILORT_RELEASE_EXTRACTED_PATH / HAILORT_LIB_TEMPLATE.format(
                    arch=self._arch.value)
                hailort_lib = next(hailort_libs_folder.glob('libhailort.so.*'))

                cmake_command = self.CMAKE_TEMPLATE.format(script_path=COMPILE_WITH_TOOLCHAIN_SCRIPT,
                                                           compile_from=PLATFORM_REPOSITORY_ROOT,
                                                           env_setup_path=env_setup_path, build_dir=self._build_dir,
                                                           includes=HAILORT_INCLUDES,
                                                           build_type=self._build_type.capitalize(),
                                                           hailort_include_dir=HAILORT_INCLUDES,
                                                           hailort_lib=hailort_lib)
                run_subprocess(cmake_command, logger=self._logger)

                make_command = self.MAKE_TEMPLATE.format(script_path=COMPILE_WITH_TOOLCHAIN_SCRIPT,
                                                         compile_from=self._build_dir, env_setup_path=env_setup_path,
                                                         build_dir=self._build_dir, make_target=make_target)
                run_subprocess(make_command, logger=self._logger)

                self._logger.info('Compilation done')

                return

        # 'environment-setup' script have to exists in the toolchain directory - raise in case it's missing
        raise FileNotFoundError("no env_setup")


def parse_args():
    parser = argparse.ArgumentParser(description='Cross-compile gst-hailo.')
    parser.add_argument('arch', type=Arch, choices=list(Arch), help='Arch to compile to')
    parser.add_argument('build_type', choices=POSSIBLE_BUILD_TYPES, help='Build and compilation type')
    parser.add_argument('toolchain_tar_path', help='Toolchain TAR path')

    return parser.parse_args()


def main():
    args = parse_args()
    logging.basicConfig(stream=sys.stdout, level=logging.INFO)

    hailort_builder = HailoRTBuild(arch=args.arch, build_type=args.build_type,
                                   toolchain_tar_path=args.toolchain_tar_path)
    hailort_builder.build()


if __name__ == '__main__':
    main()
