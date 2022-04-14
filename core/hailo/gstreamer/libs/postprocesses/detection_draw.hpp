/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include "hailo_frame.hpp"
#include "xtensor/xarray.hpp"
#include <opencv2/opencv.hpp>

G_BEGIN_DECLS
void connect_landmarks(cv::Mat &image_planes, GstStructure *landmarks, xt::xarray<float> &keypoints, cv::Scalar &color, int &thickness);
void draw_landmarks(cv::Mat &image_planes, GstStructure *landmarks, cv::Scalar &color1, cv::Scalar &color2, int &thickness);
void filter(HailoFramePtr hailo_frame);
G_END_DECLS