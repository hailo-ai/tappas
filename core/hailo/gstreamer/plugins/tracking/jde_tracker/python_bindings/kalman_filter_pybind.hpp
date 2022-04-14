/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/*
  Pybind API for Kalman Filter class
*/

#pragma once

// General cpp includes
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

// Tappas includes
// General includes
#include "hailo_common.hpp"
#include "hailo_objects.hpp"
// Tracker includes
#include "kalman_filter.hpp"
// Binding includes
#include "bindings_common.hpp"

// Open source includes
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"
#include "pybind11/detail/common.h"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"
#include "pybind11/complex.h"
#include "pybind11/functional.h"

namespace py = pybind11;
using namespace pybind11::literals;

class KalmanFilterWrapper {
    public:
    // Constructor
    KalmanFilterWrapper(std::unique_ptr<KalmanFilter> &&filter) : m_kalman_filter(std::move(filter))
    {}

    // Proxy Constructor
    static KalmanFilterWrapper create()
    {
        auto kalman_filter = std::make_unique<KalmanFilter>(KalmanFilter());
        return KalmanFilterWrapper(std::move(kalman_filter));
    }
    
    private:
        std::unique_ptr<KalmanFilter> m_kalman_filter;
};

void KalmanFilter_api_initialize_python_module(py::module &m)
{
    py::class_<KalmanFilterWrapper>(m, "KalmanFilter")
        .def(py::init(&KalmanFilterWrapper::create));
}