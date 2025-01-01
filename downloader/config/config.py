import platform
from pathlib import Path

VERSION = "v3.31"
MODEL_ZOO_VERSION = "v2.12"

CONFIG_PATH = Path(__file__).parent
DOWNLOADER_PATH = CONFIG_PATH.parent
ROOT_PATH = DOWNLOADER_PATH.parent
REQUIREMENTS_PATH = CONFIG_PATH / "requirements"

BUCKETS_FILE = CONFIG_PATH / "buckets.json"
REQUIREMENTS_FILES = ["general/cascading_networks.json",
                      "general/century.json",
                      "general/depth_estimation.json",
                      "general/detection.json",
                      "general/face_recognition.json",
                      "general/instance_segmentation.json",
                      "general/license_plate_recognition.json",
                      "general/multistream_detection.json",
                      "general/multi_device.json",
                      "general/python.json",
                      "general/re_id.json",
                      "general/tiling.json",
                      "x86/x86_sanity.json",
                      "x86/x86_multistream_detection.json",
                      "x86/x86_century.json",
                      "rockchip/detection.json",
                      "rockchip/license_plate_recognition.json",
                      "rockchip/multistream_detection.json",
                      "rockchip/tiling.json",
                      "rpi/rpi_depth_estimation.json",
                      "rpi/rpi_detection.json",
                      "imx8/imx8_cascading_networks.json",
                      "imx8/imx8_depth_estimation.json",
                      "imx8/imx8_detection.json",
                      "imx8/imx8_license_plate_recognition.json",
                      "imx8/imx8_multistream_detection.json"]

# Used during the download to decide what the source / destination is dynamically
RESERVED_DOWNLOADER_KEYWORDS = {
    '<ARCH>': platform.uname().machine,
}
