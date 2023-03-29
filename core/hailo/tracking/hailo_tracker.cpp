/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/

// Tracker Includes
#include "jde_tracker/jde_tracker.hpp"

#include "hailo_tracker.hpp"
#include "hailo_common.hpp"

std::mutex HailoTracker::mutex_;

class HailoTracker::HailoTrackerPrivate
{
public:
    std::map<std::string, JDETracker> trackers;
};

HailoTracker::HailoTracker() : priv(std::make_unique<HailoTrackerPrivate>()){};
HailoTracker::~HailoTracker(){};
HailoTracker &HailoTracker::GetInstance()
{
    std::lock_guard<std::mutex> lock(mutex_);
    static HailoTracker instance;
    return instance;
}

void HailoTracker::remove_jde_tracker(const std::string &name)
{
    std::lock_guard<std::mutex> lock(mutex_);
    priv->trackers.erase(name);
}

void HailoTracker::add_jde_tracker(const std::string &name, HailoTrackerParams tracker_params)
{
    std::lock_guard<std::mutex> lock(mutex_);

    priv->trackers.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                           std::forward_as_tuple(tracker_params.kalman_distance,
                                                 tracker_params.iou_threshold,
                                                 tracker_params.init_iou_threshold,
                                                 tracker_params.keep_tracked_frames,
                                                 tracker_params.keep_new_frames,
                                                 tracker_params.keep_lost_frames,
                                                 tracker_params.keep_past_metadata,
                                                 tracker_params.std_weight_position,
                                                 tracker_params.std_weight_position_box,
                                                 tracker_params.std_weight_velocity,
                                                 tracker_params.std_weight_velocity_box,
                                                 tracker_params.debug,
                                                 tracker_params.hailo_objects_blacklist));
}

void HailoTracker::add_jde_tracker(const std::string &name)
{
    std::lock_guard<std::mutex> lock(mutex_);
    priv->trackers.emplace(name, JDETracker());
}

std::vector<HailoDetectionPtr> HailoTracker::update(const std::string &name, std::vector<HailoDetectionPtr> &inputs)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto online_stracks = priv->trackers[name].update(inputs);
    bool debug = priv->trackers[name].get_debug();
    return JDETracker::stracks_to_hailo_detections(online_stracks, debug);
}

void HailoTracker::add_object_to_track(const std::string &name, int track_id, HailoObjectPtr obj)
{
    std::lock_guard<std::mutex> lock(mutex_);
    STrack *tracked_detection = priv->trackers[name].get_detection_with_id(track_id);
    if (nullptr != tracked_detection)
    {
        tracked_detection->add_object(obj);
    }
}

void HailoTracker::remove_matrices_from_track(const std::string &name, int track_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    STrack *tracked_detection = priv->trackers[name].get_detection_with_id(track_id);
    if (tracked_detection)
    {
        std::vector<HailoObjectPtr> matrices;
        auto detection = tracked_detection->get_hailo_detection();
        for (auto obj : detection->get_objects_typed(HAILO_MATRIX))
        {
            HailoMatrixPtr matrix = std::dynamic_pointer_cast<HailoMatrix>(obj);
            matrices.push_back(matrix);
        }
        hailo_common::remove_objects(detection, matrices);
    }
}

void HailoTracker::remove_classifications_from_track(const std::string &name, int track_id, std::string classifier_type)
{
    std::lock_guard<std::mutex> lock(mutex_);
    STrack *tracked_detection = priv->trackers[name].get_detection_with_id(track_id);
    if (tracked_detection)
    {
        hailo_common::remove_classifications(tracked_detection->get_hailo_detection(), classifier_type);
    }
}

// Setters for members accessible at element-property level
void HailoTracker::set_kalman_distance(const std::string &name, float new_distance)
{
    std::lock_guard<std::mutex> lock(mutex_);
    priv->trackers[name].set_kalman_distance(new_distance);
}
void HailoTracker::set_iou_threshold(const std::string &name, float new_iou_thr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    priv->trackers[name].set_iou_threshold(new_iou_thr);
}
void HailoTracker::set_init_iou_threshold(const std::string &name, float new_init_iou_thr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    priv->trackers[name].set_init_iou_threshold(new_init_iou_thr);
}
void HailoTracker::set_keep_tracked_frames(const std::string &name, int new_keep_tracked)
{
    std::lock_guard<std::mutex> lock(mutex_);
    priv->trackers[name].set_keep_tracked_frames(new_keep_tracked);
}
void HailoTracker::set_keep_new_frames(const std::string &name, int new_keep_new)
{
    std::lock_guard<std::mutex> lock(mutex_);
    priv->trackers[name].set_keep_new_frames(new_keep_new);
}
void HailoTracker::set_keep_lost_frames(const std::string &name, int new_keep_lost)
{
    std::lock_guard<std::mutex> lock(mutex_);
    priv->trackers[name].set_keep_lost_frames(new_keep_lost);
}
void HailoTracker::set_keep_past_metadata(const std::string &name, bool new_keep_past)
{
    std::lock_guard<std::mutex> lock(mutex_);
    priv->trackers[name].set_keep_past_metadata(new_keep_past);
}
void HailoTracker::set_std_weight_position(const std::string &name, float new_std_weight_pos)
{
    std::lock_guard<std::mutex> lock(mutex_);
    priv->trackers[name].set_std_weight_position(new_std_weight_pos);
}
void HailoTracker::set_std_weight_position_box(const std::string &name, float new_std_weight_position_box)
{
    std::lock_guard<std::mutex> lock(mutex_);
    priv->trackers[name].set_std_weight_position_box(new_std_weight_position_box);
}
void HailoTracker::set_std_weight_velocity(const std::string &name, float new_std_weight_vel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    priv->trackers[name].set_std_weight_velocity(new_std_weight_vel);
}
void HailoTracker::set_std_weight_velocity_box(const std::string &name, float new_std_weight_velocity_box)
{
    std::lock_guard<std::mutex> lock(mutex_);
    priv->trackers[name].set_std_weight_velocity_box(new_std_weight_velocity_box);
}
void HailoTracker::set_debug(const std::string &name, bool new_debug)
{
    std::lock_guard<std::mutex> lock(mutex_);
    priv->trackers[name].set_debug(new_debug);
}

void HailoTracker::set_hailo_objects_blacklist(const std::string &name, std::vector<hailo_object_t> hailo_objects_blacklist_vec)
{
    std::lock_guard<std::mutex> lock(mutex_);
    priv->trackers[name].set_hailo_objects_blacklist(hailo_objects_blacklist_vec);
}