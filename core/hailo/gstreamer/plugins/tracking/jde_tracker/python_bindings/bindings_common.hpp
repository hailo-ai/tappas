/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/*
  Common Pybind API assets
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
#include "jde_tracker.hpp"
#include "strack.hpp"
#include "kalman_filter.hpp"

// Open source includes
#include "pybind11/pybind11.h"
#include "pybind11/numpy.h"
#include "pybind11/detail/common.h"
#include "pybind11/stl.h"
#include "pybind11/stl_bind.h"
#include "pybind11/complex.h"
#include "pybind11/functional.h"

#define HAILOTAPPASAPI __attribute__ ((visibility ("default")))

namespace py = pybind11;
using namespace pybind11::literals;

__BEGIN_DECLS

/**
 * @brief Converts a 1D numpy array to an std::vector of floats.
 * 
 * @param input  -  py::array_t<float>
 *        A 1D numpy array to convert.
 *
 * @return std::vector<float> 
 *         The converted std::vector
 */
HAILOTAPPASAPI std::vector<float> numpy_to_float_vector(py::array_t<float, py::array::c_style | py::array::forcecast> input)
{
    py::buffer_info buffer = input.request();

    if (buffer.ndim != 1)
    {
        throw std::runtime_error("numpy.ndarray dims must be 1!");
    }

    //Pointer reads and writes numpy.ndarray
    float* ptr1 = (float*)buffer.ptr;

    std::vector<float> converted_vector(buffer.shape[0]);

    for (int i = 0; i < buffer.shape[0]; i++)
    {
        converted_vector[i] = ptr1[i];
    }

    return converted_vector;
}

/**
 * @brief Converts a 2d numpy array input into a
 *        vector of NewHailoDetectionPtr
 * 
 * @param input  - py::array_t<float>
 *        A numpy array of detections, in the format
 *        [xmin,ymin,xmax,ymax,confidence].
 *
 * @return std::vector<NewHailoDetectionPtr>
 *         
 */
HAILOTAPPASAPI std::vector<NewHailoDetectionPtr> numpy_to_detections(py::array_t<float, py::array::c_style | py::array::forcecast> input)
{
    py::buffer_info buffer = input.request();

    if (buffer.ndim != 2)
    {
        throw std::runtime_error("numpy.ndarray dims must be 2!");
    }

    std::vector<NewHailoDetectionPtr> converted_detections(buffer.shape[0]);
    float xmin, ymin, xmax, ymax, confidence;

    //Pointer reads and writes numpy.ndarray
    float* ptr1 = (float*)buffer.ptr;

    for (int i = 0; i < buffer.shape[0]; i++)
    {
        xmin = ptr1[(i * (int)buffer.shape[1]) + 0];
        ymin = ptr1[(i * (int)buffer.shape[1]) + 1];
        xmax = ptr1[(i * (int)buffer.shape[1]) + 2];
        ymax = ptr1[(i * (int)buffer.shape[1]) + 3];
        confidence = ptr1[(i * (int)buffer.shape[1]) + 4];
        // Strack tlwh is stored as top-left, width-height: xmin,ymin,width,height
        HailoBBox bbox(xmin, ymin, xmax - xmin, ymax - ymin);
        // NewHailoDetection is constructed as NewHailoDetection(HailoBBox, label, confidence)
        converted_detections[i] = std::make_shared<NewHailoDetection>(NewHailoDetection(bbox, "detection", confidence));
    }
    return converted_detections;
}

__END_DECLS