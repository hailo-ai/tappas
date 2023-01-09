import platform
from pathlib import Path

VERSION = "v3.23"
MODEL_ZOO_VERSION = "v2.4"

CONFIG_PATH = Path(__file__).parent
DOWNLOADER_PATH = CONFIG_PATH.parent
ROOT_PATH = DOWNLOADER_PATH.parent
REQUIREMENTS_PATH = CONFIG_PATH / "requirements"

BUCKETS_FILE = CONFIG_PATH / "buckets.json"
REQUIREMENTS_FILES = ["general/cascading_networks.json",
                      "general/century.json",
                      "general/classification.json",
                      "general/depth_estimation.json",
                      "general/detection.json",
                      "general/face_detection.json",
                      "general/face_recognition.json",
                      "general/facial_landmarks.json",
                      "general/instance_segmentation.json",
                      "general/license_plate_recognition.json",
                      "general/multinetworks_parallel.json",
                      "general/multistream_detection.json",
                      "general/multi_device.json",
                      "general/native_detection.json",
                      "general/network_switch.json",
                      "general/pose_estimation.json",
                      "general/python.json",
                      "general/re_id.json",
                      "general/segmentation.json",
                      "general/tiling.json",
                      "x86/x86_sanity.json",
                      "x86/x86_vms.json",
                      "x86/x86_multistream_detection.json",
                      "x86/x86_century.json",
                      "rpi/rpi_cascading_networks.json",
                      "rpi/rpi_classification.json",
                      "rpi/rpi_depth_estimation.json",
                      "rpi/rpi_detection.json",
                      "rpi/rpi_face_detection.json",
                      "rpi/rpi_multinetworks_parallel.json",
                      "rpi/rpi_pose_estimation.json",
                      "imx6/imx6_classification.json",
                      "imx6/imx6_depth_estimation.json",
                      "imx6/imx6_detection.json",
                      "imx8/imx8_cascading_networks.json",
                      "imx8/imx8_depth_estimation.json",
                      "imx8/imx8_detection.json",
                      "imx8/imx8_facial_landmarks.json",
                      "imx8/imx8_license_plate_recognition.json",
                      "imx8/imx8_pose_estimation.json",
                      "imx8/imx8_segmentation.json"]

# Used during the download to decide what the source / destination is dynamically
RESERVED_DOWNLOADER_KEYWORDS = {
    '<ARCH>': platform.uname().machine,
}
