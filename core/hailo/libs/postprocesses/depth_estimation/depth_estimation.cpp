/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include "depth_estimation.hpp"
#include "common/tensors.hpp"

#include "xtensor/xtensor.hpp"
#include "xtensor/xarray.hpp"

const char *output_layer_name = "fast_depth/conv20";
void fast_depth(HailoROIPtr roi)
{
    if (!roi->has_tensors())
    {
        return;
    }
    HailoTensorPtr tensor_ptr = roi->get_tensor(output_layer_name);

    // get the output buffer in uint16 format, and parse it to xarray in the proper size
    xt::xarray<uint16_t> tensor_data = common::get_xtensor_uint16(tensor_ptr);

    // de-quantization of the xarray from uint16 to float32
    xt::xarray<float> logits_dequantized = common::dequantize(tensor_data, tensor_ptr->vstream_info().quant_info.qp_scale, tensor_ptr->vstream_info().quant_info.qp_zp);
    // here, logits_dequantized containes the estimated depth of each pixel in meters.

    // allocate and memcpy to a new memory so it points to the right data
    std::vector<float> data(logits_dequantized.size());
    memcpy(data.data(), logits_dequantized.data(), sizeof(float) * logits_dequantized.size());

    hailo_common::add_object(roi, std::make_shared<HailoDepthMask>(std::move(data), tensor_ptr->width(), tensor_ptr->height(), 1.0));
}

void filter(HailoROIPtr roi)
{
    fast_depth(roi);
}