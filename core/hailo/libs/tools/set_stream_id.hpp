/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once
#include "hailo_objects.hpp"
#include "hailo_common.hpp"

#define RANDOM_DIM_LIMIT 0.001

__BEGIN_DECLS
char *init(std::string config_path, std::string func_name);
void print_stream_id(HailoROIPtr roi);
void set_stream_id(HailoROIPtr roi, void *params);
void filter(HailoROIPtr roi, void *params);
void free_resources(void *params_void_ptr);
__END_DECLS