/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#ifndef _HAILO_YOLO_POST_HPP_
#define _HAILO_YOLO_POST_HPP_
#include "hailo_frame.hpp"

G_BEGIN_DECLS
void filter(HailoFramePtr hailo_frame);
void yolov3(HailoFramePtr hailo_frame);
void yolov4(HailoFramePtr hailo_frame);
void yolov4tiny(HailoFramePtr hailo_frame);
void yolov5(HailoFramePtr hailo_frame);
void yolov5_no_persons(HailoFramePtr hailo_frame);
void yolov5_counter(HailoFramePtr hailo_frame);
G_END_DECLS
#endif