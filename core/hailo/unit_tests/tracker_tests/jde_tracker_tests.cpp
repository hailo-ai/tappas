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
#include "jde_tracker.hpp"
#include "tracker_macros.hpp"
#include "common/common.hpp"

// Open source includes
#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"
#include "xtensor/xmath.hpp"
#include "xtensor/xview.hpp"

//******************************************************************
//  LINEAR ASSIGNMENT TESTS
//******************************************************************
/**
 * @brief Unit test case for lapjv.hpp
 * 
 */
TEST_CASE( "JDE Tracker makes use of the lapjv.hpp library to solve linear assignment using Jonker-Volgenant algorithm.", "[lapjv]" ) {

    SECTION( "An NxN cost matrix should find a match for each object." ) {
        // Create a cost matrix of Mahalanobis distances between 3 tracked objects and 3 new detections
        int n = 3;
        std::vector<std::vector<float>> cost_c = {{0.00288438,  1.0,        1.0},
                                                  {       1.0,  1.0, 0.31932773},
                                                  {       1.0,  0.4,        1.0}};
        int *x_c = new int[sizeof(int) * n];
        int *y_c = new int[sizeof(int) * n];

        // Prepare the cost matrix into a double **
        double **cost_ptr;
        cost_ptr = new double *[sizeof(double *) * n];
        for (int i = 0; i < n; i++)
            cost_ptr[i] = new double[sizeof(double) * n];

        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                cost_ptr[i][j] = cost_c[i][j];
            }
        }

        // Make the linear assignment
        int ret = lapjv_internal(n, cost_ptr, x_c, y_c);

        // Check for valid return
        REQUIRE( ret == 0);  // Success code
        CHECK( x_c[0] == 0 );  // tracked[0] matches detected[0]
        CHECK( x_c[1] == 2 );  // tracked[1] matches detected[2]
        CHECK( x_c[2] == 1 );  // tracked[2] matches detected[1]
        CHECK( y_c[0] == 0 );  // detected[0] matches tracked[0]
        CHECK( y_c[1] == 2 );  // detected[1] matches tracked[2]
        CHECK( y_c[2] == 1 );  // detected[2] matches tracked[1]
    }

    SECTION( "An NxN-1 cost matrix should leave one object unmatched." ) {
        // Create a cost matrix of Mahalanobis distances between 3 tracked objects and 2 new detections
        int n = 3;
        std::vector<std::vector<float>> cost_c = {{0.00288438,  1.0},
                                                  {       1.0,  1.0},
                                                  {       1.0,  0.4}};

        int *x_c = new int[sizeof(int) * 3];
        int *y_c = new int[sizeof(int) * 2];

        // Prepare the cost matrix into a double **
        double **cost_ptr;
        cost_ptr = new double *[sizeof(double *) * 3];
        for (int i = 0; i < n; i++)
            cost_ptr[i] = new double[sizeof(double) * 2];

        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                cost_ptr[i][j] = cost_c[i][j];
            }
        }

        // Make the linear assignment
        int ret = lapjv_internal(n, cost_ptr, x_c, y_c);

        // Check for valid return
        REQUIRE( ret == 0);  // Success code
        CHECK( x_c[0] == 0 );  // tracked[0] matches detected[0]
        CHECK( x_c[2] == 1 );  // tracked[1] matches detected[1]
        CHECK( y_c[0] == 0 );  // detected[0] matches tracked[0]
        CHECK( y_c[1] == 2 );  // detected[1] matches tracked[2]
    }

}

/**
 * @brief Unit test case for jde_tracker_lapjv
 * 
 */
