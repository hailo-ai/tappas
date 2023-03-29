/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include "3ddfa.hpp"
#include <iostream>

#define FACE_LABEL "face"

/**
 * @brief Returns an adjusted HailoBBox acordding to 3ddfa cropping algorithm.
 *
 * @param image The original picture (cv::Mat).
 * @param roi The ROI to modify
 * @return HailoBBox Adjusted HailoBBox to crop.
 * @note Original algorithm at https://github.com/cleardusk/3DDFA_V2/blob/9fdbea1eb97f762221f71f5c76f08f52296c6704/utils/functions.py#L85
 */
HailoBBox algorithm_3ddfa(cv::Mat &image, const HailoBBox &roi)
{
    // Algorithm
    float old_size = (roi.width() * image.cols + roi.height() * image.rows) / 2;
    float center_x = (2 * roi.xmin() + roi.width()) / 2;
    float center_y = (2 * roi.ymin() + roi.height()) / 2 + old_size * 0.14 / image.rows;
    float size = old_size * 1.58;
    float h_size = size / image.rows;
    float w_size = size / image.cols;
    float xmin = CLAMP((center_x - w_size / 2), 0, 1);
    float ymin = CLAMP((center_y - h_size / 2), 0, 1);
    return HailoBBox(xmin, ymin, CLAMP(w_size, 0, 1 - xmin), CLAMP(h_size, 0, 1 - ymin));
}

/**
 * @brief Returns a vector of HailoROIPtr to crop and resize.
 *
 * @param image The original picture (cv::Mat).
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
        // Modify only detections with "face" label.
        if (std::string(FACE_LABEL) == detection->get_label())
        {
            // Modifies a rectengle according to 3ddfa cropping algorithm only on faces
            auto new_bbox = algorithm_3ddfa(image->get_mat(), detection->get_bbox());
            detection->set_bbox(new_bbox);
            crop_rois.emplace_back(detection);
        }
    }
    return crop_rois;
}
