/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once
#include "hailo_objects.hpp"
#include "hailo_common.hpp"

__BEGIN_DECLS
void filter(HailoROIPtr roi);
void resnet_v1_50(HailoROIPtr roi);
void mobilenet_v1(HailoROIPtr roi);
void resnet_v1_18(HailoROIPtr roi);
__END_DECLS