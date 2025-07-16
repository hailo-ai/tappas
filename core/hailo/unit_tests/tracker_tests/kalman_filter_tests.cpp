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
#include "tracker_macros.hpp"
#include "common/common.hpp"

// Open source includes
#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"
#include "xtensor/xmath.hpp"
#include "xtensor/xview.hpp"


//******************************************************************
// CHI2INV95 TESTS
//******************************************************************
/**
 * @brief Unit test case for Kalman filter chi2inv95
 * 
 */
TEST_CASE( "Kalman filter provides a chi2inv95 table.", "[kalman_chi2inv95]" ) {
    // Create a kalman filter to test
    KalmanFilter kalman_filter = KalmanFilter();

    REQUIRE( (float)kalman_filter.chi2inv95[0] == 0.0f);
    REQUIRE( (float)kalman_filter.chi2inv95[1] == 3.8415f);
    REQUIRE( (float)kalman_filter.chi2inv95[2] == 5.9915f);
    REQUIRE( (float)kalman_filter.chi2inv95[3] == 7.8147f);
    REQUIRE( (float)kalman_filter.chi2inv95[4] == 9.4877f);
    REQUIRE( (float)kalman_filter.chi2inv95[5] == 11.070f);
    REQUIRE( (float)kalman_filter.chi2inv95[6] == 12.592f);
    REQUIRE( (float)kalman_filter.chi2inv95[7] == 14.067f);
    REQUIRE( (float)kalman_filter.chi2inv95[8] == 15.507f);
    REQUIRE( (float)kalman_filter.chi2inv95[9] == 16.919f);
}


//******************************************************************
// INITIATE TESTS
//******************************************************************
/**
 * @brief Unit test for Kalman filter tracklet initiation
 * 
 */
TEST_CASE( "Kalman filter can initiate a tracklet from an unassociated measurement.", "[kalman_initiate]" ) {
    // Create a kalman filter to test
    KalmanFilter kalman_filter = KalmanFilter();

    // A blank input measuremnt
    TrackerTypes::DETECTBOX blank_input = xt::zeros<float>({1, 4});

    // A standard input measuremnt, taken from adk yolo dataset
    TrackerTypes::DETECTBOX standard_input_1 = {{141.366903, 670.522250, 0.305809785, 247.428100}};

    SECTION( "Standard inputs initiate safely" ) {
        // Expected mean and covariance, taken from adk tracker on standard_input_1
        TrackerTypes::KAL_MEAN expected_mean = {{141.366903, 670.522250, 0.305809785, 247.428100, 0.0, 0.0, 0.0, 0.0}};
        TrackerTypes::KAL_COVA expected_covariance = {{24.48826587,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        },
                                                      { 0.        , 24.48826587,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        },
                                                      { 0.        ,  0.        , 24.48826587,  0.        ,  0.        ,  0.        ,  0.        ,  0.        },
                                                      { 0.        ,  0.        ,  0.        , 24.48826587,  0.        ,  0.        ,  0.        ,  0.        },
                                                      { 0.        ,  0.        ,  0.        ,  0.        ,  6.12206647,  0.        ,  0.        ,  0.        },
                                                      { 0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  6.12206647,  0.        ,  0.        },
                                                      { 0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  1.53051662,  0.        },
                                                      { 0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  1.53051662}};

        // Iniate using standard_input_1
        auto initiate_results = kalman_filter.initiate(standard_input_1);
        TrackerTypes::KAL_MEAN initial_mean = initiate_results.first;
        TrackerTypes::KAL_COVA initial_covariance = initiate_results.second;

        // Run comparison
        CHECK( compare_float_matrices(initial_mean, expected_mean) );
        CHECK( compare_float_matrices(initial_covariance, expected_covariance) );
    }

    SECTION( "Blank measurements produce blank features" ) {
        // Expected mean and covariance, taken from adk tracker on standard_input_1
        TrackerTypes::KAL_MEAN expected_mean = xt::zeros<float>({1, 8});
        TrackerTypes::KAL_COVA expected_covariance = xt::zeros<float>({8, 8});

        // Iniate using blank_input
        auto initiate_results = kalman_filter.initiate(blank_input);
        TrackerTypes::KAL_MEAN initial_mean = initiate_results.first;
        TrackerTypes::KAL_COVA initial_covariance = initiate_results.second;

        // Run comparison
        CHECK( compare_float_matrices(initial_mean, expected_mean) );
        CHECK( compare_float_matrices(initial_covariance, expected_covariance) );
    }
}


