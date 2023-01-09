/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once
#include <gst/video/video.h>
#include "hailo_objects.hpp"
#include "hailo_common.hpp"

__BEGIN_DECLS
void arcface_rgb(HailoROIPtr roi, GstVideoFrame *frame, gchar *current_stream_id);
void arcface_rgba(HailoROIPtr roi, GstVideoFrame *frame, gchar *current_stream_id);
void filter(HailoROIPtr roi, GstVideoFrame *frame, gchar *current_stream_id);
__END_DECLS