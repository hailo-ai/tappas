/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#pragma once

// General cpp includes
#include <iostream>

// Tappas includes
#include "hailo_objects.hpp"
#include "hailo_common.hpp"

// Open source includes
#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/schema.h"
#include "rapidjson/prettywriter.h"

namespace decode_json
{
    int decode_enum(std::string class_name);
    HailoBBox decode_bbox(rapidjson::Value& object_json);
    HailoPoint decode_point(rapidjson::Value& object_json);
    void decode_detection(rapidjson::Value& object_json, HailoROIPtr roi);
    void decode_classification(rapidjson::Value& object_json, HailoROIPtr roi);
    void decode_landmarks(rapidjson::Value& object_json, HailoROIPtr roi);
    void decode_tile(rapidjson::Value& object_json, HailoROIPtr roi);
    void decode_unique_id(rapidjson::Value& object_json, HailoROIPtr roi);
    void decode_hailo_objects_from_json(rapidjson::Value& object_json, HailoROIPtr roi);
}


namespace decode_json
{
    inline int decode_enum(std::string class_name)
    {
        if (class_name == "HailoClassification") return HAILO_CLASSIFICATION;
        if (class_name == "HailoDetection") return HAILO_DETECTION;
        if (class_name == "HailoLandmarks") return HAILO_LANDMARKS;
        if (class_name == "HailoTileROI") return HAILO_TILE;
        if (class_name == "HailoUniqueID") return HAILO_UNIQUE_ID;
        return -1;
    }

    inline HailoBBox decode_bbox(rapidjson::Value& object_json)
    {
        return HailoBBox(object_json["xmin"].GetFloat(),
                         object_json["ymin"].GetFloat(),
                         object_json["width"].GetFloat(),
                         object_json["height"].GetFloat());
    }

    inline HailoPoint decode_point(rapidjson::Value& object_json)
    {
        return HailoPoint(object_json["x"].GetFloat(),
                          object_json["y"].GetFloat(),
                          object_json["confidence"].GetFloat());
    }

    inline void decode_detection(rapidjson::Value& object_json, HailoROIPtr roi)
    {
        HailoBBox bbox = decode_bbox(object_json["HailoBBox"]);

        HailoDetectionPtr detection = std::make_shared<HailoDetection>(bbox,
                                                                    object_json["class_id"].GetInt(),
                                                                    object_json["label"].GetString(),
                                                                    object_json["confidence"].GetFloat());

        // Add this detection object to the parent
        roi->add_object(detection);

        // Recurse this object
        decode_hailo_objects_from_json(object_json["SubObjects"], detection);
    }

    inline void decode_classification(rapidjson::Value& object_json, HailoROIPtr roi)
    {
        // Add this classification object to the parent
        roi->add_object(std::make_shared<HailoClassification>(object_json["classification_type"].GetString(),
                                                                object_json["class_id"].GetInt(),
                                                                object_json["label"].GetString(),
                                                                object_json["confidence"].GetFloat()));
    }

    inline void decode_matrix(rapidjson::Value& object_json, HailoROIPtr roi)
    {
        std::vector<float> matrix_data;
        for (rapidjson::Value& entry : object_json["data"].GetArray()) {
            matrix_data.emplace_back(entry.GetFloat());
        }
        // Add this matrix object to the parent
        roi->add_object(std::make_shared<HailoMatrix>(matrix_data,
                                                      object_json["height"].GetInt(),
                                                      object_json["width"].GetInt(),
                                                      object_json["features"].GetInt()));
    }

