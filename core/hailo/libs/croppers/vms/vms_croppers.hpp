/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once
#include <vector>
#include <opencv2/opencv.hpp>
#include "hailo_objects.hpp"
#include "hailo_common.hpp"

__BEGIN_DECLS
std::vector<HailoROIPtr> person_attributes(cv::Mat mat, HailoROIPtr roi);
std::vector<HailoROIPtr> face_attributes(cv::Mat image, HailoROIPtr roi);
std::vector<HailoROIPtr> face_recognition(cv::Mat image, HailoROIPtr roi);

__END_DECLS