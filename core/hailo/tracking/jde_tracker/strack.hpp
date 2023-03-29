/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
/*
  Class header for an STrack, a single shared tracklet of a larger tracking class (JDETracker).
  Tracklets refer to an instance of a tracked object, and therefore stracks hold members such as
  track state (new/tracked/lost) or track id (the unique id of the object).
*/

#pragma once

// General cpp includes
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

// Tappas includes
// General includes
#include "hailo_common.hpp"
#include "hailo_objects.hpp"
// Tracker includes
#include "kalman_filter.hpp"
#include "tracker_macros.hpp"

// Open source includes
#include <opencv2/opencv.hpp>
#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"
#include "xtensor/xio.hpp"
#include "xtensor/xmath.hpp"

__BEGIN_DECLS
enum TrackState
{
    New = 0,
    Tracked,
    Lost,
    Removed
};

/*
There are some objects that cannot be consistently scaled to match the new location of the tracked object
(Like landmarks, mask , matrix).
For example, if a face is rotated 90 degrees, the landmarks will not be in the correct location.
These metadata types will never be kept, even if keep_past_metadata is set to true.
*/

class STrack
{
    //******************************************************************
    // CLASS MEMBERS
    //******************************************************************
public:
    // Class members
    bool m_is_activated; // Is activated
    int m_track_id;      // Unique track id
    int m_frame_id;      // Current frame id (used for measuring half-life)
    int m_tracklet_len;  // Number of frames since activation
    float m_confidence;  // Tracklet's score
    int m_start_frame;   // Last activated frame id
    float m_alpha;       // Alpha blending for smoothing features

    std::vector<float> tmp_location_tlwh; // Momentary location (top left, width, height)
    std::vector<float> m_tlwh;            // Rolling top-left, width-height: xmin,ymin,width,height
    std::vector<float> m_curr_feat;       // The current features
    std::vector<float> m_smooth_feat;     // The smoothed features

    TrackerTypes::KAL_MEAN m_mean;
    TrackerTypes::KAL_COVA m_covariance;

private:
    int m_times_seen;
    int m_state;                                           // Current state: can be New, Tracked, or Lost
    KalmanFilter *m_kalman_filter;                         // A Kalman Filter instance to make predictions
    HailoDetectionPtr m_hailo_detection;                   // A shared pointer to the detection object in the pipeline
    std::vector<hailo_object_t> m_hailo_objects_blacklist; // Objects that will never be kept
    bool m_debug;                                          // Debug flag
    //******************************************************************
    // CLASS RESOURCE MANAGEMENT
    //******************************************************************
public:
    // Constructors
    STrack(std::vector<float> tlwh_ = {0., 0., 0., 0.}, float score_ = 0.0, std::vector<float> temp_feat = {0.0},
           HailoDetectionPtr detection_ptr = nullptr, int frame_id = 0,
           std::vector<hailo_object_t> hailo_objects_blacklist = {HAILO_LANDMARKS, HAILO_DEPTH_MASK, HAILO_CLASS_MASK}, bool debug = false) : m_is_activated(false), m_track_id(0), m_frame_id(frame_id), m_tracklet_len(0), m_confidence(score_),
                                                                                                                                              m_start_frame(0), m_alpha(0.9), tmp_location_tlwh(tlwh_), m_state(TrackState::New),
                                                                                                                                              m_hailo_detection(detection_ptr), m_debug(debug)
    {
        m_times_seen = 0;
        // Initialize mean/covariance to zero
        m_mean = xt::zeros<float>({1, 8});
        m_covariance = xt::zeros<float>({8, 8});
        // Initialize the rolling m_tlwh
        m_tlwh.resize(4);
        update_tlwh();
        // Update the features
        update_features(temp_feat);
        m_hailo_objects_blacklist = std::move(hailo_objects_blacklist);
    }

