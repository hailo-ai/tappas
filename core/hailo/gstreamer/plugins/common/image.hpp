/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
/**
 * @file overlay/common.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-01-20
 *
 * @copyright Copyright (c) 2022
 *
 */

#pragma once

#include <gst/video/video.h>
#include <opencv2/opencv.hpp>

__BEGIN_DECLS

/**
 * @brief Get the size of buffer with specific caps
 *
 * @param caps The caps to extract size from.
 * @return size_t The size of a buffer with those caps.
 */
size_t get_size(GstCaps *caps);

/**
 * @brief Get the mat object
 *
 * @param info The GstVideoInfo to extract mat from.
 * @param map The GstMapInfo to extract mat from.
 * @return cv::Mat The mat object.
 */
cv::Mat get_mat(GstVideoInfo *info, GstMapInfo *map);

/**
 * @brief Resizes a YUY2 image (4 channel cv::Mat)
 *
 * @param cropped_image - cv::Mat &
 *        The cropped image to resize
 *
 * @param resized_image - cv::Mat &
 *        The resized image container to fill
 *        (dims for resizing are assumed from here)
 *
 * @param interpolation - int
 *        The interpolation type to resize by.
 *        Must be a supported opencv type
 *        (bilinear, nearest neighbors, etc...)
 */
void resize_yuy2(cv::Mat &cropped_image, cv::Mat &resized_image, int interpolation = cv::INTER_LINEAR);
__END_DECLS
