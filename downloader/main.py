"""
Model files downloader from S3
"""
import logging
import os
import sys
import hashlib

import boto3
from botocore.exceptions import ClientError

from common import Downloader, parse_downloader_args
from config import config
from s3_amazon_downloader import S3AmazonDownloader


class DownloadException(Exception):
    pass


class S3Downloader(Downloader):
    S3_BUCKET_NAME_MAPPING = {
        'model_zoo': config.S3_BUCKET_MODEL_ZOO,
        'tappas': config.S3_BUCKET_TAPPAS
    }

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._s3_client = boto3.client('s3', aws_access_key_id='', aws_secret_access_key='')
        self._s3_client._request_signer.sign = (lambda *args, **kwargs: None)

    def _get_s3_bucket_name(self, bucket):
        try:
            return self.S3_BUCKET_NAME_MAPPING[bucket]
        except KeyError:
            raise KeyError(f'Unknown bucket: {bucket}')

    def _get_md5(self, requirement, arch):
        if arch == 'h8':
            requirement_key = os.path.join("ModelZoo/Compiled", config.S3_HAILO8_VERSION, "hailo8", requirement.source)
        elif arch == 'h10':
            requirement_key = os.path.join("ModelZoo/Compiled", config.S3_HAILO10_VERSION, "hailo15h", requirement.source)
        elif arch is None:
            requirement_key = os.path.join(config.TAPPAS_VERSION, requirement.source)
        else:
            raise ValueError(f'Unsupported architecture: {arch}')

        try:
            s3_bucket = self._get_s3_bucket_name(requirement.bucket)
            s3_file_meta = self._s3_client.head_object(Bucket=s3_bucket, Key=requirement_key)
            s3_etag = s3_file_meta['ETag'].strip('"')
            if '-' in s3_etag:
                # Handle multipart files on S3 - calculate md5.
                md5_hash = hashlib.md5()
                get_response = self._s3_client.get_object(Bucket=s3_bucket, Key=requirement_key)
                for chunk in get_response['Body'].iter_chunks(chunk_size=8192):
                    md5_hash.update(chunk)
                md5_sum = md5_hash.hexdigest()
                return md5_sum
            else:
                return s3_etag
        except ClientError as e:
            self._logger.warning(f'Failed to find meta for {requirement_key}, file may not exist for this version.')
            raise e

    def _dump_requirement(self, requirement, destination, arch):
        self._logger.info(f'S3 - dump only mode: {requirement.source} into {destination} added to requirements txt')
        S3AmazonDownloader.dump_requirement(relative_url=requirement.source, bucket=requirement.bucket, arch=arch,
                                            destination_path=destination, requirements_file=self.requirements_dump_file)

    def _download(self, requirement, destination, remote_md5, arch):
        max_retries = 3
        self._logger.info(f'S3 - downloading {requirement.source} into {destination}')

        for current_retry in range(max_retries):
            S3AmazonDownloader.download(relative_url=requirement.source, bucket=requirement.bucket,
                                        destination_path=destination, arch=arch)

            local_md5 = self._calculate_md5(destination)
            if local_md5 == remote_md5:
                self._logger.info(f'S3 - downloaded {requirement.source} into {destination} successfully')
                return

            self._logger.warning(f"md5sum - {local_md5} does not match with remote - {remote_md5},"
                                 f" file download corrupted - Try number {current_retry}")

        raise DownloadException(f"md5sum does not match with remote, file download corrupted - After {max_retries}")


def main():
    args = parse_downloader_args()
    logging.basicConfig(stream=sys.stdout, level=logging.INFO)
    S3Downloader(root_path=args.root_path,
                 dump_requirements=args.dump_requirements).run()


if __name__ == '__main__':
    main()
