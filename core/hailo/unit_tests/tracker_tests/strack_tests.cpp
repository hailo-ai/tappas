/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
// Catch2 includes
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"       // This includes the catch2 header-only library, no further includes needed for catch2

// General cpp includes
#include <iostream>
#include <string>
#include <vector>

// Tappas includes
#include "kalman_filter.hpp"
#include "strack.hpp"
#include "tracker_macros.hpp"
#include "common/common.hpp"

// Open source includes
#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"
#include "xtensor/xmath.hpp"
#include "xtensor/xview.hpp"

//******************************************************************
//  CONSTRUCTION TESTS
//******************************************************************
/**
 * @brief Unit test case for STrack construction
 * 
 */
TEST_CASE( "STracks are constructed with a bounding box, confidence, and features.", "[strack_construction]" ) {

    // Create a set of standard inputs, taken from adk yolo dataset
    std::vector<float> tlwh = {103.533936, 546.8082, 75.665934, 247.4281};
    float confidence = 0.87615836;
    std::vector<float> no_features = {};
    std::vector<float> one_feature = {1.0};
    std::vector<float> two_features = {1.5, 4.0};

    SECTION( "Given no features, there should not be any smoothing in the tracklet." ) {
        // Create a tracklet to test
        STrack tracklet = STrack(tlwh, confidence, no_features);
        REQUIRE( tracklet.m_smooth_feat.size() == 0);
    }

    SECTION( "Given one feature, the tracklet should smooth to the same value." ) {
        // Create a tracklet to test
        STrack tracklet = STrack(tlwh, confidence, one_feature);
        REQUIRE( tracklet.m_smooth_feat.size() == 1);
        CHECK_THAT( tracklet.m_smooth_feat, Catch::Approx(one_feature));
    }

    SECTION( "Given two features, some smoothing is applied." ) {
        std::vector<float> expected_smoothing = {0.351123, 0.936329};
        // Create a tracklet to test
        STrack tracklet = STrack(tlwh, confidence, two_features);
        REQUIRE( tracklet.m_smooth_feat.size() == 2);
        CHECK_THAT( tracklet.m_smooth_feat, Catch::Approx(expected_smoothing));
    }
}


//******************************************************************
//  PREDICTION TESTS
//******************************************************************
/**
 * @brief Unit test case for STrack multi_predict
 * 
 */
TEST_CASE( "STracks class can run prediction on a list of STracks.", "[strack_multi_predict]" ) {
    KalmanFilter kalman_filter = KalmanFilter();

    // Create a set of standard inputs, taken from adk yolo dataset
    std::vector<float> tlwh = {103.533936, 546.8082, 75.665934, 247.4281};
    float confidence = 0.87615836;
    std::vector<float> no_features = {};
    STrack tracklet = STrack(tlwh, confidence, no_features);
    // Prepare a list of tracklets
    std::vector<STrack *> strack_pool;

    SECTION( "Given an empty list of STracks, no predictions are made" ) {
        //Run multi-prediction
        STrack::multi_predict(strack_pool, kalman_filter);
        REQUIRE( strack_pool.size() == 0);
    }

    SECTION( "Given a list of STracks, a prediction is made for each" ) {
        // Ensure the list has some standard inputs, We push the same address 3 times,
        // this effectively predicts the tracklet 3 steps forward
        strack_pool.push_back(&tracklet);
        strack_pool.push_back(&tracklet);
        strack_pool.push_back(&tracklet);
        
        xt::xarray<float, xt::layout_type::row_major> expected_covariance_1 = xt::zeros<float>({8, 8});

        //Run multi-prediction
        STrack::multi_predict(strack_pool, kalman_filter);
        
        // Run comparison
        CHECK( compare_float_matrices(strack_pool[0]->m_covariance, expected_covariance_1) );
    }
}

//******************************************************************
//  ACTIVATION TESTS
//******************************************************************
/**
 * @brief Unit test case for STrack activate
 * 
 */
TEST_CASE( "STracks can be activated to iniate their mean/covariance at a given time.", "[strack_activate]" ) {
    KalmanFilter kalman_filter = KalmanFilter();

    SECTION( "A valid tracklet can be activated to valid mean/covariance." ) {
        // Create a set of standard inputs, taken from adk yolo dataset
        std::vector<float> tlwh = {103.533936, 546.8082, 75.665934, 247.4281};
        float confidence = 0.87615836;
        std::vector<float> no_features = {};
        HailoDetectionPtr det = std::make_shared<HailoDetection>(HailoDetection(HailoBBox(0,0,0,0), "label", 0.75));
        STrack tracklet = STrack(tlwh, confidence, no_features, det);


        TrackerTypes::KAL_MEAN expected_mean = {{141.366903, 670.522250, 0.305809785, 247.428100, 0.0, 0.0, 0.0, 0.0}};
        TrackerTypes::KAL_COVA expected_covariance = {{24.48826587,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        },
                                                            { 0.        , 24.48826587,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        },
                                                            { 0.        ,  0.        , 24.48826587,  0.        ,  0.        ,  0.        ,  0.        ,  0.        },
                                                            { 0.        ,  0.        ,  0.        , 24.48826587,  0.        ,  0.        ,  0.        ,  0.        },
                                                            { 0.        ,  0.        ,  0.        ,  0.        ,  6.12206647,  0.        ,  0.        ,  0.        },
                                                            { 0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  6.12206647,  0.        ,  0.        },
                                                            { 0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  1.53051662,  0.        },
                                                            { 0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  1.53051662}};

        // Activate the tracklet
        tracklet.activate(&kalman_filter, 0);

        // Run comparison
        CHECK( compare_float_matrices(tracklet.m_mean, expected_mean) );
        CHECK( compare_float_matrices(tracklet.m_covariance, expected_covariance) );
    }
}