    //******************************************************************
    // TRACKING FUNCTIONS
    //******************************************************************
    /******************** PUBLIC FUNCTIONS ****************************/
public:
    /**
     * @brief Update the HailoDetectionPtr bbox with the strack's m_tlwh
     *
     */
    void update_hailo_bbox()
    {
        if (nullptr != this->m_hailo_detection)
        {
            HailoBBox bbox(this->m_tlwh[0], this->m_tlwh[1], this->m_tlwh[2], this->m_tlwh[3]);
            this->m_hailo_detection->set_bbox(bbox);
        }
    }

    /**
     * @brief Update the HailoDetctionPtr bbox with the strack's m_tlwh
     *
     */
    void update_hailo_detection(HailoDetectionPtr new_detection, bool keep_past_metadata = true)
    {
        for (auto object : this->m_hailo_detection->get_objects())
        {
            hailo_object_t object_type = object->get_type();
            if (object_type == HAILO_UNIQUE_ID)
            {
                new_detection->add_object(object);
            }
            else if (keep_past_metadata)
            {
                // Add the sub object only if its type is not under hailo_objects_blacklist
                if (std::find(m_hailo_objects_blacklist.begin(), m_hailo_objects_blacklist.end(), object_type) == m_hailo_objects_blacklist.end())
                {
                    new_detection->add_unscaled_object(object);
                }
            }
        }

        this->m_hailo_detection = new_detection;
        update_hailo_bbox();
    }

    void add_object(HailoObjectPtr obj)
    {
        this->m_hailo_detection->add_object(obj);
    }

    /**
     * @brief Update the rolling tlwh (xmin,ymin,width,height)
     *        with the momentary tlwh(tmp_location_tlwh).
     */
    void update_tlwh()
    {
        // If this is the first update, then roling tlwh = momentary tlwh
        if ((float)xt::sum(this->m_mean)() == 0.0f)
        {
            m_tlwh[0] = tmp_location_tlwh[0];
            m_tlwh[1] = tmp_location_tlwh[1];
            m_tlwh[2] = tmp_location_tlwh[2];
            m_tlwh[3] = tmp_location_tlwh[3];
            return;
        }

        // If this is not the first time, then take the tlwh from the mean
        m_tlwh[3] = m_mean[3];                   // height
        m_tlwh[2] = m_mean[2] * m_mean[3];       // width = aspect ratio * height
        m_tlwh[1] = m_mean[1] - (m_tlwh[3] / 2); // ymin = center_y - height/2
        m_tlwh[0] = m_mean[0] - (m_tlwh[2] / 2); // xmin = center_x - width/2
    }

    /**
     * @brief Get the rolling tlbr (xmin,ymin,xmax,ymax)
     *
     * @return std::vector<float>
     *         The xmin,ymin,xmax,ymax of this STrack
     */
    std::vector<float> tlbr()
    {
        std::vector<float> tlbr(4);
        tlbr[0] = m_tlwh[0];             // xmin
        tlbr[1] = m_tlwh[1];             // ymin
        tlbr[2] = m_tlwh[0] + m_tlwh[2]; // xmax = xmin + width
        tlbr[3] = m_tlwh[1] + m_tlwh[3]; // ymax = ymin + height
        return tlbr;
    }

    /**
     * @brief Convert tlwh (xmin,ymin,width,height) to (center x, center y, aspect ratio, height),
     *        where aspect ratio is width / height
     *
     * @param tlwh_tmp  -  std::vector<float>
     *        The tlwh to convert.
     *
     * @return std::vector<float>
     *         The center x, center y, aspect ratio, height.
     */
    static std::vector<float> tlwh_to_xyah(std::vector<float> tlwh_tmp)
    {
        std::vector<float> tlwh_output = tlwh_tmp;
        tlwh_output[0] += tlwh_output[2] / 2;
        tlwh_output[1] += tlwh_output[3] / 2;
        tlwh_output[2] /= tlwh_output[3];
        return tlwh_output;
    }

    /**
     * @brief Get bounding box in (center x, center y, aspect ratio, height) format
     *
     * @return std::vector<float>
     *         The center x, center y, aspect ratio, height.
     */
    std::vector<float> to_xyah()
    {
        return STrack::tlwh_to_xyah(m_tlwh);
    }

