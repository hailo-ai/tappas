import os
import shutil
import argparse
import fileinput
import subprocess
import sys

from abc import ABC, abstractmethod
from glob import glob
from pathlib import Path
from tempfile import NamedTemporaryFile
from typing import List


ARCH_TO_DPKG = {
    "x86_64": "amd64",
    "aarch64": "arm64",
    "armv7l": "armel",  # Embedded ABI port
    "armv7lhf": "armhf",  # hard float
}


SCRIPTS = [
    "control",
    "preinst",
    "postinst",
    "prerm",
    "postrm",
    "conffiles"
]


class DebFileTarget:
    """
    This class handles copying a single source (file or directory)
    to a single directory in the debian build directory.
     path can be a regex instead of a file name.
    """

    def __init__(self, source, dest, name, blacklist=None, dest_is_filename=False):
        """
        :param source: Path to copy from.
        :type source: str / Path
        :param dest: Path to destination within build directory.
        :type dest: str / Path
        :param name: target's name
        :type name: str
        :param blacklist: List of regexes to exclude when copying directory.
        :type blacklist: list
        """
        self._src = self._normalize_src(Path(source))
        self._dst = Path(dest)
        self._name = name
        self._blacklist = shutil.ignore_patterns(*blacklist) if blacklist else None
        self._dest_is_filename = dest_is_filename

    def _normalize_src(self, source):
        sources = list(glob(str(source)))
        if len(sources) != 1:
            raise FileNotFoundError(f"{source} doesn't match 1 file exactly ({sources})")

        source = Path(sources[0])
        return source

    def copy_target(self, root_dir):
        # In case of symlinks, they will be copied as symlinks!
        dst : Path = root_dir / self._dst
        if self._src.is_dir():
            shutil.copytree(self._src, dst, ignore=self._blacklist, symlinks=True, dirs_exist_ok=True)
        elif self._src.is_file():
            if not dst.exists():
                # If the destination is given as filename, we need to mkdir for its parent, and keep the dest as the final value.
                if self._dest_is_filename:
                    dst.parent.mkdir(exist_ok=True, parents=True)
                else:
                    dst.mkdir(parents=True)
            shutil.copy2(self._src, dst, follow_symlinks=False)
        else:
            raise TypeError(f"{self._src} is not a file / direcotry")


class DebBuilder(ABC):
    def __init__(self, build_path, scripts_path, arch, version, python_version, debug: bool = False):
        self._build_path = build_path
        self._scripts_path = scripts_path
        if arch == "rpi5":
            self._arch = "aarch64"
            self._rpi5 = True
        else:
            self._arch = arch
            self._rpi5 = False
        self._version = version
        self._python_version = python_version.replace(".", "")
        self._debug = debug

    def _generate_debian(self):
        with NamedTemporaryFile(delete=False, prefix=f"{type(self).__name__}-artifact::") as log_file:
            stdout = None if self._debug else log_file
            try:
                # Build package
                subprocess.run(
                    f"dpkg-deb --build --root-owner-group {self._build_path}",
                    check=True,
                    shell=True,
                    stderr=subprocess.STDOUT,
                    stdout=stdout,
                )

                # Rename package
                subprocess.run(
                    f"dpkg-name {self._build_path}.deb --subdir {self._build_path}",
                    check=True,
                    shell=True,
                    stderr=subprocess.STDOUT,
                    stdout=stdout,
                )
            except subprocess.CalledProcessError:
                print(f"subprocess failed. See {log_file.name} for details")
                raise

    def _get_deb_path(self):
        return next(Path(self._build_path).glob(f"*{ARCH_TO_DPKG.get(self._arch, self._arch)}*.deb"))

    def build(self):
        self._generate_build_dir()
        self._generate_debian()
        return self._get_deb_path()

    def _build_deb_dir(self):
        if os.path.exists(self._build_path):
            shutil.rmtree(self._build_path)
        os.makedirs(self._build_path)

    def _copy_control_scripts(self):
        debian_folder_path = os.path.join(self._build_path, "DEBIAN")
        os.makedirs(debian_folder_path)
        self._copy_scripts(debian_folder_path)

    def _generate_build_dir(self):
        self._build_deb_dir()
        self._copy_control_scripts()
        self._copy_project_files()

    def _copy_scripts(self, debian_folder_path):
        for script in SCRIPTS:
            script_file_path = os.path.join(self._scripts_path, script)
            if os.path.exists(script_file_path):
                shutil.copy2(script_file_path, os.path.join(debian_folder_path, script))

        self._edit_control_file(os.path.join(debian_folder_path, "control"))

    def _edit_control_file(self, control_file_path):
        """
        Adds architecture to the control file.
        """
        for line in fileinput.input(control_file_path, inplace=1):
            if self._arch != "all" and line.startswith("Architecture: "):
                line = f"Architecture: {ARCH_TO_DPKG[self._arch]}\n"
            elif line.startswith("Version:"):
                line = f"Version: {self._version}\n"
            elif line.startswith("Package:"):
                # This is done to support RPI's apt server, which does not allow multiple versions of the same package.
                # So we add the version to the package name, and it appears as a different package.
                if self._rpi5:
                    line = line.strip() + f"-{self._version}v\n"
            elif line.startswith("Depends:"):
                if self._rpi5: # Raspberry Pi 5 add gstreamer1.0-libcamera to the dependencies
                    line = line.strip() + f", gstreamer1.0-libcamera\n"
            print(line, end="")

    def _copy_project_files(self):
        for target in self.targets:
            print(f"[{target._name}] :: Copying {target._src}; Destination: /{target._dst}")
            target.copy_target(self._build_path)

    @property
    @abstractmethod
    def targets(self) -> List[DebFileTarget]:
        pass


