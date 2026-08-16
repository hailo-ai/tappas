/**
 * Copyright (c) 2021-2026 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#ifndef _POSTPROCESSES_COMMON_MATH_HPP_
#define _POSTPROCESSES_COMMON_MATH_HPP_

#include "xtensor/xarray.hpp"
#include "xtensor/xeval.hpp"
#include "xtensor/xmath.hpp"
#include "xtensor/xsort.hpp"
#include "xtensor/xview.hpp"
#include "xtensor/xio.hpp"

namespace common
{

    //-------------------------------
    // SIGMOID
    //-------------------------------

    float sigmoid(float x);
    float inverse_sigmoid(float y);
    void sigmoid_inplace(float *data, const int size);

    auto xtensor_sigmoid(const auto &tensor)
    {
        return 1 / (1 + xt::exp(-tensor));
    }

    //-------------------------------
    // SOFTMAX
    //-------------------------------

    void softmax_1D(float *data, const int size);
    void softmax_2D(float *data, const int num_rows, const int num_cols);
    void softmax_3D(float *data, const int dim1_size, const int dim2_size, const int dim3_size);
    xt::xarray<float> softmax_xtensor(const xt::xarray<float> &scores);

    //-------------------------------
    // FILTERS & NORMALIZATION
    //-------------------------------

    template <typename T>
    xt::xarray<int> top_k(const xt::xarray<T> &data, const int k)
    {
        auto descending_order_array = xt::eval(-data);
        xt::xarray<int> krange = xt::arange<int>(0, k);
        xt::xarray<int> index_array = xt::argpartition(descending_order_array, krange, xt::xnone());
        auto topk_index_array = xt::view(xt::reshape_view(index_array, data.shape()), xt::all(), xt::range(0, k));
        return topk_index_array;
    }

    xt::xarray<float> vector_normalization(const xt::xarray<float> &data);

}

#endif  // _POSTPROCESSES_COMMON_MATH_HPP_
