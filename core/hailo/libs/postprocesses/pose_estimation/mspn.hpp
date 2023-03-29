/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once
#include "hailo_objects.hpp"
#include "hailo_common.hpp"

__BEGIN_DECLS

class MSPNParams
{
public:
    bool gaussian_blur;
    MSPNParams() : gaussian_blur(true) {}
};


void mspn(HailoROIPtr roi);
void filter(HailoROIPtr roi, void *params_void_ptr);
void free_resources(void *params_void_ptr);
MSPNParams *init(const std::string config_path);
__END_DECLS