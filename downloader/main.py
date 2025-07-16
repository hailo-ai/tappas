"""
Model files downloader from S3
"""
import logging
import os
import sys

import boto3
from botocore.exceptions import ClientError

from common import Downloader, parse_downloader_args
from config import config
from s3_amazon_downloader import S3AmazonDownloader


class DownloadException(Exception):
    pass


class S3Downloader(Downloader):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._s3_client = boto3.client('s3', aws_access_key_id='', aws_secret_access_key='')
        self._s3_client._request_signer.sign = (lambda *args, **kwargs: None)

    def _get_md5(self, requirement):
        requirement_key = os.path.join(config.VERSION, requirement.source)

        try:
            s3_file_meta = self._s3_client.head_object(Bucket="hailo-tappas", Key=requirement_key)
            s3_etag = s3_file_meta['ETag'].strip('"')

            return s3_etag
        except ClientError as e:
            self._logger.warning(f'Failed to find meta for {requirement_key}, file may not exist for this version.')
            raise e

    def _dump_requirement(self, requirement, destination):
        self._logger.info(f'S3 - dump only mode: {requirement.source} into {destination} added to requirements txt')
        S3AmazonDownloader.dump_requirement(relative_url=requirement.source, bucket='tappas',
                                            destination_path=destination, requirements_file=self.requirements_dump_file)

    def _download(self, requirement, destination, remote_md5):
        max_retries = 3
        self._logger.info(f'S3 - downloading {requirement.source} into {destination}')

        for current_retry in range(max_retries):
            S3AmazonDownloader.download(relative_url=requirement.source, bucket='tappas',
                                        destination_path=destination)

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
    S3Downloader(root_path=args.root_path, platform=args.platform,
                 dump_requirements=args.dump_requirements,
                 apps_list=args.apps_list).run()


if __name__ == '__main__':
    main()
