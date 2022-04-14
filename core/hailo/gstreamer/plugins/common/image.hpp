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
 * @brief Get the cv::Mat object from the GstBuffer
 *
 * @param buffer The buffer to return as cv::Mat
 * @param caps The caps of the buffer in order to get measurements.
 * @return cv::Mat A cv Matrix of buffer.
 */
cv::Mat get_image(GstBuffer *buffer, GstCaps *caps, GstMapFlags flags);
__END_DECLS

