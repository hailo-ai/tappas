/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/**
 * @file overlay.hpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-01-20
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#include <map>
#include <vector>
#include "hailo_common.hpp"

#define DEFAULT_COLOR (cv::Scalar(255, 0, 0))
typedef enum
{
    OVERLAY_STATUS_UNINITIALIZED = -1,
    OVERLAY_STATUS_OK,

} overlay_status_t;

__BEGIN_DECLS
overlay_status_t draw_all(cv::Mat &mat, HailoROIPtr roi, int font_thickness = 1, int line_thickness = 1);

cv::Scalar RGB_TO_YUY2(cv::Scalar rgb);
cv::Scalar get_color(cv::Mat &mat, cv::Scalar color = DEFAULT_COLOR);
cv::Scalar indexToColor(size_t index);

__END_DECLS