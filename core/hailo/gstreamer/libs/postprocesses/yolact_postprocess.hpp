/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#ifndef _HAILO_YOLACT_POST_HPP_
#define _HAILO_YOLACT_POST_HPP_
#include "hailo_objects.hpp"
#include "hailo_common.hpp"
#include "common/tensors.hpp"

__BEGIN_DECLS
void yolact(HailoROIPtr roi);
void filter(HailoROIPtr roi);
__END_DECLS
#endif