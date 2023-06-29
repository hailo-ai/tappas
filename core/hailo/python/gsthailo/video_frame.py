#
# Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
# Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
#
#
# SECTION:video-frame
# @title: videoframe
#

from contextlib import contextmanager


import hailo
import platform
import numpy as np

# Important: Keep the order as is
import gi
gi.require_version('Gst', '1.0')
gi.require_version("GstVideo", "1.0")
from gi.repository import Gst, GstVideo


class VideoFrame:
    def __init__(self, buffer: Gst.Buffer, caps: Gst.Caps, roi: hailo.HailoROI):
        self._buffer = buffer
        self._roi = roi
        self._video_info = GstVideo.VideoInfo()
        
        python_version = platform.sys.version_info
        
        if python_version.major != 3:
            raise RuntimeError(f"Python {python_version.major}.{python_version.minor} is not supported")
        
        if python_version.minor < 10:
            self._video_info.from_caps(caps)
        else:
            self._video_info.new_from_caps(caps)

    @property
    def roi(self) -> hailo.HailoROI:
        return self._roi

    @property
    def buffer(self) -> Gst.Buffer:
        return self._buffer

    @property
    def video_info(self) -> GstVideo.VideoInfo:
        return self._video_info

    @staticmethod
    def _video_info_from_caps(caps: Gst.Caps) -> GstVideo.VideoInfo:
        video_info = GstVideo.VideoInfo()
        video_info.from_caps(caps)

        return video_info

    @contextmanager
    def map_buffer(self) -> Gst.MapInfo:
        is_mapping_success, map_info = self._buffer.map(Gst.MapFlags.READ)

        if not is_mapping_success:
            raise RuntimeError("Could not map buffer data")

        try:
            yield map_info
        finally:
            self._buffer.unmap(map_info)

    @classmethod
    def numpy_array_from_buffer(cls, map_info: Gst.MapInfo, caps: Gst.Caps = None, video_info: GstVideo.VideoInfo = None):
        if not caps and not video_info:
            raise RuntimeError("Caps or video_info is must")

        video_info_used = video_info or cls._video_info_from_caps(caps)

        numpy_frame = np.ndarray(shape=(video_info_used.height, video_info_used.width, 3),
                                 dtype=np.uint8,
                                 buffer=map_info.data)

        return numpy_frame
