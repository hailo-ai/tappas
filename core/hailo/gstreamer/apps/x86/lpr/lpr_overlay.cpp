/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include <gst/video/video.h>
#include <iostream>
#include <map>
#include <typeinfo>
#include <math.h>
#include "lpr_overlay.hpp"
#include "hailo_detection.hpp"

// Open source includes
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

// CV Formatting
#define DEFAULT_FORMAT_FACTOR (1)
#define YUY2_FORMAT_FACTOR (0.5)

// General
#define TEXT_THICKNESS (1)
#define TEXT_CLS_THICKNESS (2)
#define SPACE " "

#define OCR_LIMIT 5
const char *FILE_DIR = "";
std::vector<std::string> file_names = {"lp_0.jpg", "lp_1.jpg", "lp_2.jpg", "lp_3.jpg", "lp_4.jpg"};

void draw_lpr(cv::Mat &mat)
{
    for (int i = 0; i < OCR_LIMIT; i++)
    {
        std::string full_path = std::string(FILE_DIR) + "/" + file_names[i];
        cv::Mat lp_image = cv::imread(full_path, cv::IMREAD_COLOR);
        if(!lp_image.data )
            continue;
        int xmin = mat.cols - lp_image.cols;
        int ymin = i * lp_image.rows;

        cv::Rect roi( cv::Point( xmin, ymin ), cv::Size( lp_image.cols, lp_image.rows ));
        cv::Mat destinationROI = mat( roi );
        cv::Mat rgb_image;
        cv::cvtColor(lp_image, rgb_image, cv::COLOR_BGR2RGB);
        rgb_image.copyTo(destinationROI);
        rgb_image.release();
        lp_image.release();
    }
}


void filter(HailoFramePtr hailo_frame)
{
    // Set the working directory
    FILE_DIR = std::getenv("TAPPAS_WORKSPACE");

    gint cv2_format;
    uint matrix_width;

    switch (hailo_frame->format)
    {
    case GST_VIDEO_FORMAT_YUY2:
        cv2_format = CV_8UC4;
        matrix_width = hailo_frame->width / 2;
        break;
    case GST_VIDEO_FORMAT_RGB:
    default:
        cv2_format = CV_8UC3;
        matrix_width = hailo_frame->width;
        break;
    }

    auto image_planes = cv::Mat(hailo_frame->height, matrix_width, cv2_format,
                                hailo_frame->plane_data, hailo_frame->stride);
    
    draw_lpr(image_planes);

    image_planes.release();
}