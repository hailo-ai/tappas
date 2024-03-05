import contextlib
import json
import logging
import os
import shlex
import shutil
import subprocess
import sys
import time
from abc import ABC, abstractmethod
from enum import Enum
from pathlib import Path
from typing import List


FOLDER_NAME = Path(__file__).resolve().parent


class Arch(Enum):
    AARCH64 = 'aarch64'
    ARMV7L = 'armv7l'
    ARMV7LHF = 'armv7lhf'
    ARMV8A = 'armv8a'

    def __str__(self):
        return self.value


class Target(Enum):
    IMX6 = 'imx6'
    IMX8 = 'imx8'
    HAILO15 = 'hailo15'

    def __str__(self):
        return self.value


def find_symlinks(target_file: Path) -> List[Path]:
    symlink_list = []

    directory = target_file.parent

    for path in directory.rglob('*'):
        if path.is_symlink() and path.resolve() == target_file.resolve():
            symlink_list.append(path.relative_to(directory))

    return symlink_list


@contextlib.contextmanager
def working_directory(target_directory):
    current_directory = os.getcwd()
    try:
        os.chdir(target_directory)
        yield target_directory
    finally:
        os.chdir(current_directory)


def progress_bar(it, prefix="", size=60, out=sys.stdout):
    # Taken from: https://stackoverflow.com/a/34482761
    count = len(it)
    start = time.time()

    def show(j):
        x = int(size*j/count)
        remaining = ((time.time() - start) / j) * (count - j)

        mins, sec = divmod(remaining, 60)
        time_str = f"{int(mins):02}:{sec:05.2f}"

        print(f"{prefix}[{u'█'*x}{('.'*(size-x))}] {j}/{count} Est wait {time_str}", end='\r', file=out, flush=True)

    for i, item in enumerate(it):
        yield item
        show(i+1)
    print("\n", flush=True, file=out)


class ShellRunner:
    """Object dedicated for running and logging shell commands."""

    def __init__(self, logger=None):
        self._logger = logger or logging.getLogger('shell_runner')

    def run(self, shell_cmd, env=None, ignore_errors=False, timeout=None, shell=False, cwd=None, print_output=False):
        """
        Run a command in a subprocess
        :param shell: should run in shell mode? PAY ATTENTION: if shell=True pass the command as string and not
         as array of strings
        :param shell_cmd: The shell command as list of string --> f.e ['python', '-c', ...]
        :param env: environment data as dict
        :param ignore_errors: if True the output of the subprocess would be checked and if failed an
         exception would be raised
        :param timeout: Amount of seconds before the subprocess will be timed out and raise TimeoutExpired exception
        :param cwd: directory to run from
        :param print_output: print output at runtime
        :return: stdout, stderr, return_code
        """
        if type(shell_cmd) is list:
            shell_cmd = self._convert_pathlib_instance_to_str(shell_cmd)

        output_method = None if print_output else subprocess.PIPE

        p = subprocess.run(shell_cmd, cwd=cwd, shell=shell, env=env, timeout=timeout,
                           stdout=output_method, stderr=output_method, stdin=output_method)

        if not print_output:
            p.stdout = p.stdout.decode()
            p.stderr = p.stderr.decode()
            self._log_subprocess(p)

        if not ignore_errors:
            p.check_returncode()

        return p

    def _convert_pathlib_instance_to_str(self, arr):
        """
        Provide support to auto convert Path-lib to str
        """
        shell_cmd = [str(word) if type(word) is Path else word
                     for word in arr]
        return shell_cmd

    def _log_subprocess(self, subprocess_results):
        out = subprocess_results.stdout
        err = subprocess_results.stderr
        return_code = subprocess_results.returncode
        cmd = " ".join([str(w) for w in subprocess_results.args])

        log_message = "CMD <{}> RETURNED <{}>.\n".format(cmd, return_code)
        if out:
            log_message = '{}STDOUT was:\n{}\n'.format(log_message, out)
        if err:
            log_message = '{}STDERR was:\n{}\n'.format(log_message, err)

        if return_code == 0:
            self._logger.debug(log_message)
        else:
            self._logger.error("An error occurred when running a sub-process: {}".format(log_message))


def install_compilers_apt_packages(arch):
    runner = ShellRunner()

    if arch == Arch.ARMV7L:
        apt_packages = ["g++-arm-linux-gnueabi", "gcc-arm-linux-gnueabi"]
    elif arch == Arch.ARMV7LHF:
        apt_packages = ["g++-arm-linux-gnueabihf", "gcc-arm-linux-gnueabihf"]
    elif arch == Arch.ARMV8A:
        apt_packages = ["g++-aarch64-linux-gnu", "gcc-aarch64-linux-gnu"]
    else:
        apt_packages = [f'g++-{arch.value}-linux-gnu', f'gcc-{arch.value}-linux-gnu']

    runner.run(shell_cmd=f'sudo apt-get install -y {" ".join(apt_packages)}', shell=True)