//******************************************************************
// PROJECT TESTS
//******************************************************************
/**
 * @brief Unit test for Kalman filter tracklet projection
 * 
 */
TEST_CASE( "Kalman filter can project state distribution to measurement space.", "[kalman_project]" ) {
    // Create a kalman filter to test
    KalmanFilter kalman_filter = KalmanFilter();

    // Blank input mean / covariance
    TrackerTypes::KAL_MEAN blank_input_mean = xt::zeros<float>({1, 8});
    TrackerTypes::KAL_COVA blank_input_covariance = xt::zeros<float>({8, 8});

    // Standard input means/covariances, taken from adk yolo dataset
    TrackerTypes::KAL_MEAN standard_input_mean_1 = {{141.366903, 670.522250, 0.305809785, 247.428100, 0.0, 0.0, 0.0, 0.0}};
    TrackerTypes::KAL_COVA standard_input_covariance_1 = {{36.7323988 ,  0.        ,  0.        ,  0.        ,  6.12206647,  0.        ,  0.        ,  0.        },
                                                          { 0.        , 36.7323988 ,  0.        ,  0.        ,  0.        ,  6.12206647,  0.        ,  0.        },
                                                          { 0.        ,  0.        , 32.14084895,  0.        ,  0.        ,  0.        ,  1.53051662,  0.        },
                                                          { 0.        ,  0.        ,  0.        , 32.14084895,  0.        ,  0.        ,  0.        ,  1.53051662},
                                                          { 6.12206647,  0.        ,  0.        ,  0.        ,  6.18328713,  0.        ,  0.        ,  0.        },
                                                          { 0.        ,  6.12206647,  0.        ,  0.        ,  0.        ,  6.18328713,  0.        ,  0.        },
                                                          { 0.        ,  0.        ,  1.53051662,  0.        ,  0.        ,  0.        ,  1.59173728,  0.        },
                                                          { 0.        ,  0.        ,  0.        ,  1.53051662,  0.        ,  0.        ,  0.        ,  1.59173728}};

    SECTION( "Standard inputs projects safely" ) {
        // Expected mean and covariance, taken from adk tracker on standard_input_1
        TrackerTypes::KAL_HMEAN expected_mean = {{141.366903, 670.522250, 0.305809785, 247.428100}};
        TrackerTypes::KAL_HCOVA expected_covariance = {{42.85446527,  0.        ,  0.        ,  0.        },
                                                       { 0.        , 42.85446527,  0.        ,  0.        },
                                                       { 0.        ,  0.        , 38.26291542,  0.        },
                                                       { 0.        ,  0.        ,  0.        , 38.26291542}};

        // Project using the standard inputs
        auto projection_results = kalman_filter.project(standard_input_mean_1, standard_input_covariance_1);
        TrackerTypes::KAL_HMEAN projected_mean = projection_results.first;
        TrackerTypes::KAL_HCOVA projected_covariance = projection_results.second;

        // Run comparison
        CHECK( compare_float_matrices(projected_mean, expected_mean) );
        CHECK( compare_float_matrices(projected_covariance, expected_covariance) );
  }

  SECTION( "Blank measurements produce blank projections" ) {
    // Expected mean and covariance, taken from adk tracker on blank input
    TrackerTypes::KAL_HMEAN expected_mean = xt::zeros<float>({1, 4});
    TrackerTypes::KAL_HCOVA expected_covariance = xt::zeros<float>({4, 4});

    // Project using blank_input
    auto projection_results = kalman_filter.project(blank_input_mean, blank_input_covariance);
    TrackerTypes::KAL_HMEAN projected_mean = projection_results.first;
    TrackerTypes::KAL_HCOVA projected_covariance = projection_results.second;

    // Run comparison
    CHECK( compare_float_matrices(projected_mean, expected_mean) );
    CHECK( compare_float_matrices(projected_covariance, expected_covariance) );
  }
}


//******************************************************************
// PREDICT TESTS
//******************************************************************
/**
 * @brief Unit test for Kalman filter tracklet prediction
 * 
 */
