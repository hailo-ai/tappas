import os
import shutil
import subprocess
import configparser
from glob import glob
import re
from pathlib import Path

from build_tool.abstract_sw_component import AbstractSwComponent
from build_tool.logging_config import config_logger
from build_tool.enums.mode import BuildMode
from build_tool.utils.env_utils import set_working_dir
from build_tool.component_installer import NoInstallationRequired


class DocBuildFailure(Exception):
    pass

class FileOrDirectoryNotFound(Exception):
    pass


class TappasDocBuilder(AbstractSwComponent):
    TAPPAS_REPOSITORY_ROOT = os.path.realpath(os.path.join(os.path.dirname(__file__), '../../..'))
    SPHINX_ROOT = os.path.realpath(os.path.dirname(__file__))
    TMP_DOC_FOLDER = os.path.join(TAPPAS_REPOSITORY_ROOT, "tmp_doc")
    TAPPAS_CONFIG_FILE_PATH = os.path.join(SPHINX_ROOT, "config")
    TAPPAS_DOCS_FONTS_PATH = os.path.join(SPHINX_ROOT, "_static/fonts")
    DOC_REQUIRED_PACKAGES = "librsvg2-bin texlive-base latexmk texlive-latex-extra texlive-fonts-extra texlive-xetex xindy"
    FREENAS_SPHINX_FONTS_PATH = "/mnt/v02/sdk/doc/2019-05-12-fonts/*"
    FREENAS_HOST = "192.168.12.21"
    FREENAS_USERNAME = "hailo"

    def __init__(self):
        self.logger = config_logger('Tappas Doc Component')
        name = self.__class__.__name__
        self._release_version = self._get_release_version()
        super().__init__(BuildMode.DEV, self.logger, name)

    def _check_subprocess_output(self, subprocess_output):
        """Copied from file tools/cross_compiler/common.py"""

        cmd = " ".join(subprocess_output.args)
        return_code = subprocess_output.returncode
        subprocess_stdout = subprocess_output.stdout.decode()
        subprocess_stderr = subprocess_output.stderr.decode()

        log_message = f"CMD <{cmd}> RETURNED <{return_code}>.\n"
        if subprocess_stdout:
            log_message += f'{log_message}STDOUT was:\n{subprocess_stdout}\n'
        if subprocess_stderr:
            log_message += f'{log_message}STDERR was:\n{subprocess_stderr}\n'
        if return_code == 0:
            try:
                self.logger.debug(log_message)
            except Exception:
                self.logger.warning("An error occurred when trying to logging the output date")
                self.logger.debug(f"Encoded log message:\n{log_message.encode('utf-8')}")
        else:
            try:
                self.logger.error(f"An error occurred when running a sub-process: {log_message}")
            except Exception:
                self.logger.error("An error occurred when trying to logging the output date")
                self.logger.error(f"Encoded log message:\n{log_message.encode('utf-8')}")
            raise subprocess.CalledProcessError(return_code, cmd, subprocess_stdout, subprocess_stderr)

    def _get_release_version(self):
        content = Path(f"{self.TAPPAS_REPOSITORY_ROOT}/core/hailo/meson.build").read_text()
        tappas_version_raw = next(line for line in content.split('\n') if "version :" in line)
        tappas_version = re.search("([0-9]+.[0-9]+.[0-9]+)", tappas_version_raw).group(1)
        return tappas_version

    def _copy_all_dir_content(self, from_dir, to_dir, ignore_pattern=''):
        if os.path.isdir(from_dir) and os.path.isdir(to_dir):
            for file_name in os.listdir(from_dir):
                file_path = os.path.join(from_dir, file_name)
                self.logger.debug(f"copying {file_path} to {to_dir}")
                if os.path.isdir(file_path):
                    shutil.copytree(file_path, os.path.join(to_dir, file_name), ignore=shutil.ignore_patterns(ignore_pattern))
                else:
                    shutil.copy2(file_path, to_dir)
        else:
            raise FileOrDirectoryNotFound(f"copying from {from_dir} to {to_dir} failed. given directory not found")

    def _build_pdf(self):
        subprocess.run(f"make -C {self.SPHINX_ROOT} latex".split(),
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)

        copy_cmd = f"cp {self.SPHINX_ROOT}/_images_src/medusa.pdf {self.SPHINX_ROOT}/_images_src/logo_small.pdf \
                    {self.SPHINX_ROOT}/_build/latex "

        subprocess.run(copy_cmd.split(),
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)

        subprocess.run(f"make -C {self.SPHINX_ROOT}/_build/latex ".split(), stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, check=True)

        self.logger.info(f"{'Building PDF documentation':<60}[tools/build_tools/tappas_sphinx/_build/latex/tappas_v{self._release_version}_user_guide.pdf]")

    def _update_release_doc_config(self, config_file_path):
        if not os.path.isfile(config_file_path):
            raise FileNotFoundError(f"{config_file_path} does not exist. Copy it from {self.TAPPAS_CONFIG_FILE_PATH}")

        doc_config = configparser.ConfigParser()
        doc_config.read(config_file_path)
        doc_config.set("doc","forced_version",self._release_version)

        with open(config_file_path, 'w') as configfile:
            doc_config.write(configfile)

    def _copy_index_conf_to_tmp(self):
        copy_cmd = f"cp {self.SPHINX_ROOT}/conf.py {self.SPHINX_ROOT}/index.rst \
                    {self.TMP_DOC_FOLDER}"

        subprocess.run(copy_cmd.split(),
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)

    def _prepare_sphinx_environment(self):
        if os.path.isdir(os.path.join(self.SPHINX_ROOT, "_build")):
            shutil.rmtree(os.path.join(self.SPHINX_ROOT, "_build"))

        doc_source_path = os.path.join(self.SPHINX_ROOT, '_source')
        if not os.path.isdir(doc_source_path):
            os.mkdir(doc_source_path)

        shutil.copy2(self.TAPPAS_CONFIG_FILE_PATH, doc_source_path)

        self._update_release_doc_config(os.path.join(doc_source_path, 'config'))

    def _download_path(self, host, username, remote_path, local_path, options=""):
        # downloads a directory using rsync
        rsyc_command = f"rsync -atcPL {options} {username}@{host}:{remote_path} {local_path}"
        rsync_output = subprocess.run(rsyc_command.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self._check_subprocess_output(rsync_output)

    def _download_path_from_freenas(self, remote_path, local_path, options=""):
        self.logger.debug('downloading from freenas ({}) -> ({}) '.format(remote_path, local_path))
        # download to local directory from Freenas server using rsync
        self._download_path(self.FREENAS_HOST, self.FREENAS_USERNAME, remote_path, local_path, options)

    def _download_fonts_freenas(self):
        self.logger.debug(f'Downloading doc fonts from freenas ({self.FREENAS_SPHINX_FONTS_PATH})')

        if not os.path.isdir(self.TAPPAS_DOCS_FONTS_PATH):
            os.mkdir(self.TAPPAS_DOCS_FONTS_PATH)

        self._download_path_from_freenas(
            self.FREENAS_SPHINX_FONTS_PATH,
            self.TAPPAS_DOCS_FONTS_PATH
        )

    def _prepare_pdf_environment(self):
        self._copy_index_conf_to_tmp()
        self.logger.debug("Asserting the required system packages for doc creation")
        dpkg_command = f"dpkg -s {self.DOC_REQUIRED_PACKAGES}"
        dpkg_output = subprocess.run(dpkg_command.split(),
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        # If some of the packages are missing, a nonzero value will return
        if dpkg_output.returncode != 0:
            self.logger.debug("Some of the packages required to install the SDK are missing, installing them")
            apt_install_command = f"sudo apt install -y {self.DOC_REQUIRED_PACKAGES}"
            subprocess.run(apt_install_command.split(),
                                                stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)

        with set_working_dir(os.path.expanduser('~')):
            if not os.path.isdir(".fonts"):
                os.mkdir(".fonts")
            self._copy_all_dir_content(self.TAPPAS_DOCS_FONTS_PATH, ".fonts")

    def _create_tmp_folder(self):
        if os.path.isdir(self.TMP_DOC_FOLDER):
            shutil.rmtree(self.TMP_DOC_FOLDER)

        root_path_length = len(self.TAPPAS_REPOSITORY_ROOT.split('/'))

        glob_ex = f'{self.TAPPAS_REPOSITORY_ROOT}/**/**/*.rst'
        rst_sources = set([result.split('/')[root_path_length] for result in glob(glob_ex, recursive=True) if 'venv' not in result])

        os.mkdir(self.TMP_DOC_FOLDER)

        for item_to_copy in rst_sources:
            copy_item_to_tmp = f"cp -r {self.TAPPAS_REPOSITORY_ROOT}/{item_to_copy} {self.TMP_DOC_FOLDER}"
            subprocess.run(copy_item_to_tmp.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)

        copy_resources = f"cp -r {self.TAPPAS_REPOSITORY_ROOT}/resources {self.TMP_DOC_FOLDER}"
        subprocess.run(copy_resources.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)

        copy_copyright = f"cp -r {self.SPHINX_ROOT}/copyright {self.TMP_DOC_FOLDER}"
        subprocess.run(copy_copyright.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)

        copy_internals = f"cp -r {self.SPHINX_ROOT}/_static {self.SPHINX_ROOT}/_templates {self.TMP_DOC_FOLDER}"
        subprocess.run(copy_internals.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)

        remove_sphinx_from_tmp = os.path.join(self.TMP_DOC_FOLDER, 'tools/build_tools/tappas_sphinx')
        if os.path.isdir(remove_sphinx_from_tmp):
            shutil.rmtree(remove_sphinx_from_tmp)

        self._copy_index_conf_to_tmp()

    def _run_pre_sphinx_tool(self):
        copy_cmd = f"cp {self.SPHINX_ROOT}/pre_sphinx.py {self.TMP_DOC_FOLDER}"

        subprocess.run(copy_cmd.split(),
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)

        exe_tool_cmd = f"{self.TMP_DOC_FOLDER}/pre_sphinx.py"
        pre_sphinx_tool = subprocess.run(exe_tool_cmd.split(),
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self._check_subprocess_output(pre_sphinx_tool)

    def prepare_environment(self):
        self._create_tmp_folder()
        self._prepare_sphinx_environment()
        self._download_fonts_freenas()
        self._prepare_pdf_environment()
        self._run_pre_sphinx_tool()

    def _clean_after_sphinx(self):
        if os.path.isdir(self.TMP_DOC_FOLDER):
            shutil.rmtree(self.TMP_DOC_FOLDER)

    def build(self):
        try:
            self._build_pdf()
        finally:
            self._clean_after_sphinx()

    def install(self):
        raise NoInstallationRequired(f"Installtion for {self.name} has nothing to do.")

    def get_path_to_build_product(self):
        return ""

    def get_artifact_name(self):
        return ""

    @staticmethod
    def get_dependencies():
        return []

