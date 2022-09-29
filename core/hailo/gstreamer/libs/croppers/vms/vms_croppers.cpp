/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include <vector>
#include "vms_croppers.hpp"

#define PERSON_LABEL "person"
#define FACE_LABEL "face"
#define FACE_ATTRIBUTES_CROP_HEIGHT_FACTOR (0.8f)
#define FACE_ATTRIBUTES_CROP_CENTER_OFFSET_FACTOR (0.04f)
#define TRACK_UPDATE 40

std::map<int, int> track_counter;

/**
 * @brief Returns a vector of Person detections to crop and resize.
 *
 * @param image The original picture (cv::Mat).
 * @param roi The main ROI of this picture.
 * @return std::vector<HailoROIPtr> vector of ROI's to crop and resize.
 */
std::vector<HailoROIPtr> person_attributes(cv::Mat image, HailoROIPtr roi)
{
    std::vector<HailoROIPtr> crop_rois;
    // Get all detections.
    std::vector<HailoDetectionPtr> detections_ptrs = hailo_common::get_hailo_detections(roi);
    for (HailoDetectionPtr &detection : detections_ptrs)
    {
        // Modify only detections with "person" label.
        if (std::string(PERSON_LABEL) == detection->get_label())
        {
            if (!hailo_common::has_classifications(detection, std::string("person_attributes")))
            {
                crop_rois.emplace_back(detection);
            }
        }
    }
    return crop_rois;
}

/**
 * @brief Returns an adjusted HailoBBox acordding to 3ddfa cropping algorithm.
 *
 * @param image The original picture (cv::Mat).
 * @param roi The ROI to modify
 * @return HailoBBox Adjusted HailoBBox to crop.
 * @note Original algorithm at https://github.com/cleardusk/3DDFA_V2/blob/9fdbea1eb97f762221f71f5c76f08f52296c6704/utils/functions.py#L85
 */
HailoBBox algorithm_face_attributes(cv::Mat &image, const HailoBBox &roi)
{
    // Algorithm
    float old_size = (roi.width() * image.cols + roi.height() * image.rows) / 2;
    float center_offset =  old_size * FACE_ATTRIBUTES_CROP_CENTER_OFFSET_FACTOR / image.rows;
    float center_x = (2 * roi.xmin() + roi.width()) / 2;
    float center_y = (2 * roi.ymin() + roi.height()) / 2 + center_offset;
    float size = old_size * 1.58;
    // Determine the new width and height, height should be a little bigger.
    float h_size = size * FACE_ATTRIBUTES_CROP_HEIGHT_FACTOR / image.rows;
    float w_size = size / image.cols;
    // Determine the top left corner of the crop.
    float xmin = CLAMP((center_x - w_size / 2), 0, 1);
    float ymin = CLAMP((center_y - h_size / 2), 0, 1);
    return HailoBBox(xmin, ymin, CLAMP(w_size, 0, 1 - xmin), CLAMP(h_size, 0, 1 - ymin));
}

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
 * @brief Returns a vector of face detections to crop and resize.
 *
 * @param image The original picture (cv::Mat).
 * @param roi The main ROI of this picture.
 * @return std::vector<HailoROIPtr> vector of ROI's to crop and resize.
 */
std::vector<HailoROIPtr> face_attributes(cv::Mat image, HailoROIPtr roi)
{
    std::vector<HailoROIPtr> crop_rois;
    // Get all detections.
    std::vector<HailoDetectionPtr> detections_ptrs = hailo_common::get_hailo_detections(roi);
    for (HailoDetectionPtr &detection : detections_ptrs)
    {
        // Modify only detections with "face" label.
        if (std::string(FACE_LABEL) == detection->get_label())
        {
            int tracking_id = get_tracking_id(detection)->get_id();

            auto counter = track_counter.find(tracking_id);
            if (counter == track_counter.end() || counter->second >= TRACK_UPDATE)
            {
                // Modifies a rectengle according to 3ddfa cropping algorithm only on faces
                auto new_bbox = algorithm_face_attributes(image, detection->get_bbox());
                detection->set_bbox(new_bbox);
                crop_rois.emplace_back(detection);
                track_counter[tracking_id] = 0;
            }
            else
                track_counter[tracking_id] += 1;
        }
    }
    return crop_rois;
}