class TappasDeb(DebBuilder):

    def __init__(self, build_path, scripts_path, arch, version, python_version, debug: bool = False):
        super().__init__(build_path, scripts_path, arch, version, python_version, debug)

    @property
    def targets(self):
        # DO NOT FORGET! These values should be updated in the pkg_config as well!

        lib_targets = [
            "libgsthailometa",
            "libhailo_cv_singleton",
            "libhailo_gst_image",
            "libhailo_tracker",
            "libgstintercept"
        ]

        gstreamer_targets = [
            "libgsthailotools",
            "libgsthailotracers",
            "libgstinstruments"
        ]

        targets = []

        # Regular shared libraries
        for lib in lib_targets:
            lib_dir = f"/usr/lib/{self._arch}-linux-gnu/"
            pattern = os.path.join(lib_dir, f"{lib}.so*")
            for path in glob(pattern):
                targets.append(DebFileTarget(
                    path,
                    f"usr/lib/{self._arch}-linux-gnu/",
                    "tappas_installation_dir"
                ))

        # GStreamer plugins
        for lib in gstreamer_targets:
            lib_dir = f"/usr/lib/{self._arch}-linux-gnu/gstreamer-1.0/"
            pattern = os.path.join(lib_dir, f"{lib}.so*")
            for path in glob(pattern):
                targets.append(DebFileTarget(
                    path,
                    f"usr/lib/{self._arch}-linux-gnu/gstreamer-1.0/",
                    "tappas_installation_dir"
                ))


        # Post-processing libraries
        targets.append(DebFileTarget(
            f"/usr/lib/{self._arch}-linux-gnu/hailo/tappas/",
            f"usr/lib/{self._arch}-linux-gnu/hailo/tappas/",
            "tappas_post_process_libs"
        ))

        # Include headers
        include_paths = [
            ("core/hailo/libs/postprocesses", "tappas_includes"),
            ("core/hailo/general", "tappas_hailo_objects"),
            ("core/hailo/tracking", "tappas_hailo_objects"),
            ("core/hailo/metadata", "tappas_gst_hailo_meta"),
            ("core/hailo/plugins/common", "tappas_gst_hailo_meta"),
        ]
        for rel_path, tag in include_paths:
            targets.append(DebFileTarget(
                f"{os.getcwd()}/{rel_path}",
                "usr/include/hailo/tappas/",
                tag
            ))

        # Package configs
        targets.extend([
            DebFileTarget(
                f"/usr/lib/{self._arch}-linux-gnu/pkgconfig/hailo-tappas-core.pc",
                f"usr/lib/{self._arch}-linux-gnu/pkgconfig/hailo-tappas-core.pc",
                "hailo_tappas_pkg_config",
                dest_is_filename=True
            ),
            DebFileTarget(
                f"/usr/lib/{self._arch}-linux-gnu/pkgconfig/gsthailometa.pc",
                f"usr/lib/{self._arch}-linux-gnu/pkgconfig/",
                "gsthailometa_pkg_config"
            )
        ])

        # Pre-load config
        targets.append(DebFileTarget(
            f"{os.getcwd()}/packaging/deb/hailo_so.conf",
            "etc/ld.so.preload.d/",
            "so_preload"
        ))

        # Open source code
        targets.append(DebFileTarget(
            f"{os.getcwd()}/sources",
            "usr/include/hailo/tappas/sources",
            "hailo_tappas_open_source"
        ))

        # Copyright
        targets.append(DebFileTarget(
            f"{os.getcwd()}/LICENSE",
            f"usr/share/doc/hailo-tappas-core-{self._version}/copyright",
            "copyright",
            dest_is_filename=True
        ))                
        
        if self._python_version:
            targets += [
                # gst_hailo_python
                DebFileTarget(
                    f"/usr/lib/{self._arch}-linux-gnu/gstreamer-1.0/libgsthailopython.so",
                    f"usr/lib/{self._arch}-linux-gnu/gstreamer-1.0/"
                    "gst_hailo_python",
                ),
                # hailo-tappas pybind
                DebFileTarget(
                    f"{os.getcwd()}/core/hailo/build.release/plugins/hailo.cpython-{self._python_version}-{self._arch}-linux-gnu.so",
                    "usr/lib/python3/dist-packages/",
                    "hailo_tappas_pybind",
                ),
                # hailo-tappas python gsthailo
                DebFileTarget(
                    f"{os.getcwd()}/core/hailo/python/gsthailo",
                    "usr/lib/python3/dist-packages/gsthailo",
                    "hailo_tappas_gsthailo",
                ),
            ]
        return targets


def parse_arguments():
    parser = argparse.ArgumentParser()
    parser.add_argument('--tappas-version', required=True, help='Version number. Should look like this: 3.28.0')
    parser.add_argument('--python-version', default="", help='Version number. Should look like this: 3.11.')
    parser.add_argument('--build-dir', default="build", help='Path to the build directory.')
    parser.add_argument('--arch',default="rpi5" ,required=True, help='CPU Architecture. Should be one of: rpi5, x86_64, aarch64, armv7l, armv7lhf')
    parser.add_argument('--debug',action="store_true", help='Print debug information.')
    return parser.parse_args()


def main():
    args = parse_arguments()
    TappasDeb(args.build_dir, "packaging/deb/tappas/control_scripts", args.arch, args.tappas_version, args.python_version, args.debug).build()


if __name__ == '__main__':
    main()
