/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once
#include "hailo_objects.hpp"
#include "hailo_common.hpp"

__BEGIN_DECLS
void filter(HailoROIPtr roi);
// Post-process function used to add landmarks on given detection.
// Used for Face Detection + Face Landmarks app.
void facial_landmarks_merged(HailoROIPtr roi);
void facial_landmarks_yuy2(HailoROIPtr roi);
__END_DECLS