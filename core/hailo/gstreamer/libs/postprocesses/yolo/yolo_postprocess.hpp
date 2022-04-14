/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#ifndef _HAILO_YOLO_POST_HPP_
#define _HAILO_YOLO_POST_HPP_
#include "hailo_objects.hpp"
#include "hailo_common.hpp"


__BEGIN_DECLS
void filter(HailoROIPtr roi);
void yolox(HailoROIPtr roi);
void yoloxx (HailoROIPtr roi);
void yolov3(HailoROIPtr roi);
void yolov4(HailoROIPtr roi);
void tiny_yolov4_license_plates(HailoROIPtr roi);
void yolov5(HailoROIPtr roi);
void yolov5_no_persons(HailoROIPtr roi);
void yolov5_counter(HailoROIPtr roi);
void yolov5_vehicles_only(HailoROIPtr roi);
__END_DECLS
#endif