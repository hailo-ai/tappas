/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/

#include "mobilenet_ssd.hpp"
#include "hailo_nms_decode.hpp"

static const std::string DEFAULT_SSD_OUTPUT_LAYER = "ssd_mobilenet_v1/nms1";

void mobilenet_ssd(HailoROIPtr roi)
{
    auto post = HailoNMSDecode(roi->get_tensor(DEFAULT_SSD_OUTPUT_LAYER), common::coco_ninety_classes);
    auto detections = post.decode<uint16_t, common::hailo_bbox_t>();
    hailo_common::add_detections(roi, detections);
}

void mobilenet_ssd_merged(HailoROIPtr roi)
{
    auto post = HailoNMSDecode(roi->get_tensor("ssd_mobilenet_v1_no_alls/nms1"), common::coco_ninety_classes);
    auto detections = post.decode<uint16_t, common::hailo_bbox_t>();
    hailo_common::add_detections(roi, detections);
}

void mobilenet_ssd_visdrone(HailoROIPtr roi)
{
    auto post = HailoNMSDecode(roi->get_tensor("ssd_mobilenet_v1_visdrone/nms1"), common::coco_visdrone_classes);
    auto detections = post.decode<uint16_t, common::hailo_bbox_t>();
    hailo_common::add_detections(roi, detections);
}

void filter(HailoROIPtr roi)
{
    mobilenet_ssd(roi);
}
