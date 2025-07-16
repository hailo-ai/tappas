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
#include "hailo_objects.hpp"
#include "hailo_common.hpp"
#include "debug.hpp"

TEST_CASE( "The debug postprocess can generate random detections", "[debug_detections]" ) {
    // Create a dummy roi
    HailoBBox main_bbox = HailoBBox(0, 0, 1, 1);

    SECTION( "The added detections are random in number and dimensions." ) {
        // Create a main roi to fill with random detections
        HailoROI main_roi = HailoROI(main_bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);

        // Generate and add the detections
        generate_random_detections(main_roi_ptr);

        std::vector<HailoDetectionPtr> new_detections = hailo_common::get_hailo_detections(main_roi_ptr);

        // Check the detections
        CHECK(new_detections.size() > 0); // There should be at least 1 new detection
        // Check that the generated detections are within the roi
        for (uint i = 0; i < new_detections.size(); i++)
        {
            HailoBBox r_bbox = new_detections[i]->get_bbox();
            CHECK(r_bbox.xmin() >= main_bbox.xmin());
            CHECK(r_bbox.ymin() >= main_bbox.ymin());
            CHECK(r_bbox.xmax() <= main_bbox.xmax());
            CHECK(r_bbox.ymax() <= main_bbox.ymax());
            CHECK(r_bbox.width() >= RANDOM_DIM_LIMIT);
            CHECK(r_bbox.height() >= RANDOM_DIM_LIMIT);
        }
    }
}