import platform
from pathlib import Path

TAPPAS_VERSION = "v5.0"
HAILO8_VERSION = "v2.16"
HAILO10_VERSION = "v5.0.0"
S3_BUCKET_TAPPAS = 'hailo-tappas'
S3_BUCKET_MODEL_ZOO = 'hailo-model-zoo'
S3_HAILO8_VERSION = "v2.16.0"
S3_HAILO10_VERSION = "v5.0.0"

CONFIG_PATH = Path(__file__).parent
DOWNLOADER_PATH = CONFIG_PATH.parent
ROOT_PATH = DOWNLOADER_PATH.parent
REQUIREMENTS_PATH = CONFIG_PATH / "requirements"

BUCKETS_FILE = CONFIG_PATH / "buckets.json"
REQUIREMENTS_FILE = "detection.json"
ARCH_SUPPORTED = ["h8", "h10"]

# Used during the download to decide what the source / destination is dynamically
RESERVED_DOWNLOADER_KEYWORDS = {
    '<ARCH>': platform.uname().machine,
}
