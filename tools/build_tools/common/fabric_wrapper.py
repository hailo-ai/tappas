import io
import logging
from pathlib import PosixPath

from fabric import Connection as FabricConnection

from common import fabric_extension


class FabricWrapper:
    def __init__(self, host, user, port=None, password=None, key_filename=None, logger=None, gateway=None):
        if password and key_filename:
            raise ValueError("`password` and `key_filename` are mutually exclusive")

        self._host = host
        self._user = user
        self._port = port
        self._password = password
        self._key_filename = key_filename
        self._logger = logger or logging.getLogger('fabric_wrapper')
        self._gateway = gateway

    def get_connection(self, gateway=None, inline_ssh_env=False):
        connect_kwargs = {}

        if self._password:
            connect_kwargs = {'password': self._password}
        elif self._key_filename:
            connect_kwargs = {"key_filename": self._key_filename}

        return FabricConnection(host=self._host, user=self._user, port=self._port,
                                connect_kwargs=connect_kwargs, gateway=gateway or self._gateway,
                                inline_ssh_env=inline_ssh_env)

    def run_command(self, command, env=None, raise_on_error=False, timeout=None, shell=False, gateway=None,
                    work_dir=None):
        """
        Run a command in a subprocess
        :param shell: should run in shell mode
        :param command: The shell command as str
        :param env: environment data as dict
        :param raise_on_error: if True the output of the subprocess would be checked and if failed an
         exception would be raised
        :param timeout: Amount of seconds before the subprocess will be timed out and raise TimeoutExpired exception
        :param gateway: Override the default gateway
        :param work_dir: The remote working_dir
        :return: stdout, stderr, return_code
        """
        inline_ssh_env = env is not None

        if work_dir and fabric_extension.check_if_exists_remote(remote_path=work_dir, remote_connection=self,
                                                                is_dir=True):
            command = f"cd {work_dir} && {command}"

        # inline_ssh_env` - necessary if the remote server has a restricted `AcceptEnv` setting
        # (which is the common default). Please checkout the docs for further details
        # https://docs.fabfile.org/en/2.6/api/connection.html#fabric.connection.Connection.__init__
        with self.get_connection(inline_ssh_env=inline_ssh_env, gateway=gateway) as con:
            p = con.run(command, shell=shell, env=env, timeout=timeout, warn=raise_on_error, pty=True)

        self._log_subprocess(p, env)

        return p.stdout, p.stderr, p.return_code

    def upload(self, local, remote_path, exclude=None, copy_folder_itself=False, mode=None):
        """
        Put a file/dir from wrapped connection’s local to the host filesystem.
        The function would auto-detect if the user intention is to upload a file from an open stream.

        Fabric default func --> upload from open stream
        Fabric Extension rsync --> upload from filesystem

        Mode:specify an exact mode, in the same vein as os.chmod,
        such as an exact octal number (0755) or a string representing one ("0755")
        """
        # Base for all the IO classes https://docs.python.org/3/library/io.html#io.IOBase
        if type(local) is PosixPath:
            local = str(local)
        if type(remote_path) is PosixPath:
            remote_path = str(remote_path)

        use_rsync = not issubclass(type(local), io.IOBase)

        with self.get_connection() as con:
            if use_rsync:
                if copy_folder_itself and not local.endswith('/'):
                    local += '/'

                return fabric_extension.rsync(con, local_path=local, remote_path=remote_path, exclude=exclude,
                                              mode=mode)
            else:
                upload_results = con.put(local=local, remote=remote_path)
                con.run(f'chmod {mode} {remote_path}')

                return upload_results

    def download(self, local, remote_path, copy_symlinks=False):
        """
        Copy a file/multiple files/dir from wrapped connection’s host to the local filesystem.
        The function would auto-detect if the user intention is to copy the file into an open stream.

        Fabric default func --> copy into an open stream
        Fabric Extension rsync --> copy into the filesystem
        :param local: Could be one of three types:
            * subclass of IOBase for a memory download
            * string for downloading a file or directory
            * list(str) for downloading many files/dirs
        :param remote_path: Could be one of two types:
            * string for downloading a file or directory
            * list(str) for downloading many files/dirs
        :param copy_symlinks: Whether symlinks should be downloaded as well
        """
        # Base for all the IO classes https://docs.python.org/3/library/io.html#io.IOBase
        use_rsync = not issubclass(type(local), io.IOBase)

        if type(remote_path) is list:
            remote_path = ' '.join(remote_path)
        if type(local) is list:
            local = ' '.join(local)

        with self.get_connection() as con:
            if use_rsync:
                rsync_flags = ''
                if copy_symlinks:
                    rsync_flags += '--copy-links'

                return fabric_extension.rsync(con, local_path=local, remote_path=remote_path, remote_to_local=True,
                                              rsync_opts=rsync_flags)
            else:
                return con.get(local=local, remote=remote_path)

    def _log_subprocess(self, fabric_results, env):
        out = fabric_results.stdout
        err = fabric_results.stderr
        return_code = fabric_results.return_code
        cmd = fabric_results.command

        log_message = "CMD <{}> RETURNED <{}>.\n".format(cmd, return_code)
        if out:
            log_message += '{}STDOUT was:\n{}\n'.format(log_message, out)
        if err:
            log_message += '{}STDERR was:\n{}\n'.format(log_message, err)

        if return_code == 0:
            self._logger.debug(log_message)
        else:
            self._logger.error("An error occurred when running a sub-process: {}".format(log_message))