TEST_CASE( "JDE Tracker can match new detections to tracked objects using linear assignment", "[jde_tracker_lapjv]" ) {
    JDETracker jde_tracker = JDETracker();

    SECTION( "An NxN cost matrix should find a match for each object." ) {
        // Create a cost matrix of Mahalanobis distances between 3 tracked objects and 3 new detections
        std::vector<int> tracked_matches;
        std::vector<int> detected_matches;
        std::vector<std::vector<float>> cost_matrix = {{0.00288438,  1.0,        1.0},
                                                       {       1.0,  1.0, 0.31932773},
                                                       {       1.0,  0.4,        1.0}};
        double expected_cost = 0.722212114;  // Expected total cost, recieved by adk lapjv for same input cost matrix

        // Make the linear assignment
        double cost = lapjv_external(cost_matrix, tracked_matches, detected_matches, true, 0.7);

        // Check for valid return
        CHECK( cost == Approx(expected_cost) );
        CHECK( tracked_matches[0] == 0 );  // tracked[0] matches detected[0]
        CHECK( tracked_matches[1] == 2 );  // tracked[1] matches detected[2]
        CHECK( tracked_matches[2] == 1 );  // tracked[2] matches detected[1]
        CHECK( detected_matches[0] == 0 );  // detected[0] matches tracked[0]
        CHECK( detected_matches[1] == 2 );  // detected[1] matches tracked[2]
        CHECK( detected_matches[2] == 1 );  // detected[2] matches tracked[1]
    }

    SECTION( "An NxN-1 cost matrix should leave one object unmatched." ) {
        // Create a cost matrix of Mahalanobis distances between 3 tracked objects and 2 new detections
        std::vector<int> tracked_matches;
        std::vector<int> detected_matches;
        std::vector<std::vector<float>> cost_matrix = {{0.00288438,  1.0},
                                                       {       1.0,  1.0},
                                                       {       1.0,  0.4}};
        double expected_cost = 0.402884383;  // Expected total cost, recieved by adk lapjv for same input cost matrix

        // Make the linear assignment
        double cost = lapjv_external(cost_matrix, tracked_matches, detected_matches, true, 0.7);

        // Check for valid return
        CHECK( cost == Approx(expected_cost) );
        CHECK( tracked_matches[0] == 0 );  // tracked[0] matches detected[0]
        CHECK( tracked_matches[1] ==-1 );  // tracked[0] is unmatched!
        CHECK( tracked_matches[2] == 1 );  // tracked[2] matches detected[1]
        CHECK( detected_matches[0] == 0 ); // detected[0] matches tracked[0]
        CHECK( detected_matches[1] == 2 ); // detected[1] matches tracked[2]

    }
}

//******************************************************************
//  DISTANCE CALCULATION TESTS
//******************************************************************
/**
 * @brief Unit test case for jde_tracker_ious
 * 
 */
TEST_CASE( "JDE Tracker can calculate iou distances between two sets of objects", "[jde_tracker_iou]" ) {
    JDETracker jde_tracker = JDETracker();

    SECTION( "Given two sets of length N, there should be an NxN cost matrix" ) {
        // Prepare a set of tracked and detected objects, taken from adk yolo dataset
        std::vector<std::vector<float>> tracked_objects = {{103.533936, 546.8082, 179.19987, 794.2363}, {50.0, 200.0, 100.0, 250.0}, {10.0, 100.0, 40.0, 150.0}};
        std::vector<std::vector<float>> detected_objects = {{103.56223, 546.8928, 179.32939, 794.3509}, {15.0, 105.0, 45.0, 155.0}, {55.0, 205.0, 105.0, 255.0}};

        // Expected cost matrix, recieved by adk tracker for same input objects
        std::vector<std::vector<float>> expected_cost_matrix = {{0.99711562 , 0.        , 0.        },
                                                                {0.        , 0.        , 0.68067227},
                                                                {0.        , 0.6      , 0.        }};

        // Calculate the iou cost matrix
        std::vector<std::vector<float>> calculated_cost_matrix = ious(tracked_objects, detected_objects);

        // Check that the two cost matrices are the same
        CHECK_THAT( calculated_cost_matrix[0], Catch::Approx(expected_cost_matrix[0]) );
        CHECK_THAT( calculated_cost_matrix[1], Catch::Approx(expected_cost_matrix[1]) );
        CHECK_THAT( calculated_cost_matrix[2], Catch::Approx(expected_cost_matrix[2]) );

    }

    SECTION( "Given two sets of length N and N-1, there should be an NxN-1 cost matrix" ) {
        // Prepare a set of tracked and detected objects, taken from adk yolo dataset
        std::vector<std::vector<float>> tracked_objects = {{103.533936, 546.8082, 179.19987, 794.2363}, {50.0, 200.0, 100.0, 250.0}, {10.0, 100.0, 40.0, 150.0}};
        std::vector<std::vector<float>> detected_objects = {{103.56223, 546.8928, 179.32939, 794.3509}, {15.0, 105.0, 45.0, 155.0}};

        // Expected cost matrix, recieved by adk tracker for same input objects
        std::vector<std::vector<float>> expected_cost_matrix = {{0.99711562   , 0.     },
                                                                {0.        , 0.     },
                                                                {0.        , 0.6}};

        // Calculate the iou cost matrix
        std::vector<std::vector<float>> calculated_cost_matrix = ious(tracked_objects, detected_objects);

        // Check that the two cost matrices are the same
        CHECK_THAT( calculated_cost_matrix[0], Catch::Approx(expected_cost_matrix[0]) );
        CHECK_THAT( calculated_cost_matrix[1], Catch::Approx(expected_cost_matrix[1]) );
        CHECK_THAT( calculated_cost_matrix[2], Catch::Approx(expected_cost_matrix[2]) );
    }
}

//******************************************************************
//  UPDATE TESTS
//******************************************************************

