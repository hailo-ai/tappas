/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
// Catch2 includes
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"       // This includes the catch2 header-only library, no further includes needed for catch2

// General cpp includes
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

// Tappas includes
#include "common/resources/test_jsons/unit_test_jsons.hpp"
#include "decode_json.hpp"
#include "hailo_objects.hpp"
#include "hailo_common.hpp"

// Open source includes
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filereadstream.h"

/*
    IF YOU EVER NEED TO PRINT THE CONTENTS OF A JSON OBJECT!
    Copy past the follwoing
    (don't forget to fill in <rapidjson::Document object> with your object):

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer( buffer );
    <rapidjson::Document object>.Accept( writer );
    std::cout << buffer.GetString() << std::endl;
*/

rapidjson::Document read_file(std::string filename)
{
    FILE* fp = fopen(filename.c_str(), "rb"); // non-Windows use "r"
    
    char readBuffer[65536];
    rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    
    rapidjson::Document d;
    d.ParseStream(is);
    
    fclose(fp);
    return d;
}


TEST_CASE( "The decode_json can decode JSON objects into Hailo Objects", "[decode_json]" ) {
    // Create a dummy roi
    HailoBBox main_bbox = HailoBBox(0, 0, 1, 1);

    SECTION( "Detections are decoded with all their properties." ) {
        // Create a main roi to fill with a detection
        HailoROI main_roi = HailoROI(main_bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);

        // Prepare a sample detection json
        rapidjson::Document test_json = read_file(DETECTION_JSON);

        // Decode the JSON
        decode_json::decode_hailo_roi(test_json, main_roi_ptr);
        HailoDetectionPtr detection = hailo_common::get_hailo_detections(main_roi_ptr)[0];
        HailoBBox decoded_bbox = detection->get_bbox();

        // // Check that the detection is decoded
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoDetection"]["confidence"].GetDouble() == detection->get_confidence() );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoDetection"]["class_id"].GetInt() == detection->get_class_id() );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoDetection"]["label"].GetString() == detection->get_label() );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoDetection"]["HailoBBox"]["xmin"].GetDouble() == Approx(decoded_bbox.xmin()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoDetection"]["HailoBBox"]["ymin"].GetDouble() == Approx(decoded_bbox.ymin()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoDetection"]["HailoBBox"]["width"].GetDouble() == Approx(decoded_bbox.width()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoDetection"]["HailoBBox"]["height"].GetDouble() == Approx(decoded_bbox.height()) );
    }

    SECTION( "Classifications are decoded with all their properties." ) {
        // Create a main roi to fill with random detections
        HailoROI main_roi = HailoROI(main_bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);

        // Prepare a sample classification json
        rapidjson::Document test_json = read_file(CLASSIFICATION_JSON);

        // Decode the JSON
        decode_json::decode_hailo_roi(test_json, main_roi_ptr);
        HailoClassificationPtr classification = hailo_common::get_hailo_classifications(main_roi_ptr)[0];

        // Check that the classification is decoded
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoClassification"]["confidence"].GetDouble() == classification->get_confidence() );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoClassification"]["class_id"].GetInt() == classification->get_class_id() );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoClassification"]["classification_type"].GetString() == classification->get_classification_type() );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoClassification"]["label"].GetString() == classification->get_label() );
    }

    SECTION( "Landmarks are decoded with all their properties." ) {
        // Create a main roi to fill with random detections
        HailoROI main_roi = HailoROI(main_bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);

        // Prepare a sample landmarks json
        rapidjson::Document test_json = read_file(LANDMARKS_JSON);

        // Decode the JSON
        decode_json::decode_hailo_roi(test_json, main_roi_ptr);
        HailoLandmarksPtr landmarks = hailo_common::get_hailo_landmarks(main_roi_ptr)[0];
        std::vector<HailoPoint> decoded_points = landmarks->get_points();
        std::vector<std::pair<int, int>> decoded_pairs = landmarks->get_pairs();

        // Check that the landmarks are decoded
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["landmarks_type"].GetString() == landmarks->get_landmarks_type() );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["threshold"].GetDouble() == landmarks->get_threshold() );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][0]["x"].GetDouble() == Approx(decoded_points[0].x()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][0]["y"].GetDouble() == Approx(decoded_points[0].y()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][0]["confidence"].GetDouble() == Approx(decoded_points[0].confidence()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][1]["x"].GetDouble() == Approx(decoded_points[1].x()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][1]["y"].GetDouble() == Approx(decoded_points[1].y()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][1]["confidence"].GetDouble() == Approx(decoded_points[1].confidence()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][2]["x"].GetDouble() == Approx(decoded_points[2].x()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][2]["y"].GetDouble() == Approx(decoded_points[2].y()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["points"][2]["confidence"].GetDouble() == Approx(decoded_points[2].confidence()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["pairs"][0][0] == decoded_pairs[0].first );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoLandmarks"]["pairs"][0][1] == decoded_pairs[0].second );
    }

    SECTION( "Tiles are decoded with all their properties." ) {
        // Create a main roi to fill with random detections
        HailoROI main_roi = HailoROI(main_bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);

        // Prepare a sample tile json
        rapidjson::Document test_json = read_file(TILE_JSON);

        // Decode the JSON
        decode_json::decode_hailo_roi(test_json, main_roi_ptr);
        HailoTileROIPtr tile = hailo_common::get_hailo_tiles(main_roi_ptr)[0];
        HailoBBox decoded_bbox = tile->get_bbox();

        // Check that the tiles are decoded
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoTileROI"]["index"] == tile->get_index() );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoTileROI"]["layer"] == tile->get_layer() );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoTileROI"]["mode"] == tile->get_mode() );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoTileROI"]["overlap_x_axis"].GetDouble() == Approx(tile->get_overlap_x_axis()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoTileROI"]["overlap_y_axis"].GetDouble() == Approx(tile->get_overlap_y_axis()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoTileROI"]["HailoBBox"]["xmin"].GetDouble() == Approx(decoded_bbox.xmin()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoTileROI"]["HailoBBox"]["ymin"].GetDouble() == Approx(decoded_bbox.ymin()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoTileROI"]["HailoBBox"]["width"].GetDouble() == Approx(decoded_bbox.width()) );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoTileROI"]["HailoBBox"]["height"].GetDouble() == Approx(decoded_bbox.height()) );
    }

    SECTION( "Unique IDs are decoded with all their properties." ) {
        // Create a main roi to fill with random detections
        HailoROI main_roi = HailoROI(main_bbox);
        HailoROIPtr main_roi_ptr = std::make_shared<HailoROI>(main_roi);

        // Prepare a sample classification json
        rapidjson::Document test_json = read_file(UNIQUE_ID_JSON);

        // Decode the JSON
        decode_json::decode_hailo_roi(test_json, main_roi_ptr);
        std::vector<HailoUniqueIDPtr> unique_ids = hailo_common::get_hailo_unique_id(main_roi_ptr);

        // Check that the ids are decoded
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoUniqueID"]["unique_id"] == unique_ids[0]->get_id() );
        CHECK( test_json["HailoROI"]["SubObjects"][0]["HailoUniqueID"]["mode"] == unique_ids[0]->get_mode() );
        CHECK( test_json["HailoROI"]["SubObjects"][1]["HailoUniqueID"]["unique_id"] == unique_ids[1]->get_id() );
        CHECK( test_json["HailoROI"]["SubObjects"][1]["HailoUniqueID"]["mode"] == unique_ids[1]->get_mode() );
    }
}