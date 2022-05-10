import platform
from pathlib import Path

VERSION = "v3.18"
MODEL_ZOO_VERSION = "v2.0"

CONFIG_PATH = Path(__file__).parent
DOWNLOADER_PATH = CONFIG_PATH.parent
ROOT_PATH = DOWNLOADER_PATH.parent
REQUIREMENTS_PATH = CONFIG_PATH / "requirements"

BUCKETS_FILE = CONFIG_PATH / "buckets.json"
REQUIREMENTS_FILES = ["x86/x86_multistream_detection.json", "x86/x86_detection.json", "x86/x86_segmentation.json", "common/opencv.json",
                      "x86/x86_pose_estimation.json", "x86/x86_face_detection.json",
                      "x86/x86_facial_landmarks.json", "x86/x86_classification.json", "x86/x86_multi_device.json",
                      "x86/x86_native_detection.json",
                      "x86/x86_depth_estimation.json", "x86/x86_multinetworks_parallel.json", "x86/x86_instance_segmentation.json",
                      "x86/x86_tiling.json", "x86/x86_cascading_networks.json", "x86/x86_network_switch.json", "x86/x86_century.json",
                      "x86/x86_license_plate_recognition.json", "x86/x86_python.json", 
                      "rpi/rpi_classification.json",
                      "rpi/rpi_depth_estimation.json", "rpi/rpi_detection.json", "rpi/rpi_face_detection.json","rpi/rpi_multinetworks_parallel.json",
                      "rpi/rpi_pose_estimation.json",
                      "arm/arm_detection.json", "arm/arm_license_plate_recognition.json", "arm/arm_facial_landmarks.json"]

# Used during the download to decide what the source / destination is dynamically
RESERVED_DOWNLOADER_KEYWORDS = {
    '<ARCH>': platform.uname().processor
}
