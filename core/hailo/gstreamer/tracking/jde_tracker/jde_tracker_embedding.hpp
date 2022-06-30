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
#include "strack.hpp"
#include "tracker_macros.hpp"


/**
 * @brief Create a cost matrix based on the features saved
 *        in each STrack. No return, is made, the matrix is
 *        filled in place.
 * 
 * @param tracks  -  std::vector<STrack*>
 *        Pointers to tracked STracks
 *
 * @param detections  -  std::vector<STrack>
 *        The newly detected STracks
 *
 * @param cost_matrix  -  std::vector<std::vector<float>>
 *        The cost matrix to fill in.
 *
 * @param cost_matrix_rows  -  int *
 *        To fill in with number of cost matrix rows.
 *
 * @param cost_matrix_cols  -  int *
 *        To fill in with number of cost matrix columns.
 */
inline void JDETracker::embedding_distance(std::vector<STrack*> &tracks,
                                           std::vector<STrack> &detections,
                                           std::vector<std::vector<float>> &cost_matrix)
{
    if (tracks.size() * detections.size() == 0)
    {
        return;
    }

    for (uint i = 0; i < tracks.size(); i++)
    {
        std::vector<float> cost_matrix_tmp(detections.size());
        std::vector<float> track_feature = tracks[i]->m_smooth_feat;
        for (uint j = 0; j < detections.size(); j++)
        {
            std::vector<float> det_feature = detections[j].m_curr_feat;
            float feat_square = 0.0;
            for (uint k = 0; k < det_feature.size(); k++)
            {
                feat_square += (track_feature[k] - det_feature[k])*(track_feature[k] - det_feature[k]);
            }
            cost_matrix_tmp[j] = std::sqrt(feat_square);
        }
        cost_matrix.push_back(cost_matrix_tmp);
    }
}


/**
 * @brief Update a cost matrix with the gating distance of all STracks.
 *        No returns are made 
 * 
 * @param cost_matrix  -  std::vector<std::vector<float>>
 *        A preliminary cost matrix made by embedding_distance
 *
 * @param tracks  -  std::vector<STrack*>
 *        Pointers to tracked STracks.
 *
 * @param detections  -  std::vector<STrack>
 *        The newly detected STracks.
 *
 * @param lambda_  -  float
 *        How much weight to give the gating distance.
 */
inline void JDETracker::fuse_motion(std::vector<std::vector<float>> &cost_matrix,
                                    std::vector<STrack*> &tracks,
                                    std::vector<STrack> &detections,
                                    float lambda_ = 0.98)
{
    if (cost_matrix.size() == 0)
        return;

    int gating_dim = 4;
    float gating_threshold = this->m_kalman_filter.chi2inv95[gating_dim];

    std::vector<TrackerTypes::DETECTBOX> measurements(detections.size());
    for (uint i = 0; i < detections.size(); i++)
    {
        std::vector<float> tlwh_ = detections[i].to_xyah();
        TrackerTypes::DETECTBOX measurement = {{tlwh_[0], tlwh_[1], tlwh_[2], tlwh_[3]}};
        measurements[i] = measurement;
    }

    for (uint i = 0; i < tracks.size(); i++)
    {
        xt::xarray<float, xt::layout_type::row_major> gating_distance = m_kalman_filter.gating_distance(tracks[i]->m_mean,
                                                                                                        tracks[i]->m_covariance,
                                                                                                        measurements);
        for (uint j = 0; j < cost_matrix[i].size(); j++)
        {
            if (gating_distance[j] > gating_threshold)
            {
                cost_matrix[i][j] = FLT_MAX;
            }
            cost_matrix[i][j] = lambda_ * cost_matrix[i][j] + (1 - lambda_)*gating_distance[j];
        }
    }
}