/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/*
  Macros and type aliases for JDE Tracker and related classes
*/

#pragma once

#include <utility>
#include "xtensor/xarray.hpp"
#include "xtensor/xfixed.hpp"
#include "xtensor/xtensor.hpp"

namespace TrackerTypes
{
    typedef xt::xtensor_fixed<float, xt::xshape<1, 4>, xt::layout_type::row_major> DETECTBOX;
    typedef xt::xarray<float, xt::layout_type::row_major> DETECTBOXSS;
    typedef xt::xtensor_fixed<float, xt::xshape<1, 128>, xt::layout_type::row_major> FEATURE;

    //Kalman Filter Macros
    typedef xt::xtensor_fixed<float, xt::xshape<1, 8>, xt::layout_type::row_major> KAL_MEAN;
    typedef xt::xtensor_fixed<float, xt::xshape<8, 8>, xt::layout_type::row_major> KAL_COVA;
    typedef xt::xtensor_fixed<float, xt::xshape<1, 4>, xt::layout_type::row_major> KAL_HMEAN;
    typedef xt::xtensor_fixed<float, xt::xshape<4, 4>, xt::layout_type::row_major> KAL_HCOVA;
    typedef std::pair<KAL_MEAN, KAL_COVA> KAL_DATA;
    typedef std::pair<KAL_HMEAN, KAL_HCOVA> KAL_HDATA;
}