/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include <vector>
#include <iostream>
#include "common/tensors.hpp"
#include "common/math.hpp"
#include "re_id.hpp"
#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"

#define OUTPUT_LAYER_NAME "repvgg_a0_person_reid_2048/fc1"

void remove_previous_matrices(HailoROIPtr roi)
{
    for (auto matrix : roi->get_objects_typed(HAILO_MATRIX))
    {
        roi->remove_object(matrix);
    }
}

HailoMatrixPtr create_matrix_ptr(xt::xarray<float> &xmatrix)
{
    // allocate and memcpy to a new memory so it points to the right data
    std::vector<float> data(xmatrix.size());
    memcpy(data.data(), xmatrix.data(), sizeof(float) * xmatrix.size());
    return std::make_shared<HailoMatrix>(std::move(data),
                                         xmatrix.shape(0), xmatrix.shape(1), xmatrix.shape(2));
}

void re_id(HailoROIPtr roi)
{

    if (!roi->has_tensors())
    {
        return;
    }

    remove_previous_matrices(roi);

    // Convert the tensor to xarray.
    auto tensor = roi->get_tensor(OUTPUT_LAYER_NAME);
    xt::xarray<float> embedding = common::get_xtensor_float(tensor);

    // vector normalization
    auto normalized_embedding = common::vector_normalization(embedding);

    roi->add_object(create_matrix_ptr(normalized_embedding));
}

void filter(HailoROIPtr roi)
{
    re_id(roi);
}
