/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#ifndef _HAILO_CENTERPOSE_POST_HPP_
#define _HAILO_CENTERPOSE_POST_HPP_
#include "hailo_objects.hpp"
#include "hailo_common.hpp"

__BEGIN_DECLS
void filter(HailoROIPtr roi);
void centerpose(HailoROIPtr roi);
void centerpose_416(HailoROIPtr roi);
void centerpose_merged(HailoROIPtr roi);
__END_DECLS
#endif