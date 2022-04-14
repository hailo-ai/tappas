import platform
from pathlib import Path

VERSION = "v3.17"
MODEL_ZOO_VERSION = "v2.0"

CONFIG_PATH = Path(__file__).parent
DOWNLOADER_PATH = CONFIG_PATH.parent
ROOT_PATH = DOWNLOADER_PATH.parent
REQUIREMENTS_PATH = CONFIG_PATH / "requirements"

BUCKETS_FILE = CONFIG_PATH / "buckets.json"
REQUIREMENTS_FILES = ["x86_multistream_detection.json", "x86_detection.json", "x86_segmentation.json", "opencv.json",
                      "x86_pose_estimation.json", "x86_face_detection.json",
                      "x86_facial_landmarks.json", "x86_classification.json", "x86_multi_device.json",
                      "arm_detection.json", "native_detection.json",
                      "x86_depth_estimation.json", "x86_multinetworks_parallel.json", "x86_instance_segmentation.json",
                      "x86_tiling.json", "x86_cascading_networks.json", "x86_network_switch.json", "x86_century.json",
                      "x86_license_plate_recognition.json", "x86_python.json",
                      "rpi_classification.json",
                      "rpi_depth_estimation.json","rpi_detection.json", "rpi_face_detection.json","rpi_multinetworks_parallel.json",
                      "rpi_pose_estimation.json"]

# Used during the download to decide what the source / destination is dynamically
RESERVED_DOWNLOADER_KEYWORDS = {
    '<ARCH>': platform.uname().processor
}
