/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#pragma once

// General cpp includes
#include <iostream>
#include <vector>
#include <mutex>
#include <thread>
#include <map>

#include "hailo_objects.hpp"

#define DEFAULT_KALMAN_DISTANCE (0.7f)
#define DEFAULT_IOU_THRESHOLD (0.8f)
#define DEFAULT_INIT_IOU_THRESHOLD (0.9f)
#define DEFAULT_KEEP_FRAMES (2)
#define DEFAULT_KEEP_PAST_METADATA (true)

struct HailoTrackerParams
{
    float kalman_distance;
    float iou_threshold;
    float init_iou_threshold;
    int keep_tracked_frames;
    int keep_new_frames;
    int keep_lost_frames;
    bool keep_past_metadata;
};

class HailoTracker
{
private:
    class HailoTrackerPrivate;
    std::unique_ptr<HailoTrackerPrivate> priv;
    HailoTracker(const HailoTracker &) = delete;
    HailoTracker &operator=(const HailoTracker &) = delete;
    ~HailoTracker();
    HailoTracker();
    static std::mutex mutex_;

public:
    static HailoTracker &GetInstance();
    void add_jde_tracker(std::string name, HailoTrackerParams params);
    void add_jde_tracker(std::string name);
    void remove_jde_tracker(std::string name);
    std::vector<HailoDetectionPtr> update(std::string name, std::vector<HailoDetectionPtr> &inputs);
    void add_object_to_track(std::string name, int id, HailoObjectPtr obj);
    void remove_classifications_from_track(std::string name, int track_id, std::string classifier_type);
    void remove_matrices_from_track(std::string name, int track_id);

    // Setters for members accessible at element-property level
    void set_kalman_distance(std::string name, float new_distance);
    void set_iou_threshold(std::string name, float new_iou_thr);
    void set_init_iou_threshold(std::string name, float new_init_iou_thr);
    void set_keep_tracked_frames(std::string name, int new_keep_tracked);
    void set_keep_new_frames(std::string name, int new_keep_new);
    void set_keep_lost_frames(std::string name, int new_keep_lost);
    void set_keep_past_metadata(std::string name, bool new_keep_past);
};
