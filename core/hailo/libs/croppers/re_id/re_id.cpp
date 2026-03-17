/**
 * Copyright (c) 2021-2026 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include <vector>
#include <iostream>
#include "re_id.hpp"
#include "opencv_utils.hpp"

#define PERSON_LABEL "person"
#define MIN_RATIO (1.7f)
#define MAX_RATIO (4.5f)
#define MIN_HEIGHT (0.3f)
#define MAX_HEIGHT (0.8f)
#define MIN_X (0.05f)
#define MAX_X (0.95f)
#define TRACK_DELAY (5)
#define MIN_QUALITY (400)
#define RE_ID_NETWORK_WIDTH 128
#define RE_ID_NETWORK_HEIGHT 256

std::map<int, int> track_counter;

HailoUniqueIDPtr get_tracking_id(HailoDetectionPtr detection)
{
    for (auto obj : detection->get_objects_typed(HAILO_UNIQUE_ID))
    {
        HailoUniqueIDPtr id = std::dynamic_pointer_cast<HailoUniqueID>(obj);
        if (id->get_mode() == TRACKING_ID)
        {
            return id;
        }
    }
    return nullptr;
}

/**
 * @brief Returns a vector of HailoROIPtr to crop and resize.
 *
 * @param image The original picture (std::shared_ptr<HailoMat>)
 * @param roi The main ROI of this picture.
 * @return std::vector<HailoROIPtr> vector of ROI's to crop and resize.
 */
std::vector<HailoROIPtr> create_crops(std::shared_ptr<HailoMat> image, HailoROIPtr roi)
{
    std::vector<HailoROIPtr> crop_rois;
    // Get all detections.
    std::vector<HailoDetectionPtr> detections_ptrs = hailo_common::get_hailo_detections(roi);
    for (HailoDetectionPtr &detection : detections_ptrs)
    {
        // Modify only detections with "person" label.
        if (std::string(PERSON_LABEL) == detection->get_label())
        {
            // Remove previous matrices
            roi->remove_objects_typed(HAILO_MATRIX);

            int tracking_id = get_tracking_id(detection)->get_id();

            auto counter = track_counter.find(tracking_id);
            if (counter == track_counter.end())
            {
                track_counter[tracking_id] = 0;
            }
            else if (counter->second < TRACK_DELAY)
            {
                track_counter[tracking_id] += 1;
            }
            else
            {
                auto bbox = detection->get_bbox();
                float quality = OpenCVUtils::quality_estimation_reid(image, bbox, RE_ID_NETWORK_WIDTH, RE_ID_NETWORK_HEIGHT);
                float ratio = (bbox.height() * image->height()) / (bbox.width() * image->width());
                if (ratio > MIN_RATIO && ratio < MAX_RATIO &&
                    bbox.height() > MIN_HEIGHT && bbox.height() < MAX_HEIGHT &&
                    bbox.xmin() > MIN_X && bbox.xmax() < MAX_X && quality > MIN_QUALITY)
                {
                    crop_rois.emplace_back(detection);
                }
            }
        }
    }
    return crop_rois;
}