    /**
     * @brief Convert tlbr(xmin,ymin,xmax,ymax) to tlwh(xmin,ymin,width,height)
     *
     * @param tlbr  -  std::vector<float>
     *        The tlbr to convert
     *
     * @return std::vector<float>
     */
    static std::vector<float> tlbr_to_tlwh(std::vector<float> &tlbr)
    {
        tlbr[2] -= tlbr[0];
        tlbr[3] -= tlbr[1];
        return tlbr;
    }

    /**
     * @brief Get the detectbox from tlwh object
     *
     * @param tlwh_tmp  -  std::vector<float>
     *        The tlwh (xmin,ymin,width,height)
     *
     * @return TrackerTypes::DETECTBOX
     *         The converted xyah (center x, center y, aspect ratio, height)
     */
    static TrackerTypes::DETECTBOX get_detectbox_from_tlwh(std::vector<float> tlwh)
    {
        std::vector<float> xyah = STrack::tlwh_to_xyah(tlwh);
        TrackerTypes::DETECTBOX xyah_box = {{xyah[0], xyah[1], xyah[2], xyah[3]}};
        return xyah_box;
    }

    // Mark state to tracked
    void mark_tracked() { m_state = TrackState::Tracked; }

    // Mark state to lost
    void mark_lost() { m_state = TrackState::Lost; }

    // Mark state to removed
    void mark_removed() { m_state = TrackState::Removed; }

    // Get the state
    int get_state() { return m_state; }

    // Get debug
    bool get_debug() { return m_debug; }

    // Get the hailo detection object
    HailoDetectionPtr get_hailo_detection() { return this->m_hailo_detection; }

    /**
     * @brief Get the next available id
     *
     * @return int
     *         The id
     */
    int next_id()
    {
        static int _count = 0;
        _count++;
        _count = _count % 100000; // Cycle ids after 100000
        return _count;
    }

    /**
     * @brief Get the ending frame id
     *
     * @return int
     *         The ending frame id
     */
    int end_frame()
    {
        return this->m_frame_id;
    }

    /**
     * @brief Run Kalman filter prediction step on all given STracks
     *
     * @param stracks  -  std::vector<STrack *>
     *        A set of STracks to run predictions on.
     *
     * @param kalman_filter  -  KalmanFilter
     *        The kalman filter with which to make the predictions.
     */
    static void multi_predict(std::vector<STrack *> &stracks, KalmanFilter &kalman_filter)
    {
        for (uint i = 0; i < stracks.size(); i++)
        {
            if (stracks[i]->m_state != TrackState::Tracked)
            {
                stracks[i]->m_mean(7) = 0;
            }
            kalman_filter.predict(stracks[i]->m_mean, stracks[i]->m_covariance);
        }
    }

    /**
     * @brief Activate the Strack, setting a starting frame id,
     *        updating the position, and initiating the mean/covariance
     *        using a kalman filter.
     *        Sets the state to Tracked
     *
     * @param kalman_filter  -  KalmanFilter
     *        A kalman filter with which to initiate the mean/covariance
     *
     * @param frame_id  -  int
     *        The current frame it, this becomes the "starting" frame id.
     */
    void activate(KalmanFilter *kalman_filter, int frame_id)
    {
        this->m_kalman_filter = kalman_filter;
        this->m_track_id = this->next_id();

        HailoUniqueIDPtr object_id = std::make_shared<HailoUniqueID>(HailoUniqueID(this->m_track_id));
        this->m_hailo_detection->add_object(object_id);

        TrackerTypes::DETECTBOX xyah_box = STrack::get_detectbox_from_tlwh(this->tmp_location_tlwh);

        auto mc = this->m_kalman_filter->initiate(xyah_box);
        this->m_mean = mc.first;
        this->m_covariance = mc.second;

        update_tlwh();

        this->m_tracklet_len = 0;
        this->m_state = TrackState::Tracked;
        this->m_frame_id = frame_id;
        this->m_start_frame = frame_id;
        update_hailo_bbox();
    }

