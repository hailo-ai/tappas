from contextlib import contextmanager
from pathlib import Path

from colorama import Fore
from requests import get
from tqdm.auto import tqdm

from config import config
from models import Buckets

KNOWN_BUCKETS = Buckets.parse_raw(Path(config.BUCKETS_FILE).read_text())


class S3AmazonDownloader:
    @classmethod
    def dump_requirement(cls, relative_url: str, bucket: str, arch: str, destination_path: Path, requirements_file: Path) -> None:
        url = cls._get_url(relative_url, bucket, arch)
        with requirements_file.open("a") as f:
            f.write(f"{url} -> {destination_path} \n")

    @classmethod
    def _get_url(cls, relative_url: str, bucket: str, arch: str) -> str:
        """
        Construct the URL based on the bucket and architecture.
        """
        if bucket == 'tappas':
            return f"{KNOWN_BUCKETS.buckets[bucket].url}/{config.TAPPAS_VERSION}/{relative_url}"
        elif bucket == 'model_zoo':
            if arch == 'h8':
                return f"{KNOWN_BUCKETS.buckets[bucket].url}/ModelZoo/Compiled/{config.S3_HAILO8_VERSION}/hailo8/{relative_url}"
            elif arch == 'h10':
                return f"{KNOWN_BUCKETS.buckets[bucket].url}/ModelZoo/Compiled/{config.S3_HAILO10_VERSION}/hailo15h/{relative_url}"
            else:
                raise ValueError(f"Unsupported architecture: {arch}")
        else:
            raise ValueError(f"Unsupported bucket: {bucket}")

    @classmethod
    def download(cls, relative_url: str, bucket: str, destination_path: Path, arch: str) -> None:
        """
        Download file in chunks
        """
        url = cls._get_url(relative_url, bucket, arch)
        resp = get(url, allow_redirects=True, stream=True)
        total_bytes = int(resp.headers.get('content-length', 0))

        with destination_path.open('wb') as destination_file:
            with cls._progress_bar(name=destination_path.name, total=total_bytes, unit='B') as progress_bar:
                for chunk in resp.iter_content(chunk_size=4096):
                    destination_file.write(chunk)
                    progress_bar.update(len(chunk))

    @classmethod
    @contextmanager
    def _progress_bar(cls, name: str, total: int, unit: str):
        with tqdm(desc=name, miniters=1, total=total, unit=unit, unit_divisor=1024, unit_scale=True,
                  bar_format="{l_bar}%s{bar}%s{r_bar}" % (Fore.BLUE, Fore.RESET)) as progress_bar:
            yield progress_bar
