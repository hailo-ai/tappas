/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#pragma once
#include "hailo_objects.hpp"
#include "hailo_common.hpp"
#include "xtensor/xarray.hpp"

__BEGIN_DECLS
class YolactParams
{
public:
    xt::xarray<float> anchors;
    YolactParams(xt::xarray<float> anchors)
    {
        this->anchors = anchors;
    }
};

void yolact_20_classes(HailoROIPtr roi, void *params_void_ptr);
void yolact800mf(HailoROIPtr roi, void *params_void_ptr);
void yolact1_6gf(HailoROIPtr roi, void *params_void_ptr);
void filter(HailoROIPtr roi, void *params_void_ptr);
YolactParams *init(const std::string config_path, const std::string function_name);
void free_resources(void *params_void_ptr);
__END_DECLS