TEST_CASE( "Kalman filter can predict the progression of tracklet mean and covariance.", "[kalman_predict]" ) {
    // Create a kalman filter to test
    KalmanFilter kalman_filter = KalmanFilter();

    // Blank input mean / covariance
    TrackerTypes::KAL_MEAN blank_input_mean = xt::zeros<float>({1, 8});
    TrackerTypes::KAL_COVA blank_input_covariance = xt::zeros<float>({8, 8});

    // Standard input means/covariances, taken from adk yolo dataset
    TrackerTypes::KAL_MEAN standard_input_mean_1 = {{141.366903, 670.522250, 0.305809785, 247.428100, 0.0, 0.0, 0.0, 0.0}};
    TrackerTypes::KAL_COVA standard_input_covariance_1 = {{24.48826587,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        },
                                                          { 0.        , 24.48826587,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        },
                                                          { 0.        ,  0.        , 24.48826587,  0.        ,  0.        ,  0.        ,  0.        ,  0.        },
                                                          { 0.        ,  0.        ,  0.        , 24.48826587,  0.        ,  0.        ,  0.        ,  0.        },
                                                          { 0.        ,  0.        ,  0.        ,  0.        ,  6.12206647,  0.        ,  0.        ,  0.        },
                                                          { 0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  6.12206647,  0.        ,  0.        },
                                                          { 0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  1.53051662,  0.        },
                                                          { 0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  0.        ,  1.53051662}};

    SECTION( "Standard inputs predicts safely" ) {
        // Expected mean and covariance, taken from adk tracker on standard_input_1
        TrackerTypes::KAL_MEAN expected_mean = {{141.366903, 670.522250, 0.305809785, 247.428100, 0.0, 0.0, 0.0, 0.0}};
        TrackerTypes::KAL_COVA expected_covariance = {{36.7323988 ,  0.        ,  0.        ,  0.        ,  6.12206647,  0.        ,  0.        ,  0.        },
                                                      { 0.        , 36.7323988 ,  0.        ,  0.        ,  0.        ,  6.12206647,  0.        ,  0.        },
                                                      { 0.        ,  0.        , 32.14084895,  0.        ,  0.        ,  0.        ,  1.53051662,  0.        },
                                                      { 0.        ,  0.        ,  0.        , 32.14084895,  0.        ,  0.        ,  0.        ,  1.53051662},
                                                      { 6.12206647,  0.        ,  0.        ,  0.        ,  6.18328713,  0.        ,  0.        ,  0.        },
                                                      { 0.        ,  6.12206647,  0.        ,  0.        ,  0.        ,  6.18328713,  0.        ,  0.        },
                                                      { 0.        ,  0.        ,  1.53051662,  0.        ,  0.        ,  0.        ,  1.59173728,  0.        },
                                                      { 0.        ,  0.        ,  0.        ,  1.53051662,  0.        ,  0.        ,  0.        ,  1.59173728}};

        // Predict using the standard inputs, the values are changed in-place
        kalman_filter.predict(standard_input_mean_1, standard_input_covariance_1);

        // Run comparison
        CHECK( compare_float_matrices(standard_input_mean_1, expected_mean) );
        CHECK( compare_float_matrices(standard_input_covariance_1, expected_covariance) );
    }

    SECTION( "Blank inputs produce blank predictions" ) {
        // Expected mean and covariance, taken from adk tracker on blank input
        TrackerTypes::KAL_MEAN expected_mean = xt::zeros<float>({1, 8});
        TrackerTypes::KAL_COVA expected_covariance = xt::zeros<float>({8, 8});

        // Predict using the blank inputs, the values are changed in-place
        kalman_filter.predict(blank_input_mean, blank_input_covariance);

        // Run comparison
        CHECK( compare_float_matrices(blank_input_mean, expected_mean) );
        CHECK( compare_float_matrices(blank_input_covariance, expected_covariance) );
    }
}


//******************************************************************
// UPDATE TESTS
//******************************************************************
/**
 * @brief Unit test for Kalman filter tracklet updating
 * 
 */
