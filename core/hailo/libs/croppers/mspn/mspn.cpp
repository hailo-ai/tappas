/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include <vector>
#include <iostream>
#include "mspn.hpp"

#define PERSON_LABEL "person"

std::vector<HailoROIPtr> create_crops_only_person(std::shared_ptr<HailoMat> image, HailoROIPtr roi)
{
    std::vector<HailoROIPtr> crop_rois;
    // Get all detections.
    std::vector<HailoDetectionPtr> detections_ptrs = hailo_common::get_hailo_detections(roi);
    for (HailoDetectionPtr &detection : detections_ptrs)
    {
        // Modify only detections with "person" label.
        if (std::string(PERSON_LABEL) == detection->get_label())
        {
            crop_rois.emplace_back(detection);
        }
    }
    return crop_rois;
}
