import hailo
import numpy as np
import json
from pathlib import Path

import gi

gi.require_version('Gst', '1.0')

from gi.repository import Gst

img_net_path = Path(__file__).parent.resolve(
    strict=True) / "imagenet_labels.json"
img_net_labels = json.loads(img_net_path.read_text())


def top1(array: np.ndarray):
    return np.argsort(-1 * array)[0]


def run(buffer: Gst.Buffer, roi: hailo.HailoROI):
    results = roi.get_tensor("resnet_v1_50/softmax1")
    if results == None:
        return -1
    arr = np.squeeze(np.array(results, copy=False))

    best_index = top1(arr)
    label = img_net_labels[str(best_index)].split(',')[0]
    confidence = round(results.fix_scale(arr[best_index]), 2)

    # add_classification function is in hailo_common.hpp and needs to be binded to python
    top_1_classification = hailo.HailoClassification(
        "imagenet", best_index, label, confidence)
    roi.add_object(top_1_classification)

    return Gst.FlowReturn.OK
