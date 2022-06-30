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

cv::Mat get_mat(GstVideoInfo *info, GstMapInfo *map)
{
    cv::Mat img;
    switch (info->finfo->format)
    {
    case GST_VIDEO_FORMAT_YUY2:
        img = cv::Mat(info->height, info->width / 2, CV_8UC4, (char *)map->data, info->stride[0]);
        break;
    case GST_VIDEO_FORMAT_RGB:
        img = cv::Mat(info->height, info->width, CV_8UC3, (char *)map->data, info->stride[0]);
        break;
    default:
        break;
    }

    return img;
}

void resize_yuy2(cv::Mat &cropped_image, cv::Mat &resized_image, int interpolation)
{
    // Split the yuy2 channels into Y U Y V
    std::vector<cv::Mat> channels(4);
    cv::split(cropped_image, channels);

    // Interlace Y channels and resize together
    cv::Mat merged_y_channels;
    cv::Mat resized_y_channels;
    cv::Mat y_channels[2] = {channels[0], channels[2]};
    cv::merge(y_channels, 2, merged_y_channels);
    // In order to resize the Y values together, they need to be viewed as a single channel Mat
    cv::Mat merged_y_channels_flat = cv::Mat(merged_y_channels.rows, merged_y_channels.cols * 2, CV_8UC1, (char *)merged_y_channels.data, merged_y_channels.cols * 2);
    cv::resize(merged_y_channels_flat, resized_y_channels, cv::Size(resized_image.cols * 2, resized_image.rows), interpolation);
    // We can make a 2 channel view of the resized image in order to split the Y channels again
    cv::Mat resized_y_channels_2_split = cv::Mat(resized_image.rows, resized_image.cols, CV_8UC2, (char *)resized_y_channels.data, resized_image.cols * 2);
    cv::split(resized_y_channels_2_split, y_channels);

    // Resize the U and V channels
    std::vector<cv::Mat> resized_channels(2);
    cv::resize(channels[1], resized_channels[0], cv::Size(resized_image.cols, resized_image.rows), interpolation);
    cv::resize(channels[3], resized_channels[1], cv::Size(resized_image.cols, resized_image.rows), interpolation);

    // Merge all resized channels
    cv::Mat channels_to_merge[4] = {y_channels[0], resized_channels[0], y_channels[1], resized_channels[1]};
    cv::merge(channels_to_merge, 4, resized_image);

    merged_y_channels.release();
    resized_y_channels.release();
    merged_y_channels_flat.release();
    resized_y_channels_2_split.release();
}