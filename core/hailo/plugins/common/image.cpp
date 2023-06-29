/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/

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

cv::Mat get_mat_from_video_info(GstVideoInfo *info, char *data)
{
    cv::Mat img;
    switch (info->finfo->format)
    {
    case GST_VIDEO_FORMAT_YUY2:
        img = cv::Mat(info->height, info->width / 2, CV_8UC4, data, info->stride[0]);
        break;
    case GST_VIDEO_FORMAT_RGB:
        img = cv::Mat(info->height, info->width, CV_8UC3, data, info->stride[0]);
        break;
    case GST_VIDEO_FORMAT_RGBA:
        img = cv::Mat(info->height, info->width, CV_8UC4, data, info->stride[0]);
        break;
    case GST_VIDEO_FORMAT_NV12:
        img = cv::Mat(info->height * 3 / 2, info->width, CV_8UC1, data, info->stride[0]);
        break;
    default:
        break;
    }

    return img;
}

cv::Mat get_mat(GstVideoInfo *info, GstMapInfo *map)
{
    return get_mat_from_video_info(info, (char *)map->data);
}

cv::Mat get_mat_from_gst_frame(GstVideoFrame *frame)
{
    return get_mat_from_video_info(&frame->info, (char *)GST_VIDEO_FRAME_PLANE_DATA(frame, 0));
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
    cv::resize(merged_y_channels_flat, resized_y_channels, cv::Size(resized_image.cols * 2, resized_image.rows), 0, 0, interpolation);
    // We can make a 2 channel view of the resized image in order to split the Y channels again
    cv::Mat resized_y_channels_2_split = cv::Mat(resized_image.rows, resized_image.cols, CV_8UC2, (char *)resized_y_channels.data, resized_image.cols * 2);
    cv::split(resized_y_channels_2_split, y_channels);

    // Resize the U and V channels
    std::vector<cv::Mat> resized_channels(2);
    cv::resize(channels[1], resized_channels[0], cv::Size(resized_image.cols, resized_image.rows), 0, 0, interpolation);
    cv::resize(channels[3], resized_channels[1], cv::Size(resized_image.cols, resized_image.rows), 0, 0, interpolation);

    // Merge all resized channels
    cv::Mat channels_to_merge[4] = {y_channels[0], resized_channels[0], y_channels[1], resized_channels[1]};
    cv::merge(channels_to_merge, 4, resized_image);

    merged_y_channels.release();
    resized_y_channels.release();
    merged_y_channels_flat.release();
    resized_y_channels_2_split.release();
}

void resize_nv12(cv::Mat &cropped_image, cv::Mat &resized_image, int interpolation)
{
    // Split the nv12 mat into Y and UV mats
    uint width = cropped_image.cols;
    uint height = cropped_image.rows;
    cv::Mat y_mat = cv::Mat(height * 2 / 3, width, CV_8UC1, (char *)cropped_image.data, cropped_image.step);
    cv::Mat uv_mat = cv::Mat(height / 3, width / 2, CV_8UC2, (char *)cropped_image.data + ((height * 2 / 3) * width), cropped_image.step);

    // Resize the U and V channels
    uint resize_width = resized_image.cols;
    uint resize_height = resized_image.rows;

    cv::Mat resized_y_channel = cv::Mat(resize_height * 2 / 3, resize_width, CV_8UC1, (char *)resized_image.data, resize_width);
    cv::Mat resized_uv_channels = cv::Mat(resize_height / 3, resize_width / 2, CV_8UC2, (char *)resized_image.data + ((resize_height * 2 / 3) * resize_width), resize_width);
    cv::resize(y_mat, resized_y_channel, cv::Size(resize_width, resize_height * 2 / 3), 0, 0, interpolation);
    cv::resize(uv_mat, resized_uv_channels, cv::Size(resize_width / 2, resize_height / 3), 0, 0, interpolation);
}

HailoBBox resize_letterbox_rgb(cv::Mat &cropped_image, cv::Mat &resized_image, cv::Scalar color, int interpolation)
{
    cv::Mat tmp;
    float ratio = std::min(float(resized_image.rows) / cropped_image.rows,
                           float(resized_image.cols) / cropped_image.cols);
    int new_width = std::round(cropped_image.cols * ratio);
    int new_height = std::round(cropped_image.rows * ratio);

    cv::resize(cropped_image, tmp, cv::Size(new_width, new_height), 0, 0, interpolation);

    float middle_point_width = (resized_image.cols - new_width) / 2;
    float middle_point_height = (resized_image.rows - new_height) / 2;

    // Calculate the number of pixels we should colorize
    int top = std::round(middle_point_height - 0.1);
    int bottom = std::round(middle_point_height + 0.1);
    int left = std::round(middle_point_width - 0.1);
    int right = std::round(middle_point_width + 0.1);

    HailoBBox letterboxed_scale = HailoBBox(-(left / float(new_width)),                      // x-offset
                                            -(top / float(new_height)),                      // y-offset
                                            1.0 / (new_width / float(resized_image.cols)),   // width factor
                                            1.0 / (new_height / float(resized_image.rows))); // height factor

    cv::copyMakeBorder(tmp, resized_image, top, bottom, left, right, cv::BORDER_CONSTANT, color);
    tmp.release();
    return letterboxed_scale;
}

