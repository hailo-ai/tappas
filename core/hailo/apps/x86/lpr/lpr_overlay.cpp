/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/

#include <gst/video/video-format.h>
#include <iostream>
#include <map>
#include <typeinfo>
#include <math.h>

// Hailo includes
#include "lpr_overlay.hpp"
#include "image.hpp"
#include "hailo_cv_singleton.hpp"

// Open source includes
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

// General
#define TEXT_THICKNESS (1)
#define TEXT_CLS_THICKNESS (2)

#define OCR_LIMIT 5

void draw_lpr(cv::Mat &mat)
{
    cv::Mat singleton_image;
    cv::Mat destinationROI;
    for (int i = 0; i < OCR_LIMIT; i++)
    {
        // Get the cv::Mat for this index, if any
        singleton_image = CVMatSingleton::GetInstance().get_mat_at_key(i);
        if (!singleton_image.empty())
        {
            // The CV Map singleton had an image for this key, so draw it
            int xmin = mat.cols - singleton_image.cols;
            int ymin = i * singleton_image.rows;

            cv::Rect roi(cv::Point(xmin, ymin), cv::Size(singleton_image.cols, singleton_image.rows));
            destinationROI = mat(roi);
            singleton_image.copyTo(destinationROI);
        }

        singleton_image.release();
        destinationROI.release();
    }
}

void filter(HailoROIPtr roi, GstVideoFrame *frame, gchar *current_stream_id)
{
    auto image_planes = get_mat_from_gst_frame(frame);
    draw_lpr(image_planes);

    image_planes.release();
}