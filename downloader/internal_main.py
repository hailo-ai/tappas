#!/usr/bin/env python3
"""
Model files downloader from freenas - Used internally
"""
import logging
import subprocess
import sys
from pathlib import Path

from common import Downloader, Platform, parse_downloader_args
from config import config


class FreenasDownloader(Downloader):
    FREENAS_IP = "192.168.12.21"

    FREENAS_BASE_DIRS = {
        Downloader.TAPPAS_BUCKET: Path(f"/mnt/v02/sdk/releases/tappas/{config.VERSION}"),
        Downloader.MODEL_ZOO_BUCKET: Path(f"/mnt/v02/sdk/releases/model_zoo/{config.MODEL_ZOO_VERSION}/tmp_hefs/files/")
    }

    @staticmethod
    def get_tappas_stored_hefs():
        """
        Helper function to find out which hefs are stored within TAPPAS storage
        """
        folder_requirements = Downloader.get_folders_requirements(Platform.ANY)
        folder_requirements_by_suffix = Downloader.get_requirements_by_suffix(folder_requirements)
        hefs_requirements = folder_requirements_by_suffix.get('.hef')
        tappas_hefs = [hef_requirement for hef_requirement in hefs_requirements
                       if hef_requirement.bucket == Downloader.TAPPAS_BUCKET]

        return tappas_hefs

    def _get_md5(self, requirement):
        path = self._get_freenas_path(requirement)
        get_md5_command = f'ssh hailo@{self.FREENAS_IP} "md5sum {path}"'
        md5_stdout = subprocess.run(get_md5_command, check=True, shell=True, stdout=subprocess.PIPE).stdout
        md5 = md5_stdout.decode().split()[0]

        return md5

    def _download(self, requirement, destination, remote_md5):
        path = self._get_freenas_path(requirement)

        download_command = f"rsync -avz -P hailo@{self.FREENAS_IP}:{path} {destination}"
        subprocess.run(download_command.split(), check=True)

    def _get_freenas_path(self, requirement):
        return self.FREENAS_BASE_DIRS[requirement.bucket] / requirement.source

    def _requirements_manipulation(self, requirements):
        self._adjust_model_zoo_requirements(requirements)


def main():
    args = parse_downloader_args()
    logging.basicConfig(stream=sys.stdout, level=logging.INFO)
    FreenasDownloader(root_path=args.root_path, platform=args.platform,
                      dump_requirements=args.dump_requirements,
                      apps_list=args.apps_list).run()


if __name__ == '__main__':
    main()
