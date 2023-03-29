/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once
#include "hailo_objects.hpp"
#include "hailo_common.hpp"
#include "xtensor/xarray.hpp"

class FaceDetectionParams
{
public:
    xt::xarray<float> anchors;
    xt::xarray<float> anchors_multiplier;
    xt::xarray<float> anchor_variance;
    std::vector<std::vector<int>> anchor_min_size;
    float score_threshold;
    float iou_threshold;
    int num_branches;

    FaceDetectionParams(xt::xarray<float> anchors,
    xt::xarray<float> anchors_multiplier,
    xt::xarray<float> anchor_variance,
    std::vector<std::vector<int>> anchor_min_size,
    float score_threshold,
    float iou_threshold,
    int num_branches) {
        this->anchors = anchors;
        this->anchors_multiplier =anchors_multiplier;
        this->anchor_variance = anchor_variance;
        this->anchor_min_size = anchor_min_size;
        this->score_threshold = score_threshold;
        this->iou_threshold = iou_threshold;
        this->num_branches = num_branches;
    }
};

__BEGIN_DECLS
void retinaface(HailoROIPtr roi, void *params_void_ptr);
void lightface(HailoROIPtr roi, void *params_void_ptr);
void filter(HailoROIPtr roi, void *params_void_ptr);
FaceDetectionParams *init(const std::string config_path, const std::string function_name);
void free_resources(void *params_void_ptr);
xt::xarray<float> get_anchors(const std::vector<std::vector<int>> &anchor_min_sizes,
                              const xt::xarray<int> &anchor_steps,
                              const int width,
                              const int height);

__END_DECLS