import json
from pathlib import Path

import hailo
import numpy as np

# Importing VideoFrame before importing GST is must
from gsthailo import VideoFrame
from gi.repository import Gst

img_net_path = Path(__file__).parent.resolve(strict=True) / "imagenet_labels.json"
img_net_labels = json.loads(img_net_path.read_text())


def top1(array: np.ndarray):
    return np.argsort(-1 * array)[0]


def run(video_frame: VideoFrame):
    results = video_frame.roi.get_tensor("resnet_v1_50/softmax1")
    
    if results is None:
        return Gst.FlowReturn.ERROR

    arr = np.squeeze(np.array(results, copy=False))

    best_index = top1(arr)
    label = img_net_labels[str(best_index)].split(',')[0]
    confidence = round(results.fix_scale(arr[best_index]), 2)

    # add_classification function is in hailo_common.hpp and needs to be binded to python
    top_1_classification = hailo.HailoClassification("imagenet", best_index,
                                                     label, confidence)
    video_frame.roi.add_object(top_1_classification)

    return Gst.FlowReturn.OK
