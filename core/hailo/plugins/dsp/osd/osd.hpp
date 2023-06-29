/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once

#include <gst/video/video.h>
#include <map>
#include <vector>
#include "hailo_objects.hpp"
#include "osd_utils.hpp"
#include "dsp/gsthailodsp.h"

typedef enum
{
    OSD_STATUS_UNINITIALIZED = -1,
    OSD_STATUS_OK,
} osd_status_t;

class OsdParams
{
public:
    std::vector<osd::staticText> static_texts;
    std::vector<osd::staticImage> static_images;
    std::vector<osd::dateTime> date_times;

    OsdParams(std::vector<osd::staticText> static_texts,
              std::vector<osd::staticImage> static_images,
              std::vector<osd::dateTime> date_times) {
        this->static_texts = static_texts;
        this->static_images = static_images;
        this->date_times = date_times;
    }
};

__BEGIN_DECLS
OsdParams *load_json_config(const std::string config_path);
void free_param_resources(OsdParams *params_ptr);
osd_status_t initialize_overlay_images(OsdParams *params, int full_image_width, int full_image_height);
osd_status_t blend_all(GstVideoFrame *video_frame, OsdParams *params);
__END_DECLS