/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/

#include "mobilenet_ssd.hpp"
#include "hailo_nms_decode.hpp"

static const std::string DEFAULT_SSD_OUTPUT_LAYER = "ssd_mobilenet_v1/nms1";
static const std::string DEFAULT_SSD_MERGED_OUTPUT_LAYER = "ssd_mobilenet_v1_no_alls/nms1";
static const std::string DEFAULT_SSD_VISDRONE_OUTPUT_LAYER = "ssd_mobilenet_v1_visdrone/nms1";


static void mobilenet_ssd_base(HailoROIPtr roi, const std::string output_layer, std::map<uint8_t, std::string> &labels_dict)
{
    if (!roi->has_tensors())
    {
        return;
    }

    auto post = HailoNMSDecode(roi->get_tensor(output_layer), labels_dict);
    auto detections = post.decode<float32_t, common::hailo_bbox_float32_t>();
    hailo_common::add_detections(roi, detections);
}

void mobilenet_ssd(HailoROIPtr roi)
{
    mobilenet_ssd_base(roi, DEFAULT_SSD_OUTPUT_LAYER, common::coco_ninety_classes);
}

void mobilenet_ssd_merged(HailoROIPtr roi)
{
    mobilenet_ssd_base(roi, DEFAULT_SSD_MERGED_OUTPUT_LAYER, common::coco_ninety_classes);
}

void mobilenet_ssd_visdrone(HailoROIPtr roi)
{
    mobilenet_ssd_base(roi, DEFAULT_SSD_VISDRONE_OUTPUT_LAYER, common::coco_visdrone_classes);
}

void filter(HailoROIPtr roi)
{
    mobilenet_ssd(roi);
}
