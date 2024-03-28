import argparse
import hashlib
import logging
import os
import subprocess
import tarfile
from abc import ABC, abstractmethod
from enum import Enum
from pathlib import Path

from config import config
from models import FolderRequirements


class Platform(Enum):
    X86 = 'x86'
    ARM = 'arm'
    IMX8 = 'imx8'
    Rockchip = 'rockchip'
    RaspberryPI = 'rpi'
    
    ANY = 'any'

    def __str__(self):
        return self.value


class Downloader(ABC):
    COMMON_SUFFIX = ['.hef', '.mp4']
    COMMON_RESOURCES_PATH = Path(__file__).parent.parent.resolve() / Path("apps/h8/gstreamer/resources")
    TAPPAS_BUCKET = 'tappas'
    MODEL_ZOO_BUCKET = 'model_zoo'
    DEFAULT_APP_REQUIREMENT_FILE = "download_requirements.txt"

    def __init__(self, root_path=None, platform=Platform.X86, dump_requirements=False, apps_list=None):
        self._md5_cache_dict = dict()
        self._logger = logging.getLogger(__file__)
        self._root_path = root_path or config.ROOT_PATH
        self.dump_requirements = dump_requirements
        self.requirements_dump_file = Path(self.DEFAULT_APP_REQUIREMENT_FILE)
        self.platform = platform
        self.apps_list = apps_list or []

        self._logger.info(f'Initialized with platform - {platform}')

    @abstractmethod
    def _download(self, requirement, destination, remote_md5):
        pass

    @abstractmethod
    def _get_md5(self, path):
        pass

    def _dump_requirement(self, requirement, destination):
        pass

    @staticmethod
    def get_folders_requirements(platform):
        requirements = []

        for requirements_file in config.REQUIREMENTS_FILES:
            req_path = config.REQUIREMENTS_PATH / requirements_file
            is_general_req = str(requirements_file).startswith(f'general/') and platform not in [Platform.IMX8, Platform.Rockchip]
            is_platform_req = str(requirements_file).startswith(f'{str(platform)}/')
            if platform == Platform.ANY or is_platform_req or is_general_req:
                requirements_file_content = req_path.read_text()
                requirements.append(FolderRequirements.parse_raw(requirements_file_content))

        return requirements

    @staticmethod
    def get_requirements_by_suffix(folder_requirements):
        requirements_by_suffix_dict = dict()

        for folder_requirement in folder_requirements:
            for requirement in folder_requirement.requirements:
                requirement_source = Path(requirement.source)
                requirement_suffix = requirement_source.suffix

                if requirement_suffix not in Downloader.COMMON_SUFFIX:
                    continue

                if requirement_suffix not in requirements_by_suffix_dict:
                    requirements_by_suffix_dict[requirement_suffix] = set()

                requirements_by_suffix_dict[requirement_suffix].add(requirement)

        return requirements_by_suffix_dict
    
    def _requirements_manipulation(self, requirements):
        pass

    @staticmethod
    def get_apps_requirements(apps_files):
        apps_requirements = []
        for requirements_file in apps_files:
            req_path = config.REQUIREMENTS_PATH / requirements_file
            if requirements_file in config.REQUIREMENTS_FILES:
                requirements_file_content = req_path.read_text()
                apps_requirements.append(FolderRequirements.parse_raw(requirements_file_content))
        return apps_requirements

    def _translate_apps_to_files(self, apps_list):
        apps_files = []
        for app in apps_list:
            arch, app_name = app.split('/')
            if arch.startswith('general'):
                app_file = f'{arch}/{app_name}.json'
            else:
                app_file = f'{arch}/{arch}_{app_name}.json'
            apps_files.append(app_file)
        return apps_files

    def run(self, init_common_dir=True):
        if self.apps_list:
            requirements = self.get_apps_requirements(self._translate_apps_to_files(self.apps_list))
        else:
            requirements = self.get_folders_requirements(self.platform)

        self._requirements_manipulation(requirements)

        if self.dump_requirements:
            if self.requirements_dump_file.exists():
                self.requirements_dump_file.open('w')

            # Dump mode only - dump requirements into text file
            for requirement in requirements:
                self._dump_folder_requirements(requirement)
        else:
            # Default download mnode
            if init_common_dir:
                self._download_common_resources(requirements)

            for requirement in requirements:
                self._download_folder_requirements(requirement)

    def _extract_tar(self, tar_path, extract_to):
        with tarfile.open(tar_path, "r:gz") as tar_file:
            tar_file.extractall(path=extract_to)

    def _dump_folder_requirements(self, folder_requirements: FolderRequirements):
        for requirement in folder_requirements.requirements:
            self._dump_requirement(requirement=requirement, destination=folder_requirements.path)

    def _download_folder_requirements(self, folder_requirements: FolderRequirements):
        project_dir = self._root_path / folder_requirements.path

        for requirement in folder_requirements.requirements:
            destination_path = project_dir / requirement.destination
            destination_path.parent.mkdir(parents=True, exist_ok=True)

            self._download_file(destination_path, requirement)

    def _calculate_md5(self, file_path):
        md5 = hashlib.md5(file_path.read_bytes())
        md5 = md5.hexdigest()

        return md5

    def _update_md5_cache(self, file_path):
        md5 = self._calculate_md5(file_path)
        self._md5_cache_dict[md5] = file_path

    def _download_file(self, destination_path, requirement):
        md5 = self._get_md5(requirement)
        if destination_path.is_file():
            local_file_md5 = hashlib.md5(destination_path.read_bytes()).hexdigest()
            if md5 == local_file_md5:
                self._logger.info(
                    f'{destination_path.name} already exists inside {destination_path.parent}. Skipping download')
                self._update_md5_cache(destination_path)
                return
            else:
                destination_path.unlink()

        if md5 in self._md5_cache_dict:
            self._logger.info(f'Creating softlink {self._md5_cache_dict[md5]} to {destination_path}')
            create_symlink_command = f"ln -s {self._md5_cache_dict[md5]} {destination_path}"
            subprocess.run(create_symlink_command.split(), check=True)
        else:
            self._logger.info(f'Downloading {requirement.source} into {destination_path}')
            self._download(requirement=requirement, destination=destination_path, remote_md5=md5)
            self._update_md5_cache(destination_path)

            if requirement.should_extract:
                self._logger.info(f'Extracting {destination_path} to folder')
                self._extract_tar(destination_path, destination_path.parent)
                os.remove(destination_path)

    def _adjust_model_zoo_requirements(self, folders_requirements):
        """
        Until the ADK will upload their HEF's to the model-zoo, we have to copy them manually.
        This function adjust the hef paths
        """
        for folder_requirement in folders_requirements:
            model_zoo_requirements = filter(lambda req: req.bucket == self.MODEL_ZOO_BUCKET,
                                            folder_requirement.requirements)

            for requirement in model_zoo_requirements:
                network_name = Path(requirement.source).stem
                requirement.source = f"{network_name}/{network_name}.hef"

    def _download_common_resources(self, requirements):
        requirements_by_suffix_dict = self.get_requirements_by_suffix(requirements)

        for suffix, requirements in requirements_by_suffix_dict.items():
            # The first char stores a dot, for example ".hef" --> "hef"
            current_suffix_resources_dir = self.COMMON_RESOURCES_PATH / suffix[1:]
            current_suffix_resources_dir.mkdir(parents=True, exist_ok=True)

            for requirement in requirements:
                destination_path = current_suffix_resources_dir / Path(requirement.destination).name
                self._download_file(destination_path=destination_path, requirement=requirement)


def parse_downloader_args():
    def dir_path_parser(path_to_dir):
        if not Path(path_to_dir).is_dir():
            return ValueError(f'{path_to_dir} - is not a path to a directory')

        return Path(path_to_dir)

    parser = argparse.ArgumentParser(description='Downloader tool')
    parser.add_argument('platform', type=Platform, choices=list(Platform), default=Platform.X86.value,
                        help='Build and compilation type')
    parser.add_argument('--root-path', type=dir_path_parser,
                        help='Root path to the TAPPAS directory - Default is the TAPPAS dir')
    parser.add_argument('--dump-requirements', action='store_true', default=False,
                        help='Dump mode only - dump apps requirements into text file into download_requirements.txt'
                             ' (includes hef files and media)')
    parser.add_argument('--apps-list', nargs='+', type=str,
                        help='Specific APPs to set up')
    return parser.parse_args()
