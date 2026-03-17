/**
* Copyright (c) 2021-2026 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/**
 * @file overlay.hpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-01-20
 * 
 * @copyright Copyright (c) 2022-2026
 * 
 */
#pragma once

#include <map>
#include <vector>
#include <cstddef>
#include "hailo_objects.hpp"
#include "hailomat.hpp"

typedef enum
{
    OVERLAY_STATUS_UNINITIALIZED = -1,
    OVERLAY_STATUS_OK,

} overlay_status_t;

/**
 * @brief Simple BGR color structure (OpenCV-independent)
 */
struct rgb_color_t {
    double val[3];

    rgb_color_t(double b = 0, double g = 0, double r = 0) : val{b, g, r} {}

    double& operator[](size_t i) { return val[i]; }
    const double& operator[](size_t i) const { return val[i]; }
};

__BEGIN_DECLS
overlay_status_t draw_all(HailoMat &hmat, HailoROIPtr roi, float landmark_point_radius, bool show_confidence = true, bool local_gallery = false, uint mask_overlay_n_threads = 0);
void face_blur(HailoMat &mat, HailoROIPtr roi);

rgb_color_t indexToColor(size_t index);

__END_DECLS
