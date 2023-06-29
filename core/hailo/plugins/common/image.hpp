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
#include "hailomat.hpp"
#include "hailo_objects.hpp"

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
 * @param data Pointer to the data.
 * @return cv::Mat The mat object.
 */
cv::Mat get_mat_from_video_info(GstVideoInfo *info, char *data);

/**
 * @brief Get the mat object
 *
 * @param info The GstVideoInfo to extract mat from.
 * @param map The GstMapInfo to extract mat from.
 * @return cv::Mat The mat object.
 */
cv::Mat get_mat(GstVideoInfo *info, GstMapInfo *map);

/**
 * @brief Get the mat object from given GstVideoFrame
 *
 * @param info The GstVideoFrame to extract mat from.
 * @return cv::Mat The mat object.
 */
cv::Mat get_mat_from_gst_frame(GstVideoFrame *frame);

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

/**
 * @brief Resizes a NV12 image (1 channel cv::Mat)
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
void resize_nv12(cv::Mat &cropped_image, cv::Mat &resized_image, int interpolation = cv::INTER_LINEAR);

/**
 * @brief Resize an image using Letterbox strategy
 *
 * @param cropped_image - cv::Mat &
 *        The cropped image to resize
 *
 * @param resized_image - cv::Mat &
 *        The resized image container to fill
 *        (dims for resizing are assumed from here)
 *
 * @param color - cv::Scalar
 *        The color to fill the letterbox with
 *
 * @param interpolation - int
 *        The interpolation type to resize by.
 *        Must be a supported opencv type
 *        (bilinear, nearest neighbors, etc...)
 */
HailoBBox resize_letterbox_rgb(cv::Mat &cropped_image, cv::Mat &resized_image, cv::Scalar color, int interpolation = cv::INTER_LINEAR);

/**
 * @brief Resize an NV12 image using Letterbox strategy
 *
 * @param cropped_image - cv::Mat &
 *        The cropped image to resize
 *
 * @param resized_image - cv::Mat &
 *        The resized image container to fill
 *        (dims for resizing are assumed from here)
 *
 * @param color - cv::Scalar
 *        The color to fill the letterbox with
 *
 * @param interpolation - int
 *        The interpolation type to resize by.
 *        Must be a supported opencv type
 *        (bilinear, nearest neighbors, etc...)
 */
HailoBBox resize_letterbox_nv12(cv::Mat &cropped_image, cv::Mat &resized_image, cv::Scalar color, int interpolation = cv::INTER_LINEAR);
__END_DECLS

std::shared_ptr<HailoMat> get_mat_by_format(GstBuffer *buffer, GstVideoInfo *info, int line_thickness = 1, int font_thickness = 1);
