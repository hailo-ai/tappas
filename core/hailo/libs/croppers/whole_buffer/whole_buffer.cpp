/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include "whole_buffer.hpp"

/**
 * @brief Returns a vector of HailoROIPtr to crop and resize.
 *        Specifically, this algorithm doesn't make any actual filter,
 *        it just returns the whole ROI (ie: crop the whole picture)
 *
 * @param image The original picture (cv::Mat).
 * @param roi The main ROI of this picture.
 * @return std::vector<HailoROIPtr> vector of ROI's to crop and resize.
 */
std::vector<HailoROIPtr> create_crops(std::shared_ptr<HailoMat> image, HailoROIPtr roi)
{
    std::vector<HailoROIPtr> crop_rois;
    crop_rois.emplace_back(roi);
    return crop_rois;
}
