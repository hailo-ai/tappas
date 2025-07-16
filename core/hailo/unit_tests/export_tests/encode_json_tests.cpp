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
#include "encode_json.hpp"
#include "hailo_objects.hpp"
#include "hailo_common.hpp"

// Open source includes
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"

/*
    IF YOU EVER NEED TO PRINT THE CONTENTS OF A JSON OBJECT!
    Copy past the follwoing
    (don't forget to fill in <rapidjson::Document object> with your object):

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer( buffer );
    <rapidjson::Document object>.Accept( writer );
    std::cout << buffer.GetString() << std::endl;
*/


TEST_CASE( "The encode_json can encode Hailo Objects to a JSON object", "[encode_json]" ) {
    // Create a dummy roi
    HailoBBox main_bbox = HailoBBox(0, 0, 1, 1);

    SECTION( "Detections are encoded with all their properties." ) {
        // Create a main roi to fill with a detection
        HailoROI main_roi = HailoROI(main_bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);

        // Create an add a test detection
        HailoBBox car_bbox = HailoBBox(0.1, 0.8, 0.3, 0.1);
        HailoDetection car = HailoDetection(car_bbox, "car", 1.0);
        main_roi_ptr->add_object(std::make_shared<HailoDetection>(car));

        // Encode the JSON
        rapidjson::Document doc = encode_json::encode_hailo_roi(main_roi_ptr);

        // Check that the detection is encoded
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["confidence"].GetDouble() == Approx(1.0) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["class_id"].GetInt() == -1 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["label"].GetString() == std::string("car") );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["HailoBBox"]["xmin"].GetDouble() == Approx(0.1) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["HailoBBox"]["ymin"].GetDouble() == Approx(0.8) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["HailoBBox"]["width"].GetDouble() == Approx(0.3) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["HailoBBox"]["height"].GetDouble() == Approx(0.1) );
    }

    SECTION( "Classifications are encoded with all their properties." ) {
        // Create a main roi to fill with random detections
        HailoROI main_roi = HailoROI(main_bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);

        // Create and add a test classification
        HailoClassification license_plate_text = HailoClassification("ocr", "123456789", 1.0);
        main_roi_ptr->add_object(std::make_shared<HailoClassification>(license_plate_text));

        // Encode the JSON
        rapidjson::Document doc = encode_json::encode_hailo_roi(main_roi_ptr);

        // Check that the classification is encoded
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassification"]["confidence"].GetDouble() == Approx(1.0) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassification"]["class_id"].GetInt() == -1 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassification"]["classification_type"].GetString() == std::string("ocr") );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassification"]["label"].GetString() == std::string("123456789") );
    }

    SECTION( "Landmarks are encoded with all their properties." ) {
        // Create a main roi to fill with random detections
        HailoROI main_roi = HailoROI(main_bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);

        // Create and add a test classification
        HailoPoint point0 = HailoPoint(0.1, 0.2, 0.3);
        HailoPoint point1 = HailoPoint(0.4, 0.5, 0.6);
        HailoPoint point2 = HailoPoint(0.7, 0.8, 0.9);
        std::vector<std::pair<int, int>> point_pairs = {{0, 2}};
        HailoLandmarks landmarks = HailoLandmarks("pose", {point0, point1, point2}, 0.5, point_pairs);
        main_roi_ptr->add_object(std::make_shared<HailoLandmarks>(landmarks));

        // Encode the JSON
        rapidjson::Document doc = encode_json::encode_hailo_roi(main_roi_ptr);

        // Check that the landmarks are encoded
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["landmarks_type"].GetString() == std::string("pose") );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["threshold"].GetDouble() == Approx(0.5) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][0]["x"].GetDouble() == Approx(0.1) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][0]["y"].GetDouble() == Approx(0.2) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][0]["confidence"].GetDouble() == Approx(0.3) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][1]["x"].GetDouble() == Approx(0.4) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][1]["y"].GetDouble() == Approx(0.5) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][1]["confidence"].GetDouble() == Approx(0.6) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][2]["x"].GetDouble() == Approx(0.7) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][2]["y"].GetDouble() == Approx(0.8) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][2]["confidence"].GetDouble() == Approx(0.9) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["pairs"][0][0] == 0 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["pairs"][0][1] == 2 );
    }

    SECTION( "Tiles are encoded with all their properties." ) {
        // Create a main roi to fill with random detections
        HailoROI main_roi = HailoROI(main_bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);

        // Create and add a test classification
        HailoBBox tile_bbox = HailoBBox(0.0, 0.0, 0.5, 0.5);
        HailoTileROI tile = HailoTileROI(tile_bbox, 0, 0.1, 0.2, 3, MULTI_SCALE);
        main_roi_ptr->add_object(std::make_shared<HailoTileROI>(tile));

        // Encode the JSON
        rapidjson::Document doc = encode_json::encode_hailo_roi(main_roi_ptr);

        // Check that the tiles are encoded
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoTileROI"]["index"] == 0 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoTileROI"]["layer"] == 3 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoTileROI"]["mode"] == 1 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoTileROI"]["overlap_x_axis"].GetDouble() == Approx(0.1) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoTileROI"]["overlap_y_axis"].GetDouble() == Approx(0.2) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoTileROI"]["HailoBBox"]["xmin"].GetDouble() == Approx(0.0) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoTileROI"]["HailoBBox"]["ymin"].GetDouble() == Approx(0.0) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoTileROI"]["HailoBBox"]["width"].GetDouble() == Approx(0.5) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoTileROI"]["HailoBBox"]["height"].GetDouble() == Approx(0.5) );
    }

    SECTION( "Unique IDs are encoded with all their properties." ) {
        // Create a main roi to fill with random detections
        HailoROI main_roi = HailoROI(main_bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);

        // Create and add a test classification
        HailoUniqueID id0 = HailoUniqueID(1913, TRACKING_ID);
        HailoUniqueID id1 = HailoUniqueID(1996, GLOBAL_ID);
        main_roi_ptr->add_object(std::make_shared<HailoUniqueID>(id0));
        main_roi_ptr->add_object(std::make_shared<HailoUniqueID>(id1));

        // Encode the JSON
        rapidjson::Document doc = encode_json::encode_hailo_roi(main_roi_ptr);

        // Check that the ids are encoded
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoUniqueID"]["unique_id"] == 1913 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoUniqueID"]["mode"] == TRACKING_ID );
        CHECK( doc["HailoROI"]["SubObjects"][1]["HailoUniqueID"]["unique_id"] == 1996 );
        CHECK( doc["HailoROI"]["SubObjects"][1]["HailoUniqueID"]["mode"] == GLOBAL_ID );
    }

    SECTION( "Depth Masks are encoded with all their properties." ) {
        // Create a main roi to fill with random detections
        HailoROI main_roi = HailoROI(main_bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);

        // Create and add a test depth mask
        std::vector<float> data= {0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9};
        hailo_common::add_object(main_roi_ptr, std::make_shared<HailoDepthMask>(std::move(data), 10, 1, 1.0));

        // Encode the JSON
        rapidjson::Document doc = encode_json::encode_hailo_roi(main_roi_ptr);

        // Check that the ids are encoded
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDepthMask"]["mask_width"] == 10 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDepthMask"]["mask_height"] == 1 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDepthMask"]["transparency"].GetDouble() == Approx(1.0) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDepthMask"]["data"][0].GetDouble() == Approx(0.0) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDepthMask"]["data"][1].GetDouble() == Approx(0.1) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDepthMask"]["data"][2].GetDouble() == Approx(0.2) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDepthMask"]["data"][3].GetDouble() == Approx(0.3) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDepthMask"]["data"][4].GetDouble() == Approx(0.4) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDepthMask"]["data"][5].GetDouble() == Approx(0.5) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDepthMask"]["data"][6].GetDouble() == Approx(0.6) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDepthMask"]["data"][7].GetDouble() == Approx(0.7) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDepthMask"]["data"][8].GetDouble() == Approx(0.8) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDepthMask"]["data"][9].GetDouble() == Approx(0.9) );
    }

    SECTION( "Class Masks are encoded with all their properties." ) {
        // Create a main roi to fill with random detections
        HailoROI main_roi = HailoROI(main_bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);

        // Create and add a test depth mask
        std::vector<uint8_t> data= {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        hailo_common::add_object(main_roi_ptr, std::make_shared<HailoClassMask>(std::move(data), 10, 1, 1.0));

        // Encode the JSON
        rapidjson::Document doc = encode_json::encode_hailo_roi(main_roi_ptr);

        // Check that the ids are encoded
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassMask"]["mask_width"] == 10 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassMask"]["mask_height"] == 1 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassMask"]["transparency"].GetDouble() == Approx(1.0) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassMask"]["data"][0].GetDouble() == Approx(0) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassMask"]["data"][1].GetDouble() == Approx(1) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassMask"]["data"][2].GetDouble() == Approx(2) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassMask"]["data"][3].GetDouble() == Approx(3) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassMask"]["data"][4].GetDouble() == Approx(4) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassMask"]["data"][5].GetDouble() == Approx(5) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassMask"]["data"][6].GetDouble() == Approx(6) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassMask"]["data"][7].GetDouble() == Approx(7) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassMask"]["data"][8].GetDouble() == Approx(8) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoClassMask"]["data"][9].GetDouble() == Approx(9) );
    }


    SECTION( "Conf Class Masks are encoded with all their properties." ) {
        // Create a main roi to fill with random detections
        HailoROI main_roi = HailoROI(main_bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);

        // Create and add a test depth mask
        std::vector<float> data= {0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9};
        hailo_common::add_object(main_roi_ptr, std::make_shared<HailoConfClassMask>(std::move(data), 10, 1, 1.0, 11));

        // Encode the JSON
        rapidjson::Document doc = encode_json::encode_hailo_roi(main_roi_ptr);

        // Check that the ids are encoded
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoConfClassMask"]["mask_width"] == 10 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoConfClassMask"]["mask_height"] == 1 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoConfClassMask"]["transparency"].GetDouble() == Approx(1.0) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoConfClassMask"]["class_id"] == 11 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoConfClassMask"]["data"][0].GetDouble() == Approx(0.0) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoConfClassMask"]["data"][1].GetDouble() == Approx(0.1) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoConfClassMask"]["data"][2].GetDouble() == Approx(0.2) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoConfClassMask"]["data"][3].GetDouble() == Approx(0.3) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoConfClassMask"]["data"][4].GetDouble() == Approx(0.4) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoConfClassMask"]["data"][5].GetDouble() == Approx(0.5) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoConfClassMask"]["data"][6].GetDouble() == Approx(0.6) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoConfClassMask"]["data"][7].GetDouble() == Approx(0.7) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoConfClassMask"]["data"][8].GetDouble() == Approx(0.8) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoConfClassMask"]["data"][9].GetDouble() == Approx(0.9) );
    }

    SECTION( "Conf Class Masks are encoded with all their properties." ) {
        // Create a main roi to fill with random detections
        HailoROI main_roi = HailoROI(main_bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);

        // Create and add a test depth mask
        std::vector<float> data= {0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9};
        hailo_common::add_object(main_roi_ptr, std::make_shared<HailoMatrix>(std::move(data), 1, 10, 1));

        // Encode the JSON
        rapidjson::Document doc = encode_json::encode_hailo_roi(main_roi_ptr);

        // Check that the ids are encoded
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoMatrix"]["width"] == 10 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoMatrix"]["height"] == 1 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoMatrix"]["features"] == 1 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoMatrix"]["data"][0].GetDouble() == Approx(0.0) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoMatrix"]["data"][1].GetDouble() == Approx(0.1) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoMatrix"]["data"][2].GetDouble() == Approx(0.2) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoMatrix"]["data"][3].GetDouble() == Approx(0.3) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoMatrix"]["data"][4].GetDouble() == Approx(0.4) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoMatrix"]["data"][5].GetDouble() == Approx(0.5) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoMatrix"]["data"][6].GetDouble() == Approx(0.6) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoMatrix"]["data"][7].GetDouble() == Approx(0.7) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoMatrix"]["data"][8].GetDouble() == Approx(0.8) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoMatrix"]["data"][9].GetDouble() == Approx(0.9) );
    }

}