HailoBBox resize_letterbox_nv12(cv::Mat &cropped_image, cv::Mat &resized_image, cv::Scalar color, int interpolation)
{
    // Convert the color to YUV pixel format
    uint y = RGB2Y(color[0], color[1], color[2]);
    uint u = RGB2U(color[0], color[1], color[2]);
    uint v = RGB2V(color[0], color[1], color[2]);
    cv::Scalar nv12_color(y, u, v);

    uint width = cropped_image.cols;
    uint height = cropped_image.rows;

    uint resize_width = resized_image.cols;
    uint resize_height = resized_image.rows;

    // Split the image into Y and UV channels
    cv::Mat y_mat = cv::Mat(height * 2 / 3, width, CV_8UC1, (char *)cropped_image.data, cropped_image.step);
    cv::Mat resized_y_channel = cv::Mat(resize_height * 2 / 3, resize_width, CV_8UC1, (char *)resized_image.data, resize_width);

    cv::Mat uv_mat = cv::Mat(height / 3, width / 2, CV_8UC2, (char *)cropped_image.data + ((height * 2 / 3) * width), cropped_image.step);
    cv::Mat resized_uv_channel = cv::Mat(resize_height / 3, resize_width / 2, CV_8UC2, (char *)resized_image.data + ((resize_height * 2 / 3) * resize_width), resize_width);

    // Perform the letterbox resize on the Y and UV channels separately
    HailoBBox letterboxed_scale = resize_letterbox_rgb(y_mat, resized_y_channel, nv12_color, interpolation);
    resize_letterbox_rgb(uv_mat, resized_uv_channel, nv12_color, interpolation);

    return letterboxed_scale;
}

std::shared_ptr<HailoMat> get_mat_by_format(GstBuffer *buffer, GstVideoInfo *info, int line_thickness, int font_thickness)
{
    std::shared_ptr<HailoMat> hmat = nullptr;
    GstVideoFrame frame;
    bool success = gst_video_frame_map(&frame, info, buffer, GstMapFlags(GST_MAP_READ));
    if (!success)
    {
        gst_video_frame_unmap(&frame);
        GST_CAT_ERROR(GST_CAT_DEFAULT, "Failed to map buffer to video frame, Buffer may be not writable");
        throw std::runtime_error("Failed to map buffer to video frame, Buffer may be not writable");
    }

    uint8_t *plane0_data = (uint8_t *)GST_VIDEO_FRAME_PLANE_DATA(&frame, 0);

    switch (GST_VIDEO_INFO_FORMAT(info))
    {
    case GST_VIDEO_FORMAT_RGB:
    {
        hmat = std::make_shared<HailoRGBMat>(plane0_data,
                                             GST_VIDEO_INFO_HEIGHT(info),
                                             GST_VIDEO_INFO_WIDTH(info),
                                             GST_VIDEO_INFO_PLANE_STRIDE(info, 0),
                                             line_thickness,
                                             font_thickness);
        break;
    }
    case GST_VIDEO_FORMAT_RGBA:
    {
        hmat = std::make_shared<HailoRGBAMat>(plane0_data,
                                              GST_VIDEO_INFO_HEIGHT(info),
                                              GST_VIDEO_INFO_WIDTH(info),
                                              GST_VIDEO_INFO_PLANE_STRIDE(info, 0),
                                              line_thickness,
                                              font_thickness);
        break;
    }
    case GST_VIDEO_FORMAT_YUY2:
    {
        hmat = std::make_shared<HailoYUY2Mat>(plane0_data,
                                              GST_VIDEO_INFO_HEIGHT(info),
                                              GST_VIDEO_INFO_WIDTH(info),
                                              GST_VIDEO_INFO_PLANE_STRIDE(info, 0),
                                              line_thickness,
                                              font_thickness);
        break;
    }
    case GST_VIDEO_FORMAT_NV12:
    {
        hmat = std::make_shared<HailoNV12Mat>(plane0_data,
                                            GST_VIDEO_INFO_HEIGHT(info),
                                            GST_VIDEO_INFO_WIDTH(info),
                                            GST_VIDEO_INFO_PLANE_STRIDE(info, 0),
                                            GST_VIDEO_INFO_PLANE_STRIDE(info, 1),
                                            line_thickness,
                                            font_thickness,
                                            plane0_data,
                                            GST_VIDEO_FRAME_PLANE_DATA(&frame, 1));
        break;
    }

    default:
        break;
    }

    gst_video_frame_unmap(&frame);
    return hmat;
}