    inline void decode_landmarks(rapidjson::Value& object_json, HailoROIPtr roi)
    {
        // Decode points
        std::vector<HailoPoint> decoded_points;
        for (rapidjson::Value& entry : object_json["points"].GetArray()) {
            decoded_points.emplace_back(decode_point(entry));
        }

        // Decode point pairs
        std::vector<std::pair<int, int>> decoded_pairs;
        for (rapidjson::Value& entry : object_json["pairs"].GetArray()) {
            auto pair = entry.GetArray();
            decoded_pairs.push_back(std::make_pair(pair[0].GetInt(), pair[1].GetInt()));
        }

        // Add this landmarks object to the parent
        roi->add_object(std::make_shared<HailoLandmarks>(object_json["landmarks_type"].GetString(),
                                                        decoded_points,
                                                        object_json["threshold"].GetFloat(),
                                                        decoded_pairs));
    }

    inline void decode_tile(rapidjson::Value& object_json, HailoROIPtr roi)
    {
        HailoBBox bbox = decode_bbox(object_json["HailoBBox"]);

        HailoTileROIPtr tile = std::make_shared<HailoTileROI>(bbox,
                                                            object_json["index"].GetInt(),
                                                            object_json["overlap_x_axis"].GetFloat(),
                                                            object_json["overlap_y_axis"].GetFloat(),
                                                            object_json["layer"].GetInt(),
                                                            (hailo_tiling_mode_t)object_json["mode"].GetInt());

        // Add this tile object to the parent
        roi->add_object(tile);

        // Recurse this object
        decode_hailo_objects_from_json(object_json["SubObjects"], tile);
    }

    inline void decode_unique_id(rapidjson::Value& object_json, HailoROIPtr roi)
    {
        // Add this classification object to the parent
        roi->add_object(std::make_shared<HailoUniqueID>(object_json["unique_id"].GetInt(),
                                                        (hailo_unique_id_mode_t)object_json["mode"].GetInt()));
    }

    inline void decode_hailo_objects_from_json(rapidjson::Value& object_json, HailoROIPtr roi)
    {
        assert(object_json.IsArray());
        for (rapidjson::Value& entry : object_json.GetArray()) {
            for(rapidjson::Value::ConstMemberIterator iter=entry.MemberBegin(); iter != entry.MemberEnd(); iter++)
            {
                switch (decode_enum(iter->name.GetString()))
                {
                    case HAILO_DETECTION:
                    {
                        decode_detection(entry["HailoDetection"], roi);
                        break;
                    }
                    case HAILO_CLASSIFICATION:
                    {
                        decode_classification(entry["HailoClassification"], roi);
                        break;
                    }
                    case HAILO_LANDMARKS:
                    {
                        decode_landmarks(entry["HailoLandmarks"], roi);
                        break;
                    }
                    case HAILO_TILE:
                    {
                        decode_tile(entry["HailoTileROI"], roi);
                        break;
                    }
                    case HAILO_MATRIX:
                    {
                        decode_matrix(entry["HailoMatrix"], roi);
                        break;
                    }
                    case HAILO_UNIQUE_ID:
                    {
                        decode_unique_id(entry["HailoUniqueID"], roi);
                        break;
                    }
                    default:
                        // continue
                    break;
                }
            }
        }
    }

    inline void decode_hailo_roi(rapidjson::Document& document, HailoROIPtr roi)
    {
        assert(document.IsObject());
        assert(document.HasMember("HailoROI"));
        assert(document["HailoROI"].HasMember("SubObjects"));
        decode_hailo_objects_from_json(document["HailoROI"]["SubObjects"], roi);
    }

    inline void decode_hailo_face_recognition_result(rapidjson::Value object_json, HailoROIPtr roi, std::vector<std::string> &embedding_names)
    {
        assert(object_json.IsArray());
        for (rapidjson::Value& entry : object_json.GetArray()) 
        {
            if (entry.HasMember("FaceRecognition"))
            {
                assert(entry["FaceRecognition"].HasMember("Embeddings"));
                embedding_names.emplace_back(entry["FaceRecognition"]["Name"].GetString());
                for (rapidjson::Value& embedding_entry : entry["FaceRecognition"]["Embeddings"].GetArray())
                    decode_matrix(embedding_entry["HailoMatrix"], roi);
            }
        }
    }

}