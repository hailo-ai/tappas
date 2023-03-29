/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once
#include "hailo_objects.hpp"
#include "hailo_common.hpp"

#define RANDOM_DIM_LIMIT 0.001

__BEGIN_DECLS
void filter(HailoROIPtr roi);
void frame_counter_a(HailoROIPtr roi);
void frame_counter_b(HailoROIPtr roi);
void frame_counter_c(HailoROIPtr roi);
void frame_counter_d(HailoROIPtr roi);
void frame_counter_e(HailoROIPtr roi);
void time_a(HailoROIPtr roi);
void time_b(HailoROIPtr roi);
void time_c(HailoROIPtr roi);
void time_d(HailoROIPtr roi);
void time_e(HailoROIPtr roi);
void sleep10(HailoROIPtr roi);
void identity(HailoROIPtr roi);
void generate_random_detections(HailoROIPtr roi);
void generate_center_detection(HailoROIPtr roi);
void generate_bottom_detection(HailoROIPtr roi);
void print_roi_bboxs(HailoROIPtr roi);
void dump_tensors_to_npy(HailoROIPtr roi);
__END_DECLS