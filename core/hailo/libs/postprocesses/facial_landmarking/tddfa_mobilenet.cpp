/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <sys/stat.h>

#include "xtensor/xio.hpp"
#include "xtensor/xarray.hpp"
#include "xtensor/xtensor.hpp"
#include "xtensor/xpad.hpp"
#include "xtensor/xshape.hpp"
#include "xtensor/xsort.hpp"
#include "xtensor/xstrided_view.hpp"
#include "xtensor/xview.hpp"
#include "xtensor/xnpy.hpp"
#include "tddfa_mobilenet.hpp"
#include "common/math.hpp"
#include "common/tensors.hpp"
#include "const_tensors.hpp"

const char *output_layer_name = "tddfa_mobilenet_v1/fc1"; // there are 62 params
#define TRANS_DIM (12)
#define SHAPE_DIM (40)
#define OUTPUT_SIZE (68)
#define FACE_HEIGHT (120)
#define FACE_WIDTH (FACE_HEIGHT)

//******************************************************************
// BASEL FACE MODEL CALCULATION
//******************************************************************

/*The Morphable Model is a generative model consisting of a linear 3D shape and appearance model plus an imaging model,
which maps the 3D surface onto an image. The 3D shape and appearance are modeled by taking linear combinations of a
training set of example faces.
The Basel Face Model is a 3D morphable Face model that is publicly available.
*/

bool is_path_exists(const std::string &s)
{
  struct stat buffer;
  return (stat (s.c_str(), &buffer) == 0);
}

std::string get_post_proc_data_dir()
{
    // check if post processes exists in rootfs under /usr/lib
    std::string post_proc_data_dir = "/usr/lib/hailo-post-processes/post_processes_data";
    if(is_path_exists(post_proc_data_dir))
        return post_proc_data_dir;

    // if not - they should exist in the workspace (x86 structure) - take it from the environment variable
    std::string tappas_path = std::getenv("TAPPAS_WORKSPACE");
    if (tappas_path == "")
        throw std::invalid_argument("TAPPAS_WORKSPACE environment variable is not set, cannot find post_processes_data directory");

    return tappas_path + "/apps/h8/gstreamer/libs/post_processes/post_processes_data";
}

std::string post_proc_data_dir = get_post_proc_data_dir();
// read large const arrays from memory
xt::xarray<float> W_EXP_BASE =
  xt::load_npy<float>(post_proc_data_dir + "/w_exp_base.npy");
xt::xarray<float> W_SHP_BASE =
  xt::load_npy<float>(post_proc_data_dir + "/w_shp_base.npy");
xt::xarray<float> trans_bfm_u_base = xt::transpose(bfm_u_base);

xt::xarray<float> calc_params_view(xt::xarray<float> face_3dmm_params)
{
    xt::xarray<float> face_params_view = xt::view(face_3dmm_params, xt::range(0, TRANS_DIM));
    return xt::reshape_view(face_params_view, {3, 4});
}

xt::xarray<float> calc_offset(xt::xarray<float> face_3dmm_params, xt::xarray<float> R_)
{
    xt::xarray<float> offset_ = xt::view(R_, xt::all(), -1);
    xt::xarray<float> offset = xt::reshape_view(offset_, {3, 1});
    return offset;
}

// Compute tensor dot product along specified axes for arrays.
// In this case along axis 1 of the first matrix and axis 0 of the second.
xt::xarray<float> two_dim_dot_product(xt::xarray<float, xt::layout_type::row_major> matrix_1,
                                      xt::xarray<float, xt::layout_type::row_major> matrix_2)
{
    uint axis_length = matrix_1.shape(1);
    if (axis_length != matrix_2.shape(0))
    {
        throw std::invalid_argument("two_dim_dot_product error: axis don't match!");
    }

    float row_sum;
    xt::xarray<float>::shape_type shape = {matrix_1.shape(0), matrix_2.shape(1)};
    xt::xarray<float, xt::layout_type::row_major> product_matrix(shape);
    for (uint i = 0; i < matrix_1.shape(0); ++i)
    {
        for (uint j = 0; j < matrix_2.shape(1); ++j)
        {
            row_sum = 0.0;
            for (uint k = 0; k < axis_length; ++k)
            {
                row_sum += matrix_1(i, k) * matrix_2(k, j);
            }
            product_matrix(i, j) = row_sum;
        }
    }
    return product_matrix;
}

xt::xarray<float> calc_trans_sum(xt::xarray<float> face_3dmm_params)
{
    xt::xarray<float> raw_alpha_shp = xt::view(face_3dmm_params, xt::range(TRANS_DIM, TRANS_DIM + SHAPE_DIM));
    xt::xarray<float> alpha_shape = xt::reshape_view(raw_alpha_shp, {40, 1});

    xt::xarray<float> raw_alpha_exp = xt::view(face_3dmm_params, xt::range(TRANS_DIM + SHAPE_DIM, _));
    xt::xarray<float> alpha_exp = xt::reshape_view(raw_alpha_exp, {10, 1});
    xt::xarray<float> shape_mul = two_dim_dot_product(W_SHP_BASE, alpha_shape);
    xt::xarray<float> exp_mul = two_dim_dot_product(W_EXP_BASE, alpha_exp);

    xt::xarray<float> sum = trans_bfm_u_base + shape_mul + exp_mul;
    xt::xarray<float> reshape_sum = xt::reshape_view(sum, {OUTPUT_SIZE, 3});
    xt::xarray<float> trans_sum = xt::transpose(reshape_sum);
    return trans_sum;
}

