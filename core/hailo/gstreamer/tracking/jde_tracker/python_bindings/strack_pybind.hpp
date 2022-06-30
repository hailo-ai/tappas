/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/*
  Pybind API for STrack class
*/

#pragma once

// General cpp includes
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
#include "strack.hpp"
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

class STrackWrapper {
    public:
    // Default Constructor
    STrackWrapper() : m_strack(nullptr)
    {}

    // Constructor
    STrackWrapper(std::unique_ptr<STrack> &&strack) : m_strack(std::move(strack))
    {}

    // Proxy Constructor
    static STrackWrapper create(const py::array_t<float, py::array::c_style | py::array::forcecast> input_tlwh,
                  const float input_confidence,
                  const py::array_t<float, py::array::c_style | py::array::forcecast> input_features)
    {
        std::vector<float> tlwh = numpy_to_float_vector(input_tlwh);
        std::vector<float> features = numpy_to_float_vector(input_features);
        auto strack = std::make_unique<STrack>(STrack(tlwh, 0.9, features));
        return STrackWrapper(std::move(strack));
    }

    int track_id()
    {
        return m_strack->m_track_id;
    }

    int get_state()
    {
        return m_strack->get_state();
    }

    float get_confidence()
    {
        return m_strack->m_confidence;
    }

    py::array_t<float> tlwh()
    {
        py::array_t<float> casted_tlwh = py::array(m_strack->m_tlwh.size(), m_strack->m_tlwh.data());
        return casted_tlwh;
    }

    private:
        std::unique_ptr<STrack> m_strack;
};

void STrack_api_initialize_python_module(py::module &m)
{
    py::enum_<TrackState>(m, "TrackState")
        .value("New", New)
        .value("Tracked", Tracked)
        .value("Lost", Lost)
        .value("Removed", Removed);

    py::class_<STrackWrapper>(m, "STrack")
        .def(py::init(&STrackWrapper::create))
        .def_property_readonly("track_id", &STrackWrapper::track_id)
        .def_property_readonly("state", &STrackWrapper::get_state)
        .def_property_readonly("confidence", &STrackWrapper::get_confidence)
        .def_property_readonly("tlwh", &STrackWrapper::tlwh);
}