TEST_CASE( "Kalman filter can update a tracklet based on a newly measured detection.", "[kalman_update]" ) {
    // Create a kalman filter to test
    KalmanFilter kalman_filter = KalmanFilter();

    // Blank input measurement
    TrackerTypes::KAL_COVA blank_input_measurement = xt::zeros<float>({1, 4});

    // Standard input means/covariances, taken from adk yolo dataset
    TrackerTypes::KAL_MEAN standard_input_mean_1 = {{141.366903, 670.522250, 0.305809785, 247.428100, 0.0, 0.0, 0.0, 0.0}};
    TrackerTypes::KAL_COVA standard_input_covariance_1 = {{36.7323988 ,  0.        ,  0.        ,  0.        ,  6.12206647,  0.        ,  0.        ,  0.        },
                                                          { 0.        , 36.7323988 ,  0.        ,  0.        ,  0.        ,  6.12206647,  0.        ,  0.        },
                                                          { 0.        ,  0.        , 32.14084895,  0.        ,  0.        ,  0.        ,  1.53051662,  0.        },
                                                          { 0.        ,  0.        ,  0.        , 32.14084895,  0.        ,  0.        ,  0.        ,  1.53051662},
                                                          { 6.12206647,  0.        ,  0.        ,  0.        ,  6.18328713,  0.        ,  0.        ,  0.        },
                                                          { 0.        ,  6.12206647,  0.        ,  0.        ,  0.        ,  6.18328713,  0.        ,  0.        },
                                                          { 0.        ,  0.        ,  1.53051662,  0.        ,  0.        ,  0.        ,  1.59173728,  0.        },
                                                          { 0.        ,  0.        ,  0.        ,  1.53051662,  0.        ,  0.        ,  0.        ,  1.59173728}};
    TrackerTypes::DETECTBOX standard_input_measurements_1 = {{141.558855, 670.597415, 0.305674486, 247.071030}};

    SECTION( "Standard inputs update safely" ) {
        // Expected mean and covariance, taken from adk tracker on standard_input_1
        TrackerTypes::KAL_MEAN expected_mean = {{141.531433,  670.586677,  0.305696133,  247.128161,  0.0274217143,  0.0107378571, -0.00000541196597, -0.0142828000}};
        TrackerTypes::KAL_COVA expected_covariance = {{5.24748554, 0.        , 0.        , 0.        , 0.87458092, 0.        , 0.        , 0.        },
                                                      {0.        , 5.24748554, 0.        , 0.        , 0.        , 0.87458092, 0.        , 0.        },
                                                      {0.        , 0.        , 5.14253583, 0.        , 0.        , 0.        , 0.24488266, 0.        },
                                                      {0.        , 0.        , 0.        , 5.14253583, 0.        , 0.        , 0.        , 0.24488266},
                                                      {0.87458092, 0.        , 0.        , 0.        , 5.30870621, 0.        , 0.        , 0.        },
                                                      {0.        , 0.87458092, 0.        , 0.        , 0.        , 5.30870621, 0.        , 0.        },
                                                      {0.        , 0.        , 0.24488266, 0.        , 0.        , 0.        , 1.53051662, 0.        },
                                                      {0.        , 0.        , 0.        , 0.24488266, 0.        , 0.        , 0.        , 1.53051662}};

        // Update using the standard inputs
        auto projection_results = kalman_filter.update(standard_input_mean_1, standard_input_covariance_1, standard_input_measurements_1);
        TrackerTypes::KAL_MEAN updated_mean = projection_results.first;
        TrackerTypes::KAL_COVA updated_covariance = projection_results.second;

        // Run comparison
        CHECK( compare_float_matrices(updated_mean, expected_mean) );
        CHECK( compare_float_matrices(updated_covariance, expected_covariance) );
    }

    SECTION( "Blank measurements produce drastic changes in mean, but not covariance" ) {
        // Expected mean and covariance, taken from adk tracker on blank input measurement
        TrackerTypes::KAL_MEAN expected_mean = {{20.1952719,  95.7888929,  0.0489295656,  39.5884960, -20.1952719, -95.7888929, -0.0122323914, -9.89712402}};
        TrackerTypes::KAL_COVA expected_covariance = {{5.24748554, 0.        , 0.        , 0.        , 0.87458092, 0.        , 0.        , 0.        },
                                                      {0.        , 5.24748554, 0.        , 0.        , 0.        , 0.87458092, 0.        , 0.        },
                                                      {0.        , 0.        , 5.14253583, 0.        , 0.        , 0.        , 0.24488266, 0.        },
                                                      {0.        , 0.        , 0.        , 5.14253583, 0.        , 0.        , 0.        , 0.24488266},
                                                      {0.87458092, 0.        , 0.        , 0.        , 5.30870621, 0.        , 0.        , 0.        },
                                                      {0.        , 0.87458092, 0.        , 0.        , 0.        , 5.30870621, 0.        , 0.        },
                                                      {0.        , 0.        , 0.24488266, 0.        , 0.        , 0.        , 1.53051662, 0.        },
                                                      {0.        , 0.        , 0.        , 0.24488266, 0.        , 0.        , 0.        , 1.53051662}};

        // Update using blank_input
        auto projection_results = kalman_filter.update(standard_input_mean_1, standard_input_covariance_1, blank_input_measurement);
        TrackerTypes::KAL_MEAN updated_mean = projection_results.first;
        TrackerTypes::KAL_COVA updated_covariance = projection_results.second;

        // Run comparison
        CHECK( compare_float_matrices(updated_mean, expected_mean) );
        CHECK( compare_float_matrices(updated_covariance, expected_covariance) );
    }
}


