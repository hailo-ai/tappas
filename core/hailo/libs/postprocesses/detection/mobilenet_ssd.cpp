/**
 * Copyright (c) 2021-2026 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/

#include "mobilenet_ssd.hpp"
#include "hailo_nms_decode.hpp"

static const std::string DEFAULT_SSD_OUTPUT_LAYER = "ssd_mobilenet_v1/nms1";
static const std::string DEFAULT_SSD_H10_OUTPUT_LAYER = "ssd_mobilenet_v1/ssd_nms_postprocess";
static const std::string DEFAULT_SSD_MERGED_OUTPUT_LAYER = "ssd_mobilenet_v1_no_alls/nms1";
static const std::string DEFAULT_SSD_VISDRONE_OUTPUT_LAYER = "ssd_mobilenet_v1_visdrone/nms1";


void mobilenet_ssd(HailoROIPtr roi)
{
    nms_decode_to_roi(roi, DEFAULT_SSD_OUTPUT_LAYER, common::coco_ninety_classes);
}

void mobilenet_ssd_h10(HailoROIPtr roi)
{
    nms_decode_to_roi(roi, DEFAULT_SSD_H10_OUTPUT_LAYER, common::coco_ninety_classes);
}

void mobilenet_ssd_merged(HailoROIPtr roi)
{
    nms_decode_to_roi(roi, DEFAULT_SSD_MERGED_OUTPUT_LAYER, common::coco_ninety_classes);
}

void mobilenet_ssd_visdrone(HailoROIPtr roi)
{
    nms_decode_to_roi(roi, DEFAULT_SSD_VISDRONE_OUTPUT_LAYER, common::coco_visdrone_classes);
}

void filter(HailoROIPtr roi)
{
    mobilenet_ssd(roi);
}
