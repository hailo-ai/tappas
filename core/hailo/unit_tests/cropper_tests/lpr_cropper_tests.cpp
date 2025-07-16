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
#include "common/resources/license_plates/license_plates.hpp"
#include "lpr_croppers.hpp"

// Open source includes
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>


TEST_CASE( "Given an image, quality estimation can determine if there is too much blur.", "[quality_estimation]" ) {
    HailoBBox bbox = HailoBBox(0, 0, 1, 1); // ROI from 0 to 1 is the whole image

    SECTION( "Images that are relatively clear should be above threshold" ) {
        // Load clear images
        cv::Mat input_pe3820 = cv::imread(CLEAR_PLATE_PE3820, cv::IMREAD_COLOR);
        std::shared_ptr<HailoRGBMat> input_pe3820_h = std::make_shared<HailoRGBMat>(input_pe3820, "input_pe3820");
        cv::Mat input_sm7080 = cv::imread(CLEAR_PLATE_SM7080, cv::IMREAD_COLOR);
        std::shared_ptr<HailoRGBMat> input_sm7080_h = std::make_shared<HailoRGBMat>(input_sm7080, "input_sm7080");
        // cv::Mat input_yhi4 = cv::imread(CLEAR_PLATE_YHI4, cv::IMREAD_COLOR);
        cv::Mat input_lpr = cv::imread(CLEAR_PLATE_LPR_4253980, cv::IMREAD_COLOR);
        std::shared_ptr<HailoRGBMat> input_lpr_h = std::make_shared<HailoRGBMat>(input_lpr, "input_lpr");

        // Calculate the variance
        float variance1 = quality_estimation(input_pe3820_h, bbox, 0.1);
        float variance2 = quality_estimation(input_sm7080_h, bbox, 0.1);
        // float variance3 = quality_estimation(input_yhi4, bbox, 0.1);
        float variance4 = quality_estimation(input_lpr_h, bbox, 0.1);
        
        // Check that variance is above threshold
        CHECK( variance1 >= QUALITY_THRESHOLD );
        CHECK( variance2 >= QUALITY_THRESHOLD );
        // CHECK( variance3 >= QUALITY_THRESHOLD );
        CHECK( variance4 >= QUALITY_THRESHOLD );
    }

    SECTION( "Images that are blurry should be below threshold" ) {
        // Load blurry images
        cv::Mat input_apq5 = cv::imread(BLURRY_PLATE_APQ5, cv::IMREAD_COLOR);
        std::shared_ptr<HailoRGBMat> input_apq5_h = std::make_shared<HailoRGBMat>(input_apq5, "input_apq5");
        cv::Mat input_kho5 = cv::imread(BLURRY_PLATE_KHO5, cv::IMREAD_COLOR);
        std::shared_ptr<HailoRGBMat> input_kho5_h = std::make_shared<HailoRGBMat>(input_kho5, "input_kho5");

        // Calculate the variance
        float variance1 = quality_estimation(input_apq5_h, bbox, 0.1);
        float variance2 = quality_estimation(input_kho5_h, bbox, 0.1);

        // Check that variance is below threshold
        if (QUALITY_THRESHOLD > 0.0)
        {
            CHECK( variance1 <= QUALITY_THRESHOLD );
            CHECK( variance2 <= QUALITY_THRESHOLD );
        }
    }
}

TEST_CASE( "Given a HailoROIPtr, vehicles_without_ocr will filter out detections with no OCR", "[vehicles_without_ocr]" ) {
    // Load a dummy image
    auto dummy_image = std::make_shared<HailoRGBMat>(cv::Mat(1920, 1080, CV_8UC3), "");
    HailoBBox bbox = HailoBBox(0, 0, 1, 1); // The dummy image is sized (full size)
    HailoBBox car_bbox = HailoBBox(0.1, 0.8, 0.3, 0.1); // The car bbox

    SECTION( "If no detections have an ocr, they should all be picked for cropping." ) {
        // Create a main roi with some untracked vehicles
        HailoROI main_roi = HailoROI(bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);
        HailoDetection car1 = HailoDetection(car_bbox, "car", 1.0);
        HailoDetection car2 = HailoDetection(car_bbox, "car", 1.0);
        HailoDetection car3 = HailoDetection(car_bbox, "car", 1.0);
        std::vector<HailoDetection> cars = {car1, car2, car3};
        hailo_common::add_detections(main_roi_ptr, cars);

        // Filter out the untracked vehicles
        std::vector<HailoROIPtr> untracked_vehicles = vehicles_without_ocr(dummy_image, main_roi_ptr);

        // Check that all untracked vehicles are returned
        CHECK( untracked_vehicles.size() == 3 );
    }

    SECTION( "If a detection has an ocr, then it should be excluded from cropping." ) {
        // Create a main roi with some untracked and tracked vehicles
        HailoROI main_roi = HailoROI(bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);
        HailoDetection car1 = HailoDetection(car_bbox, "car", 1.0);
        HailoDetection car2 = HailoDetection(car_bbox, "car", 1.0);
        HailoDetection car3 = HailoDetection(car_bbox, "car", 1.0);
        car2.add_object(std::make_shared<HailoClassification>("ocr", "PE3820", 1.0)); // Add an ocr to one of the cars
        std::vector<HailoDetection> cars = {car1, car2, car3};
        hailo_common::add_detections(main_roi_ptr, cars);

        // Filter out the untracked vehicles
        std::vector<HailoROIPtr> untracked_vehicles = vehicles_without_ocr(dummy_image, main_roi_ptr);

        // Check that all untracked vehicles are returned
        CHECK( untracked_vehicles.size() == 2 );
    }
}