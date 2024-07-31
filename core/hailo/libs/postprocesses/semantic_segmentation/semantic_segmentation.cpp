/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include <regex>
#include "semantic_segmentation.hpp"
#include "common/tensors.hpp"

void semantic_segmentation(HailoROIPtr roi)
{
    if (!roi->has_tensors())
    {
        return;
    }
    HailoTensorPtr tensor_ptr;
    std::vector<HailoTensorPtr> tensors = roi->get_tensors();
    // find the argmax1 tensor
    for (auto tensor : tensors)
    {
        if (std::regex_search(tensor->name(), std::regex("argmax"))) 
        {
            tensor_ptr = tensor;
        }
    }
    if (!tensor_ptr)
    {
        std::cerr << "Semantic Segmentation post process: No argmax tensor found" << std::endl;
        return;
    }
    xt::xarray<uint8_t> tensor_data = common::get_xtensor(tensor_ptr);

    // allocate and memcpy to a new memory so it points to the right data
    std::vector<uint8_t> data(tensor_ptr->size());
    memcpy(data.data(), tensor_ptr->data(), sizeof(uint8_t) * tensor_ptr->size());
    auto obj_ptr = std::make_shared<HailoClassMask>(std::move(data), tensor_ptr->width(), tensor_ptr->height(), 0.3);
    hailo_common::add_object(roi, obj_ptr);
}

void filter(HailoROIPtr roi)
{
    semantic_segmentation(roi);
}