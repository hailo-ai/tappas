/**
* Copyright (c) 2021-2026 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#ifndef _POSTPROCESSES_COMMON_TENSORS_HPP_
#define _POSTPROCESSES_COMMON_TENSORS_HPP_

#include "hailo_objects.hpp"
#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"

namespace common
{
    //-------------------------------
    // COMMON TRANSFORMS
    //-------------------------------
    template <typename T>
    xt::xarray<float> dequantize(const xt::xarray<T> &input, const float &qp_scale, const float &qp_zp)
    {
        // Rescale the input using the given scale and zero-point
        auto rescaled_data = (input - qp_zp) * qp_scale;
        return rescaled_data;
    }

    xt::xarray<uint8_t> get_xtensor(const HailoTensorPtr &tensor);
    xt::xarray<uint16_t> get_xtensor_uint16(const HailoTensorPtr &tensor);
    xt::xarray<float> get_xtensor_float(const HailoTensorPtr &tensor);

    /**
     * @brief Get the only the tensors (vector) from a map of string->tensor.
     *
     * @param tensors A map between tensors name to the tensor pointer
     * @return std::vector<HailoTensorPtr> A vector of tensor pointer.
     */
    std::vector<HailoTensorPtr> get_tensor_values(const std::map<std::string, HailoTensorPtr> &tensors);

    /**
     * @brief Scalar dequantization: (value - zero_point) * scale
     */
    template <typename T>
    float dequantize_value(const T val, const float qp_scale, const float qp_zp)
    {
        return (float(val) - qp_zp) * qp_scale;
    }

    /**
     * @brief Scalar quantization: (value / scale) + zero_point
     */
    template <typename T>
    T quantize_value(const float val, const float qp_scale, const float qp_zp)
    {
        return T((val / qp_scale) + qp_zp);
    }

}

#endif  // _POSTPROCESSES_COMMON_TENSORS_HPP_
