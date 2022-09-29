/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#pragma once

// General cpp includes
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// Tappas includes
#include "hailo_objects.hpp"
#include "kalman_filter.hpp"
#include "strack.hpp"
#include "tracker_macros.hpp"

/**
 * @brief Convert HailoDetectionPtr into STracks
 *
 * @param inputs  -  std::vector<HailoDetectionPtr>
 *        A vector of new detections.
 *
 * @return std::vector<STrack>
 *         The translated Stracks.
 */
inline std::vector<STrack> JDETracker::hailo_detections_to_stracks(std::vector<HailoDetectionPtr> &inputs, int frame_id = 0)
{
    std::vector<STrack> detections(inputs.size());
    for (uint i = 0; i < inputs.size(); i++)
    {
        HailoBBox bbox = inputs[i]->get_bbox();
        std::vector<float> detection_box = {bbox.xmin(), bbox.ymin(), bbox.width(), bbox.height()};
        STrack strack(detection_box, inputs[i]->get_confidence(), {}, inputs[i], frame_id);
        detections[i] = strack;
    }

    return detections;
}

/**
 * @brief Convert a vector of Stracks to a vector of HailoDetectionPtr
 *
 * @param stracks  -  std::vector<STrack>
 *        A vector of stracks.
 *
 * @return std::vector<HailoDetectionPtr>
 *         The translated HailoDetectionPtr.
 *
 */
inline std::vector<HailoDetectionPtr> JDETracker::stracks_to_hailo_detections(std::vector<STrack> &stracks)
{
    std::vector<HailoDetectionPtr> objects;
    objects.reserve(stracks.size());
    for (uint i = 0; i < stracks.size(); i++)
    {
        HailoDetectionPtr detection_ptr = stracks[i].get_hailo_detection();
        if (nullptr != detection_ptr)
        {
            objects.emplace_back(detection_ptr);
        }
        else
        {
            // Strack tlwh is stored as top-left, width-height: xmin,ymin,width,height
            HailoBBox bbox(stracks[i].m_tlwh[0], stracks[i].m_tlwh[1], stracks[i].m_tlwh[2], stracks[i].m_tlwh[3]);
            // HailoDetection is constructed as HailoDetection(HailoBBox, label, confidence)
            objects.emplace_back(std::make_shared<HailoDetection>(HailoDetection(bbox, "tracked", stracks[i].m_confidence)));
        }
    }

    return objects;
}

inline STrack *JDETracker::get_detection_with_id(int target_track_id)
{
    for (uint i = 0; i < m_tracked_stracks.size(); i++)
    {
        if (target_track_id == m_tracked_stracks[i].m_track_id)
        {
            return &m_tracked_stracks[i];
        }
    }
    return nullptr;
}

inline std::vector<STrack> JDETracker::get_tracked_stracks()
{
    return m_tracked_stracks;
}
