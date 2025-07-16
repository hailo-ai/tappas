#!/usr/bin/env python

"""
Model files uploader to S3
"""
import hashlib
import logging
import os
import shutil
import subprocess
import sys
import threading
from pathlib import Path

import boto3
from boto3.s3.transfer import TransferConfig, S3Transfer
from botocore.exceptions import ClientError

from config import config
from common import Platform
from internal_main import Downloader, FreenasDownloader

# FREENAS and Local Paths
FREENAS_REMOTE_TAPPAS_DIR = Path("releases/tappas")
LOCAL_PACKAGING_TMP_PATH = Path("/tmp/hailo_tappas_packing")
RSYNC_FLAG = "-vr"
DELETE_FLAG = '--delete'
FREENAS_IP = "192.168.12.21"
HAILO_USERNAME = "hailo"
HOST = f'{HAILO_USERNAME}@{FREENAS_IP}'
S3_TAPPAS_BUCKET = 'hailo-tappas'

# Logger Configurations
logging.basicConfig(stream=sys.stdout, level=logging.INFO)
logger = logging.getLogger(__file__)

# S3 Client Configurations
s3_client = boto3.client('s3')

# S3 Transfer Configurations
s3_transfer_config = TransferConfig(multipart_threshold=1024 * 1024 * 1024)  # Set multipart_threshold to 1GB
s3_transfer = S3Transfer(s3_client, s3_transfer_config)


# Uploader Flow Exception
class S3UploadFlowError(Exception):
    pass


class ProgressPercentage:
    """
    https://boto3.amazonaws.com/v1/documentation/api/1.9.42/_modules/boto3/s3/transfer.html
    """
    def __init__(self, filename):
        self._filename = filename
        self._size = float(os.path.getsize(filename))
        self._seen_so_far = 0
        self._lock = threading.Lock()

    def __call__(self, bytes_amount):
        # To simplify we'll assume this is hooked up
        # to a single filename.
        with self._lock:
            self._seen_so_far += bytes_amount
            percentage = (self._seen_so_far / self._size) * 100
            sys.stdout.write(
                "\r%s  %s / %s  (%.2f%%)" % (
                    self._filename, self._seen_so_far, self._size,
                    percentage))
            sys.stdout.flush()


# Download the files to upload to the local machine
def download_from_freenas(source_path, target_path, is_directory=False, symlink=False, should_delete=True):
    source_path = str(source_path)
    target_path = str(target_path)

    if not os.path.isdir(target_path):
        os.makedirs(target_path, exist_ok=True)

    logger.info('Downloading from: {}'.format(source_path))
    cmd = ["rsync"]

    if is_directory:
        cmd.append('-r')
        source_path += '/*'
    if symlink:
        cmd.append('--links')
    cmd.append(RSYNC_FLAG)
    if should_delete:
        cmd.append(DELETE_FLAG)
    cmd.extend(['{}:{}'.format(HOST, source_path), target_path])

    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as e:
        logger.exception(e)
        raise S3UploadFlowError('Failed to rsync with Freenas')
    logger.info('Downloaded to: {}'.format(target_path))


def does_file_exist_in_s3(s3_client, bucket, key, local_md5sum):
    """
    Check if the file already exists in s3
    """
    try:
        # Check if the file exists by getting it's meta
        s3_file_meta = s3_client.head_object(Bucket=bucket, Key=key)
        s3_etag = s3_file_meta['ETag'].strip('"')
        if s3_etag != local_md5sum:  # If the file does exist, check it's etag
            return False
    except ClientError as e:
        if e.response['Error']['Message'] == 'Not Found':
            return False
        logger.exception(e)
        return False
    return True


def md5sum(filename):
    """
    Calculate an md5sum
    :param filename: the filename
    """
    md5 = hashlib.md5()
    with open(filename, 'rb') as f:
        for chunk in iter(lambda: f.read(128 * md5.block_size), b''):
            md5.update(chunk)

    return md5.hexdigest()


def download_model_zoo_requirements(local_tmp_path):
    folders_requirements = Downloader.get_folders_requirements(Platform.ANY)

    for folder_requirement in folders_requirements:
        model_zoo_requirements = filter(lambda req: req.bucket == Downloader.MODEL_ZOO_BUCKET,
                                        folder_requirement.requirements)

        for requirement in model_zoo_requirements:
            network_name = Path(requirement.source).stem
            model_zoo_source = f"{network_name}/{network_name}.hef"

            freenas_download(freenas_relative_path=model_zoo_source,
                             destination=Path(local_tmp_path) / requirement.source)


def freenas_download(freenas_relative_path, destination):
    path = FreenasDownloader.FREENAS_BASE_DIRS[Downloader.MODEL_ZOO_BUCKET] / freenas_relative_path

    download_command = f"rsync -avz -P hailo@{FreenasDownloader.FREENAS_IP}:{path} {destination}"
    subprocess.run(download_command.split(), check=True)


def _get_freenas_path(self, requirement):
    return self.FREENAS_BASE_DIRS[requirement.bucket] / requirement.source


def upload_to_s3(local_tmp_path):
    for root_path, _, files in os.walk(local_tmp_path):
        for file_name in files:
            file_path = os.path.join(root_path, file_name)
            logger.info(f'--------- Checking file: {file_path}')
            file_key = config.VERSION + file_path.partition(config.VERSION)[2]
            file_md5sum = md5sum(file_path)

            if does_file_exist_in_s3(s3_client, bucket=S3_TAPPAS_BUCKET, key=file_key, local_md5sum=file_md5sum):
                logger.info(f'{file_key} already exists inside s3, skipping upload.')
            else:
                logger.info(f'S3 - uploading {file_name} into {file_key}')
                s3_transfer.upload_file(file_path, S3_TAPPAS_BUCKET, file_key,
                                        callback=ProgressPercentage(file_path))


def main():
    logger.info('STARTING UPLOAD FLOW')
    freenas_version_path = FREENAS_REMOTE_TAPPAS_DIR / config.VERSION
    local_tmp_path = LOCAL_PACKAGING_TMP_PATH / config.VERSION
    shutil.rmtree(local_tmp_path, ignore_errors=True)

    logger.info('SYNCING FREENAS FILES')
    download_from_freenas(freenas_version_path, local_tmp_path, is_directory=True)
    download_model_zoo_requirements(local_tmp_path)

    logger.info('UPLOADING FILES TO S3')
    upload_to_s3(local_tmp_path)
    logger.info('FINISHED UPLOADING SUCCESSFULLY')


if __name__ == '__main__':
    main()
