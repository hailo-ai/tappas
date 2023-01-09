/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/*
  Pybind API for JDE Tracker class
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
// Binding includes
#include "bindings_common.hpp"
#include "strack_pybind.hpp"

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


class JDETrackerWrapper {
    public:
    // Constructor
    JDETrackerWrapper(std::unique_ptr<JDETracker> &&tracker) : m_jde_tracker(std::move(tracker))
    {}

    // Proxy Constructor
    static JDETrackerWrapper create(const float kalman_dist, const float iou_thr, const float init_iou_thr,
                                    const int keep_tracked, const int keep_new, const int keep_lost,
                                    float std_weight_position, float std_weight_pos_box, float std_weight_velocity, float std_weight_vel_box)
    {
        auto jde_tracker = std::make_unique<JDETracker>(JDETracker(kalman_dist, iou_thr, init_iou_thr,
                                                                   keep_tracked, keep_new, keep_lost,
                                                                   std_weight_position, std_weight_pos_box, std_weight_velocity, std_weight_vel_box));
        return JDETrackerWrapper(std::move(jde_tracker));
    }

    // Setters for members access
    void set_kalman_distance(float new_distance) { m_jde_tracker->set_kalman_distance(new_distance); }
    void set_iou_threshold(float new_iou_thr) { m_jde_tracker->set_iou_threshold(new_iou_thr); }
    void set_init_iou_threshold(float new_init_iou_thr) { m_jde_tracker->set_init_iou_threshold(new_init_iou_thr); }
    void set_keep_tracked_frames(int new_keep_tracked) { m_jde_tracker->set_keep_tracked_frames(new_keep_tracked); }
    void set_keep_new_frames(int new_keep_new) { m_jde_tracker->set_keep_new_frames(new_keep_new); }
    void set_keep_lost_frames(int new_keep_lost) { m_jde_tracker->set_keep_lost_frames(new_keep_lost); }

    // Getters for members access
    float get_kalman_distance() { return m_jde_tracker->get_kalman_distance(); }
    float get_iou_threshold() { return m_jde_tracker->get_iou_threshold(); }
    float get_init_iou_threshold() { return m_jde_tracker->get_init_iou_threshold(); }
    float get_keep_tracked_frames() { return m_jde_tracker->get_keep_tracked_frames(); }
    float get_keep_new_frames() { return m_jde_tracker->get_keep_new_frames(); }
    float get_keep_lost_frames() { return m_jde_tracker->get_keep_lost_frames(); }

    /**
     * @brief Wrap the update() function of the JDETracker
     *        class, which updates the tracker with new detections.
     * 
     * @param input_detections  -  py::array_t<float>
     *        A numpy array of detections, each entry
     *        formatted to [xmin,ymin,xmax,ymax,confidence]
     *
     * @param report_unconfirmed  -  bool
     *        If true, then report unconfirmed(new) stracks
     *        along with the tracked stracks.
     *
     * @return py::list 
     *         A python list of STrackWrapper
     */
    py::list update(py::array_t<float, py::array::c_style | py::array::forcecast> input_detections, bool report_unconfirmed)
    {
        std::vector<HailoDetectionPtr> converted_detections = numpy_to_detections(input_detections);
        std::vector<STrack> online_stracks = m_jde_tracker->update(converted_detections, report_unconfirmed);
        std::vector<STrackWrapper> output_pystracks(online_stracks.size());
        for (uint i = 0; i < online_stracks.size(); i++)
        {
            auto strack = std::make_unique<STrack>(online_stracks[i]);
            output_pystracks[i] = STrackWrapper(std::move(strack));
        }
        return py::cast(std::move(output_pystracks));
    }

    private:
        std::unique_ptr<JDETracker> m_jde_tracker;
};


void JDETracker_api_initialize_python_module(py::module &m)
{
    py::class_<JDETrackerWrapper>(m, "JDETracker")
        .def(py::init(&JDETrackerWrapper::create),
             "kalman_dist"_a=0.7, "iou_thr"_a=0.8, "init_iou_thr"_a=0.9,
             "keep_tracked"_a=2, "keep_new"_a=2, "keep_lost"_a=2,
             "std_weight_position"_a=0.01, "std_weight_pos_box"_a=0.01, "std_weight_velocity"_a=0.001, "std_weight_vel_box"_a=0.001)
        .def("getKalmanDistance", &JDETrackerWrapper::get_kalman_distance)
        .def("setKalmanDistance", &JDETrackerWrapper::set_kalman_distance)
        .def("getIouThreshold", &JDETrackerWrapper::get_iou_threshold)
        .def("setIouThreshold", &JDETrackerWrapper::set_iou_threshold)
        .def("getInitIouThreshold", &JDETrackerWrapper::get_init_iou_threshold)
        .def("setInitIouThreshold", &JDETrackerWrapper::set_init_iou_threshold)
        .def("getKeepTrackedFrames", &JDETrackerWrapper::get_keep_tracked_frames)
        .def("setKeepTrackedFrames", &JDETrackerWrapper::set_keep_tracked_frames)
        .def("getKeepNewFrames", &JDETrackerWrapper::get_keep_new_frames)
        .def("setKeepNewFrames", &JDETrackerWrapper::set_keep_new_frames)
        .def("getKeepLostFrames", &JDETrackerWrapper::get_keep_lost_frames)
        .def("setKeepLostFrames", &JDETrackerWrapper::set_keep_lost_frames)
        .def("update", &JDETrackerWrapper::update,
             "input_detections"_a=nullptr, "report_unconfirmed"_a=false);
}