std::vector<std::vector<float>> stracks_to_tlwh(std::vector<STrack> tracks)
{
    std::vector<std::vector<float>> tlwhs(tracks.size() , std::vector<float> (4));
    for (uint i = 0; i < tracks.size(); i++)
    {
        tlwhs[i] = tracks[i].m_tlwh;
    }
    return tlwhs;
}

/**
 * @brief Unit test case for jde_tracker_update
 * 
 */
TEST_CASE( "JDE Tracker can update its lst of tracked objects based on new detections", "[jde_tracker_update]" ) {

    SECTION( "Given a few frames of detections, some objects should be tracked." ) {
        JDETracker jde_tracker = JDETracker(); // A JDETracker to test

        // Prepare a set of detected objects, taken from adk yolo dataset
        HailoDetection detected1_box1(HailoBBox(103.533936, 546.8082, 75.665934, 247.4281), "", 0.87615836);
        HailoDetection detected1_box2(HailoBBox(50.0, 200.0, 50.0, 50.0), "", 0.9);
        HailoDetection detected1_box3(HailoBBox(10.0, 100.0, 30.0, 50.0), "", 0.9);
        std::vector<HailoDetectionPtr> inputs1;
        inputs1.push_back(std::make_shared<HailoDetection>(detected1_box1));
        inputs1.push_back(std::make_shared<HailoDetection>(detected1_box2));
        inputs1.push_back(std::make_shared<HailoDetection>(detected1_box3));

        // Prepare etections from the following frame in the same dataset
        HailoDetection detected2_box1(HailoBBox(103.56223, 546.8928, 75.76716, 247.4581), "", 0.87676835);
        HailoDetection detected2_box2(HailoBBox(15.0, 105.0, 30.0, 50.0), "", 0.9);
        HailoDetection detected2_box3(HailoBBox(55.0, 205.0, 50.0, 50.0), "", 0.9);
        std::vector<HailoDetectionPtr> inputs2;
        inputs2.push_back(std::make_shared<HailoDetection>(detected2_box1));
        inputs2.push_back(std::make_shared<HailoDetection>(detected2_box2));
        inputs2.push_back(std::make_shared<HailoDetection>(detected2_box3));

        // Prepare etections from the following frame in the same dataset
        HailoDetection detected3_box1(HailoBBox(103.7972, 547.0619, 75.52331, 247.07103), "", 0.87676835);
        HailoDetection detected3_box2(HailoBBox(20.0, 110.0, 30.0, 50.0), "", 0.9);
        std::vector<HailoDetectionPtr> inputs3;
        inputs3.push_back(std::make_shared<HailoDetection>(detected3_box1));
        inputs3.push_back(std::make_shared<HailoDetection>(detected3_box2));

        // First round update
        std::vector<STrack> results1 = jde_tracker.update(inputs1);
        REQUIRE( results1.size() == 0 );  // There are no tracked objects after just one frame

        // Second round update
        std::vector<STrack> results2 = jde_tracker.update(inputs2);
        // The expected tracked objects, recieved by adk tracker for same input objects
        std::vector<std::vector<float>> expected_tlwhs2 = {{103.53394, 546.80823, 75.66593, 247.4281}, {50., 200.,  50.,  50.}, {10., 100.,  30.,  50.}};
        REQUIRE( results2.size() == 3 );  // After seeing the same 3 objects twice, there should be 3 tracked objects
        std::vector<std::vector<float>> results2_tlwhs = stracks_to_tlwh(results2);
        CHECK_THAT( results2_tlwhs[0], Catch::Approx(expected_tlwhs2[0]) );
        CHECK_THAT( results2_tlwhs[1], Catch::Approx(expected_tlwhs2[1]) );
        CHECK_THAT( results2_tlwhs[2], Catch::Approx(expected_tlwhs2[2]) );

        // Third round update
        std::vector<STrack> results3 = jde_tracker.update(inputs3);
        // The expected tracked objects, recieved by adk tracker for same input objects
        std::vector<std::vector<float>> expected_tlwhs3 = {{103.75837161, 547.02259654,  75.54612335, 247.1281612,}, {18.57142857, 108.57142857,  30., 50.}, {50., 200., 50., 50.}};
        // Although one tracked object did not get a match, it is still tracked since only 1 frame has past unmatched 
        REQUIRE( results3.size() == 3 );
        std::vector<std::vector<float>> results3_tlwhs = stracks_to_tlwh(results3);
        CHECK_THAT( results3_tlwhs[0], Catch::Approx(expected_tlwhs3[0]) );
        CHECK_THAT( results3_tlwhs[1], Catch::Approx(expected_tlwhs3[1]) );
        CHECK_THAT( results3_tlwhs[2], Catch::Approx(expected_tlwhs3[2]) );
    }

}