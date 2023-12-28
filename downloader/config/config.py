import platform
from pathlib import Path

VERSION = "v3.27"
MODEL_ZOO_VERSION = "v2.9"

CONFIG_PATH = Path(__file__).parent
DOWNLOADER_PATH = CONFIG_PATH.parent
ROOT_PATH = DOWNLOADER_PATH.parent
REQUIREMENTS_PATH = CONFIG_PATH / "requirements"

BUCKETS_FILE = CONFIG_PATH / "buckets.json"
REQUIREMENTS_FILES = ["general/detection.json",
                      "general/license_plate_recognition.json",
                      "general/multistream_detection.json"]

# Used during the download to decide what the source / destination is dynamically
RESERVED_DOWNLOADER_KEYWORDS = {
    '<ARCH>': platform.uname().machine,
}
