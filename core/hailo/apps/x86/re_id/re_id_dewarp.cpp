/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/

#include <gst/video/video-format.h>
#include <gst/video/video.h>
#include <iostream>
#include <map>
#include <typeinfo>
#include <math.h>

// Hailo includes
#include "re_id_overlay.hpp"
#include "hailo_common.hpp"

// Open source includes
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

cv::Mat fisheye_map1;
cv::Mat fisheye_map2;
bool fisheye_map_created = false;

void filter(HailoROIPtr roi, GstVideoFrame *frame, gchar *current_stream_id)
{
    cv::Mat remap_mat;
    cv::Mat image_mat(GST_VIDEO_FRAME_HEIGHT(frame),
                GST_VIDEO_FRAME_WIDTH(frame),
                CV_8UC3,
                GST_VIDEO_FRAME_PLANE_DATA(frame, 0),
                GST_VIDEO_FRAME_PLANE_STRIDE(frame, 0));
    if (!fisheye_map_created)
    {
        // Static Matrices that represent the fisheye configuration for the specifix videos/cameras we use.

        // Camera Matrix
        cv::Mat cam(3, 3, cv::DataType<float>::type);
        cam.at<float>(0, 0) = 1328.3905382843832f;
        cam.at<float>(0, 1) = 0.0f;
        cam.at<float>(0, 2) = 1006.5378470232891f;

        cam.at<float>(1, 0) = 0.0f;
        cam.at<float>(1, 1) = 1356.204081943469f;
        cam.at<float>(1, 2) = 649.6687619615067f;

        cam.at<float>(2, 0) = 0.0f;
        cam.at<float>(2, 1) = 0.0f;
        cam.at<float>(2, 2) = 1.0f;

        // Distance Coeffitients matrix
        cv::Mat dist(4, 1, cv::DataType<float>::type);
        dist.at<float>(0, 0) = -0.04559713237248377f;
        dist.at<float>(1, 0) = -0.2200614611319084f;
        dist.at<float>(2, 0) = 0.47521443770963995f;
        dist.at<float>(3, 0) = -0.38690394174238846f;

        // Create Fisheye dewarp map.
        cv::fisheye::initUndistortRectifyMap(cam, dist, cv::Mat(), cam,
                                             cv::Size(GST_VIDEO_FRAME_WIDTH(frame),
                                                      GST_VIDEO_FRAME_HEIGHT(frame)),
                                             CV_16SC2, fisheye_map1, fisheye_map2);
        // Mark boolean true in order to do this calculation only once.
        fisheye_map_created = true;
    }

    // Remap using 2 fisheye dewarp maps.
    cv::remap(image_mat, remap_mat, fisheye_map1, fisheye_map2, cv::INTER_LINEAR);

    // Copy the dewarped image to the buffer image.
    memcpy(image_mat.data, remap_mat.data, sizeof(uint8_t) * remap_mat.rows * remap_mat.cols * 3);
    remap_mat.release();
    image_mat.release();
}