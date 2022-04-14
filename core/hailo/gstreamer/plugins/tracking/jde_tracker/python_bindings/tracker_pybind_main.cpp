/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/*
  Pybind API for tracking classes
*/

// General cpp includes
#include <iostream>
#include <stdexcept>
#include <string>

// Tappas includes
// General includes
#include "hailo_common.hpp"
#include "hailo_objects.hpp"
// Tracker includes
#include "jde_tracker.hpp"
#include "strack.hpp"
#include "kalman_filter.hpp"
// Binding includes
#include "bindings_common.hpp"
#include "kalman_filter_pybind.hpp"
#include "strack_pybind.hpp"
#include "jde_tracker_pybind.hpp"

// Open source includes
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"
#include "pybind11/stl.h"
#include "pybind11/functional.h"

namespace py = pybind11;
using namespace pybind11::literals;


PYBIND11_MODULE(pyhailotracker, m) {
    m.doc() = "Python Binding for Tappas Tracker Classes";

    KalmanFilter_api_initialize_python_module(m);
    STrack_api_initialize_python_module(m);
    JDETracker_api_initialize_python_module(m);
}