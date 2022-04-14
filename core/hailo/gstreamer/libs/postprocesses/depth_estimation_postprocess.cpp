/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/

#include <vector>
#include <string>
#include <gst/gst.h>

#include "depth_estimation_postprocess.hpp"
#include "tensor_meta.hpp"
#include "hailo_frame.hpp"
#include "common/common.hpp"

#include "xtensor/xtensor.hpp"
#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"

#define MIN_DISTANCE 0.5
#define MAX_DISTANCE 3

void fast_depth(HailoFramePtr hailo_frame)
{
    auto tensors = hailo_frame->get_tensors();
    if (tensors.size() == 0)
    {
        return;
    }
    HailoTensorPtr output_tensor = tensors[0];

    std::vector<std::size_t> shape = {output_tensor->width, output_tensor->height, output_tensor->channels};
    int size = output_tensor->width * output_tensor->height * output_tensor->channels;

    // get the output buffer in uint16 format, and parse it to xarray in the proper size
    xt::xarray<uint16_t> logits = xt::adapt(output_tensor->data_as_uint16, size, xt::no_ownership(), shape);
    // de-quantization of the xarray from uint16 to float32
    auto logits_dequantized = common::dequantize(logits, output_tensor->qp_scale, output_tensor->qp_zp);

    float min = MIN_DISTANCE;
    float max = MAX_DISTANCE;

    auto amax = xt::amax(logits_dequantized);
    auto amin = xt::amin(logits_dequantized);

    if (amax[0] > max)
        max = amax[0];
    if (amin[0] < min)
        min = amin[0];

    // change the scale of the logits to match 255 RGB range (the results is values between zero to one)
    auto logits_rescaled = (logits_dequantized - min) / (max - min);

    // clip the logits between 0 to 255 and cast it to uint8
    auto logits_clipped = xt::cast<uint8_t>(xt::clip((logits_rescaled * 255), 0, 255));

    std::vector<std::size_t> frame_shape = {hailo_frame->width, hailo_frame->height, 3};
    int frame_size = hailo_frame->width * hailo_frame->height * 3;
    auto frame_matrix = xt::adapt(hailo_frame->plane_data, frame_size, xt::no_ownership(), frame_shape);

    // empty frame
    xt::view(frame_matrix, xt::all(), xt::all(), xt::all()) = xt::view(xt::zeros<uint8_t>(frame_shape), xt::all(), xt::all(), xt::all());
    // place color values in the features of the frame matrix (placing the value in the R of RGB)
    xt::view(frame_matrix, xt::all(), xt::all(), 0) = xt::view(logits_clipped, xt::all(), xt::all(), 0);
}

void filter(HailoFramePtr hailo_frame)
{
    fast_depth(hailo_frame);
}