TEST_CASE( "The encode_json can encode Hailo Objects with complex schemes", "[encode_json]" ) {
    // Create a dummy roi
    HailoBBox main_bbox = HailoBBox(0, 0, 1, 1);

    SECTION( "Nested objects are encoded with all their properties." ) {
        // Create a main roi to fill with random detections
        HailoROI main_roi = HailoROI(main_bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);

        // Create an LPR example: vehicle detection that contains a license plate detection, that contains an ocr classification
        HailoBBox car_bbox = HailoBBox(0.1, 0.8, 0.3, 0.1); // The car bbox
        HailoDetection car = HailoDetection(car_bbox, "car", 1.0);
        HailoBBox license_plate_bbox = HailoBBox(0.15, 0.85, 0.1, 0.05); // The license plate bbox
        HailoDetection license_plate = HailoDetection(license_plate_bbox, "license_plate", 1.0);
        HailoClassification license_plate_text = HailoClassification("ocr", "123456789", 1.0);
        license_plate.add_object(std::make_shared<HailoClassification>(license_plate_text));
        car.add_object(std::make_shared<HailoDetection>(license_plate));
        main_roi_ptr->add_object(std::make_shared<HailoDetection>(car));

        // Encode the JSON
        rapidjson::Document doc = encode_json::encode_hailo_roi(main_roi_ptr);
        
        // Check that the ids are encoded
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["confidence"].GetDouble() == Approx(1.0) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["class_id"].GetInt() == -1 );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["label"].GetString() == std::string("car") );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["HailoBBox"]["xmin"].GetDouble() == Approx(0.1) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["HailoBBox"]["ymin"].GetDouble() == Approx(0.8) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["HailoBBox"]["width"].GetDouble() == Approx(0.3) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["HailoBBox"]["height"].GetDouble() == Approx(0.1) );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["SubObjects"][0]["HailoDetection"]["label"].GetString() == std::string("license_plate") );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["SubObjects"][0]["HailoDetection"]["SubObjects"][0]["HailoClassification"]["label"].GetString() == std::string("123456789") );
        CHECK( doc["HailoROI"]["SubObjects"][0]["HailoDetection"]["SubObjects"][0]["HailoDetection"]["SubObjects"][0]["HailoClassification"]["classification_type"].GetString() == std::string("ocr") );
    }
}