//******************************************************************
// GATING DISTANCE TESTS
//******************************************************************
/**
 * @brief Unit test for Kalman filter tracklet gating distance
 * 
 */
TEST_CASE( "Kalman filter can compute gating distance between state distribution and measurements.", "[kalman_gating_distance]" ) {
    // Create a kalman filter to test
    KalmanFilter kalman_filter = KalmanFilter();

    // Blank input measurement
    TrackerTypes::DETECTBOX blank_input_measurements_1 = {{0.0, 0.0, 0.0, 0.0}};

    // Standard input means/covariances/measurements, taken from adk yolo dataset
    TrackerTypes::KAL_MEAN standard_input_mean_1 = {{141.366903, 670.522250, 0.305809785, 247.428100, 0.0, 0.0, 0.0, 0.0}};
    TrackerTypes::KAL_COVA standard_input_covariance_1 = {{36.7323988 ,  0.        ,  0.        ,  0.        ,  6.12206647,  0.        ,  0.        ,  0.        },
                                                          { 0.        , 36.7323988 ,  0.        ,  0.        ,  0.        ,  6.12206647,  0.        ,  0.        },
                                                          { 0.        ,  0.        , 32.14084895,  0.        ,  0.        ,  0.        ,  1.53051662,  0.        },
                                                          { 0.        ,  0.        ,  0.        , 32.14084895,  0.        ,  0.        ,  0.        ,  1.53051662},
                                                          { 6.12206647,  0.        ,  0.        ,  0.        ,  6.18328713,  0.        ,  0.        ,  0.        },
                                                          { 0.        ,  6.12206647,  0.        ,  0.        ,  0.        ,  6.18328713,  0.        ,  0.        },
                                                          { 0.        ,  0.        ,  1.53051662,  0.        ,  0.        ,  0.        ,  1.59173728,  0.        },
                                                          { 0.        ,  0.        ,  0.        ,  1.53051662,  0.        ,  0.        ,  0.        ,  1.59173728}};
    TrackerTypes::DETECTBOX standard_input_measurements_1 = {{141.558855, 670.597415, 0.305674486, 247.071030}};

    SECTION( "Standard measurements get reasonable distances" ) {
        // Expected Mahalanobis distance, taken from adk tracker on standard_input_1
        xt::xarray<float> expected_distance = {0.0043238};
        
        // Compute the gating distance using the standard inputs
        xt::xarray<float> distance_results = kalman_filter.gating_distance(standard_input_mean_1, standard_input_covariance_1, {standard_input_measurements_1});

        // Run comparison
        CHECK( compare_float_matrices(distance_results, expected_distance) );
    }

    SECTION( "Blank measurements get large invalid distances" ) {
        // Expected Mahalanobis distance, taken from adk tracker on standard_input_1
        xt::xarray<float> expected_distance = {12557.665};
        
        // Compute the gating distance using the standard inputs
        xt::xarray<float> distance_results = kalman_filter.gating_distance(standard_input_mean_1, standard_input_covariance_1, {blank_input_measurements_1});

        // Run comparison
        CHECK( compare_float_matrices(distance_results, expected_distance) );
    }
}