class MesonInstaller(ABC):
    def __init__(self, arch, build_type, toolchain_dir_path, src_build_dir, remote_machine_ip=None, clean_build_dir=False,
                 install_to_toolchain_rootfs=False):
        self._arch = arch
        self._build_type = build_type
        self._clean_build_dir = clean_build_dir
        self._install_to_toolchain_rootfs = install_to_toolchain_rootfs
        self._remote_machine_ip = remote_machine_ip
        self._deploy_to_remote_machine = self._remote_machine_ip is not None
        self._src_build_dir = src_build_dir

        self._logger = logging.getLogger(__file__)
        self._runner = ShellRunner()

        self._output_build_dir = FOLDER_NAME / f'{self._arch.value}-gsthailotools-build-{self._build_type}'
        self._toolchain_dir_path = Path(toolchain_dir_path).absolute().resolve()
        self._toolchain_rootfs_base_path = self._toolchain_dir_path / "sysroots" / f"{self._arch.value}-poky-linux"

        self._install_toolchain()

    @abstractmethod
    def get_meson_build_command(self):
        pass

    def _install_toolchain(self):
        if (self._toolchain_dir_path / "sysroots").is_dir():
            self._logger.info('Toolchain has been already unpacked and installed successfully. Skipping')
            return

        self._logger.info('Starting the installation of the toolchain')

        toolchain_installers = [self._toolchain_dir_path / Path(member.name)
                                for member in self._toolchain_dir_path.glob('*.sh')]

        for toolchain_installer in toolchain_installers:
            self._logger.info("installing {}".format(toolchain_installer))

            if "LD_LIBRARY_PATH" in os.environ:
                raise EnvironmentError("LD_LIBRARY_PATH is set, The SDK will not operate correctly, exiting")

            install_toolchain_command = f"{toolchain_installer} -d {self._toolchain_dir_path} -y"
            self._runner.run(shell_cmd=install_toolchain_command, shell=True)

        self._logger.info('Toolchain ready to use ({})'.format(self._toolchain_dir_path))

    def run_meson_build_command(self, env=None):
        self._logger.info("Running Meson build.")

        build_cmd = self.get_meson_build_command()

        self._runner.run(build_cmd, env=env, print_output=True)
        self._logger.info('Done running Meson command')

    def run_ninja_install_command(self, env=None):
        self._logger.info("Running Ninja install command.")
        env["DESTDIR"] = self._toolchain_rootfs_base_path

        ninja_cmd = ['ninja', 'install', '-C', self._output_build_dir]
        self._runner.run(ninja_cmd, env, print_output=True)
        self._logger.info('Done running Ninja install')

    def run_ninja_build_command(self, env=None):
        self._logger.info("Running Ninja command.")

        ninja_cmd = ['ninja', '-C', self._output_build_dir]
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

    def deploy_artifacts_to_remote_machine(self):
        meson_path_cmd = "env -i which meson"
        meson_path = self._runner.run(shell_cmd=meson_path_cmd, shell=True).stdout.strip()

        meson_introspect_cmd = f"{meson_path} introspect {self._output_build_dir} --installed"
        meson_introspect_output = self._runner.run(shell_cmd=meson_introspect_cmd, shell=True).stdout

        files_and_dest_paths = json.loads(meson_introspect_output)
        amount_of_files_range = range(len(files_and_dest_paths.keys()))
        progress_bar_generator = progress_bar(amount_of_files_range, "Progress: ")

        for key, value in files_and_dest_paths.items():
            if not Path(key).is_file():
                # Introspect returns some non-existing files
                pass
            else:
                destination = f"root@{self._remote_machine_ip}:{value}"
                rsync_cmd = f"rsync -avz --update --progress {shlex.quote(key)} {shlex.quote(destination)}"
                self._runner.run(rsync_cmd, shell=True)

                # We need this section because meson introspect is buggy and returns wrong paths for symlinks
                # Maybe once we would drop kirkstone, this one would be solved
                for symlink_file in find_symlinks(Path(key)):
                    symlink_destination = f"root@{self._remote_machine_ip}:{Path(value).parent / symlink_file}"
                    symlink_rsync_cmd = f"rsync -avz --update --progress {Path(key).parent / symlink_file} {symlink_destination}"
                    self._runner.run(symlink_rsync_cmd, shell=True)

            next(progress_bar_generator)

    def get_custom_environment(self):
        env_setup_file = next(Path(self._toolchain_dir_path).glob('*environment-setup*')).absolute().resolve()
        env_from_environ_setup = self.get_env_variables_from_source_file(env_setup_file)

        return env_from_environ_setup

    def build(self):
        self._logger.info("Building gsthailotools plugins and post processes")
        env = self.get_custom_environment()

        src_dir_name = self._src_build_dir.parts[-1]
        self._output_build_dir = self._output_build_dir / src_dir_name
        print(f"Build dir: {self._output_build_dir}")

        with working_directory(self._src_build_dir):
            if self._output_build_dir.is_dir() and self._clean_build_dir:
                shutil.rmtree(self._output_build_dir)

            self.run_meson_build_command(env)
            self.run_ninja_build_command(env)

            if self._install_to_toolchain_rootfs:
                self.run_ninja_install_command(env)
            if self._deploy_to_remote_machine:
                self.deploy_artifacts_to_remote_machine()

            self._output_build_dir = self._output_build_dir.parent

        self._logger.info(f"Build done. Outputs could be found in {self._output_build_dir}")