    /**
     * @brief Re-activates this STrack using a given STrack.
     *        Updates this STrack's mean/covariance using the given STrack's location,
     *        and resets the starting frame / unique id.
     *        Sets the state to Tracked
     *
     * @param new_track  -  STrack
     *        The tracklet by which to update this one.
     *
     * @param frame_id  -  int
     *        The current frame id
     *
     * @param new_id  -  bool
     *        If to update this unique id
     */
    void re_activate(STrack &new_track, int frame_id, bool new_id, bool keep_past_metadata)
    {
        TrackerTypes::DETECTBOX xyah_box = STrack::get_detectbox_from_tlwh(new_track.m_tlwh);

        auto mc = this->m_kalman_filter->update(this->m_mean, this->m_covariance, xyah_box);
        this->m_mean = mc.first;
        this->m_covariance = mc.second;

        update_tlwh();

        update_features(new_track.m_curr_feat);
        this->m_tracklet_len = 0;
        this->m_state = TrackState::Tracked;
        this->m_is_activated = true;
        this->m_frame_id = frame_id;
        if (new_id)
            this->m_track_id = next_id();

        // Update the HailoDetectionPtr to the new track and update it's id
        update_hailo_detection(new_track.get_hailo_detection(), keep_past_metadata);
    }

    /**
     * @brief Update this STrack using a matched STrack.
     *        Updates this STrack's mean/covariance using the given STrack's location,
     *        and resets the starting frame / unique id.
     *        Sets the state to Tracked
     *
     * @param new_track  -  STrack
     *        The tracklet by which to update this one.
     *
     * @param frame_id  -  int
     *        The current frame id
     *
     * @param update_feature  -  bool
     *        If to update this STrack's features
     */
    void update(STrack &new_track, int frame_id, bool keep_past_metadata = true, bool update_feature = true)
    {
        this->m_frame_id = frame_id;
        this->m_tracklet_len++;

        TrackerTypes::DETECTBOX xyah_box = STrack::get_detectbox_from_tlwh(new_track.m_tlwh);

        auto mc = this->m_kalman_filter->update(this->m_mean, this->m_covariance, xyah_box);
        this->m_mean = mc.first;
        this->m_covariance = mc.second;

        update_tlwh();

        this->m_state = TrackState::Tracked;
        this->m_is_activated = true;

        this->m_confidence = new_track.m_confidence;
        if (update_feature)
            update_features(new_track.m_curr_feat);

        // Update the HailoDetectionPtr to the new track and update it's id
        update_hailo_detection(new_track.get_hailo_detection(), keep_past_metadata);
    }

    /******************** PRIVATE FUNCTIONS ****************************/
private:
    /**
     * @brief Update the tracklet's features and applies smoothing.
     *
     * @param feat  -  std::vector<float>
     *        The features to update
     */
    void update_features(std::vector<float> feat)
    {
        cv::Mat feat_mat(feat);
        float feat_value = cv::norm(feat_mat);
        for (uint i = 0; i < feat.size(); ++i)
        {
            feat[i] /= feat_value;
        }
        this->m_curr_feat.assign(feat.begin(), feat.end());
        if (this->m_smooth_feat.size() == 0)
        {
            this->m_smooth_feat.assign(feat.begin(), feat.end());
        }
        else
        {
            for (uint i = 0; i < this->m_smooth_feat.size(); ++i)
            {
                this->m_smooth_feat[i] = m_alpha * this->m_smooth_feat[i] + (1 - m_alpha) * feat[i];
            }
        }

        cv::Mat smooth_feat_mat(this->m_smooth_feat);
        float smmoth_feat_value = cv::norm(smooth_feat_mat);
        for (uint i = 0; i < this->m_smooth_feat.size(); ++i)
        {
            this->m_smooth_feat[i] /= smmoth_feat_value;
        }
    }
};
__END_DECLS