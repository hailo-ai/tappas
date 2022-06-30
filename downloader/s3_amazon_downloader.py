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
    def dump_requirement(cls, relative_url: str, bucket: str, destination_path: Path, requirements_file: Path) -> None:
        url = f"{KNOWN_BUCKETS.buckets[bucket].url}/{config.VERSION}/{relative_url}"
        with requirements_file.open("a") as f:
            f.write(f"{url} -> {destination_path} \n")

    @classmethod
    def download(cls, relative_url: str, bucket: str, destination_path: Path) -> None:
        """
        Download file in chunks
        """
        url = f"{KNOWN_BUCKETS.buckets[bucket].url}/{config.VERSION}/{relative_url}"

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