xt::xarray<float> BFM(xt::xarray<float> bfm_params_xarray)
{
    // this is the implementation of the Basel face model

    // normalization:
    xt::xarray<float> face_3dmm_params = (bfm_params_xarray * TDDFA_RESCALE_PARAMS_STD) + TDDFA_RESCALE_PARAMS_MEAN;
    xt::xarray<float> face_params_view = calc_params_view(face_3dmm_params);
    xt::xarray<float> offset = calc_offset(face_3dmm_params, face_params_view);
    xt::xarray<float> transposed_sum = calc_trans_sum(face_3dmm_params);
    xt::xarray<float> reshaped_params = xt::view(face_params_view, xt::all(), xt::range(0, 3));
    // perform normal tensor dot operation- one the first tensor iterate the colums ({1}) and
    // on the second tensor iterate the rows ({0})
    xt::xarray<float> mul = two_dim_dot_product(reshaped_params, transposed_sum);
    xt::xarray<float> trans_raw_landmarks = mul + offset;
    return trans_raw_landmarks;
}

//******************************************************************
// FACE LANDMARKS SPECIFIC PARAMETERS
//******************************************************************

xt::xarray<float> calc_bfm_params_xarray(HailoTensorPtr bfm_params)
{
    xt::xarray<uint8_t> bfm_params_xarray = common::get_xtensor(bfm_params);
    xt::xarray<uint8_t> flatten_bfm_params = xt::flatten(bfm_params_xarray);
    auto qp_zp = bfm_params->vstream_info().quant_info.qp_zp;
    auto qp_scale = bfm_params->vstream_info().quant_info.qp_scale;
    xt::xarray<float> bfm_params_dequantize = common::dequantize(flatten_bfm_params, qp_scale, qp_zp);
    return bfm_params_dequantize;
}

xt::xarray<float> calc_landmarks(xt::xarray<float> sum3, int face_height)
{
    xt::xarray<float> landmarks_raw = xt::transpose(sum3);
    // each row consist 3 values which are the x,y,z values of the dots that we will draw.
    // we want to draw only in 2D - so we need only x,y values but not z.
    // cut out the third value in each row
    xt::xarray<float> landmarks_without_z = xt::view(landmarks_raw, xt::all(), xt::range(0, 2));
    // construct empty array it the shape we need and modify it
    xt::xarray<float> landmarks = xt::zeros<float>({OUTPUT_SIZE, 2});
    xt::col(landmarks, 0) = xt::col(landmarks_without_z, 0);
    // the original repo assumes drawing is upside down so here we need to flip it:
    xt::col(landmarks, 1) = face_height - xt::col(landmarks_without_z, 1);
    return landmarks;
}

xt::xarray<float> facial_landmark_postprocess(HailoTensorPtr bfm_params)
{
    xt::xarray<float> bfm_params_xarray = calc_bfm_params_xarray(bfm_params);
    xt::xarray<float> transposed_raw_landmarks = BFM(bfm_params_xarray);
    xt::xarray<float> landmarks = calc_landmarks(transposed_raw_landmarks, FACE_HEIGHT);
    // Make landmarks relative to the face instead of absulute.
    xt::view(landmarks, xt::all(), 0) = xt::view(landmarks, xt::all(), 0) / FACE_WIDTH;
    xt::view(landmarks, xt::all(), 1) = xt::view(landmarks, xt::all(), 1) / FACE_HEIGHT;
    return landmarks;
}

void facial_landmark(HailoROIPtr roi)
{
    std::vector<HailoPoint> points;
    if (roi->has_tensors())
    {
        xt::xarray<float> landmarks = facial_landmark_postprocess(roi->get_tensor(output_layer_name));
        points.reserve(landmarks.shape(0));
        for (uint i = 0; i < landmarks.shape(0); i++)
        {
            points.emplace_back(HailoPoint(landmarks(i, 0), landmarks(i, 1)));
        }
        roi->add_object(std::make_shared<HailoLandmarks>("landmarks", points));
    }
}

void filter(HailoROIPtr roi)
{
    facial_landmark(roi);
}

void facial_landmarks_merged(HailoROIPtr roi)
{
    output_layer_name = "tddfa_mobilenet_v1/fc1";
    facial_landmark(roi);
}

void facial_landmarks_yuy2(HailoROIPtr roi)
{
    output_layer_name = "tddfa_mobilenet_v1_yuy2/fc1";
    facial_landmark(roi);
}
