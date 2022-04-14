/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/**
 * @file overlay.cpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-01-20
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "common/image.hpp"

size_t get_size(GstCaps *caps)
{
    size_t size;
    GstVideoInfo *info = gst_video_info_new();
    gst_video_info_from_caps(info, caps);
    size = info->size;
    gst_video_info_free(info);
    return size;
}

cv::Mat get_image(GstBuffer *buffer, GstCaps *caps, GstMapFlags flags)
{
    GstMapInfo map;
    GstVideoInfo *info = gst_video_info_new();
    cv::Mat img;
    gst_buffer_map(buffer, &map, flags);
    gst_video_info_from_caps(info, caps);
    switch (info->finfo->format)
    {
    case GST_VIDEO_FORMAT_YUY2:
        img = cv::Mat(info->height, info->width / 2, CV_8UC4, (char *)map.data, info->stride[0]);

        break;
    case GST_VIDEO_FORMAT_RGB:
        img = cv::Mat(info->height, info->width, CV_8UC3, (char *)map.data, info->stride[0]);
        break;
    default:
        break;
    }
    gst_buffer_unmap(buffer, &map);
    gst_video_info_free(info);
    return img;
}