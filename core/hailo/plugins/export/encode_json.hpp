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

namespace encode_json
{
    void encode_bbox(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoBBox bbox);
    void encode_point(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoPoint point);
    void encode_detection(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoDetectionPtr detection);
    void encode_classification(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoClassificationPtr classification);
    void encode_landmarks(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoLandmarksPtr landmarks);
    void encode_tile(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoTileROIPtr tile);
    void encode_unique_id(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoUniqueIDPtr id);
    rapidjson::Value encode_mask(rapidjson::Document::AllocatorType& allocator, HailoMaskPtr mask);
    void encode_depth_mask(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoDepthMaskPtr mask);
    void encode_class_mask(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoClassMaskPtr mask);
    void encode_conf_class_mask(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoConfClassMaskPtr mask);
    void encode_matrix(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoMatrixPtr matrix);
    void encode_hailo_objects_to_json(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoROIPtr roi);
}


namespace encode_json
{
    inline void encode_bbox(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoBBox bbox)
    {
        rapidjson::Value hailo_bbox( rapidjson::kObjectType );
        hailo_bbox.AddMember( "xmin", bbox.xmin(), allocator );
        hailo_bbox.AddMember( "ymin", bbox.ymin(), allocator );
        hailo_bbox.AddMember( "width", bbox.width(), allocator );
        hailo_bbox.AddMember( "height", bbox.height(), allocator );
        object_json.AddMember("HailoBBox", hailo_bbox, allocator);
    }

    inline void encode_point(rapidjson::Value& array_json, rapidjson::Document::AllocatorType& allocator, HailoPoint point)
    {
        rapidjson::Value hailo_point( rapidjson::kObjectType );
        hailo_point.AddMember( "x", point.x(), allocator );
        hailo_point.AddMember( "y", point.y(), allocator );
        hailo_point.AddMember( "confidence", point.confidence(), allocator );
        array_json.PushBack(hailo_point, allocator);
    }

    inline void encode_detection(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoDetectionPtr detection)
    {
        rapidjson::Value entry_object( rapidjson::kObjectType );

        // Prepare the json values of the feilds
        rapidjson::Value label( detection->get_label(), allocator );   // Label
        rapidjson::Value class_id( detection->get_class_id());         // Class ID
        rapidjson::Value confidence( detection->get_confidence());     // Confidence

        // Add feilds to the detection entry
        entry_object.AddMember( "label", label, allocator );
        entry_object.AddMember( "class_id", class_id, allocator );
        entry_object.AddMember( "confidence", confidence, allocator );
        encode_bbox(entry_object, allocator, detection->get_bbox());

        // Recurse this object
        encode_hailo_objects_to_json(entry_object, allocator, detection);

        // Add this detection object to the parent
        object_json.AddMember("HailoDetection", entry_object, allocator);
    }

    inline void encode_classification(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoClassificationPtr classification)
    {
        rapidjson::Value entry_object( rapidjson::kObjectType );

        // Prepare the json values of the feilds
        rapidjson::Value label( classification->get_label(), allocator );   // Label
        rapidjson::Value classification_type( classification->get_classification_type(), allocator );   // classification_type
        rapidjson::Value class_id( classification->get_class_id());         // Class ID
        rapidjson::Value confidence( classification->get_confidence());     // Confidence

        // Add feilds to the classification entry
        entry_object.AddMember( "label", label, allocator );
        entry_object.AddMember( "classification_type", classification_type, allocator );
        entry_object.AddMember( "class_id", class_id, allocator );
        entry_object.AddMember( "confidence", confidence, allocator );

        // Add this classification object to the parent
        object_json.AddMember("HailoClassification", entry_object, allocator);
    }

    inline void encode_landmarks(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoLandmarksPtr landmarks)
    {
        rapidjson::Value entry_object( rapidjson::kObjectType );

        // Prepare the json values of the feilds
        rapidjson::Value landmarks_type( landmarks->get_landmarks_type(), allocator );   // landmarks_type
        rapidjson::Value threshold( landmarks->get_threshold());     // threshold

        // Add feilds to the landmarks entry
        entry_object.AddMember( "landmarks_type", landmarks_type, allocator );
        entry_object.AddMember( "threshold", threshold, allocator );
        rapidjson::Value point_array(rapidjson::kArrayType);
        for(auto &point : landmarks->get_points())
            encode_point(point_array, allocator, point);
        entry_object.AddMember("points", point_array, allocator);
        rapidjson::Value pair_array(rapidjson::kArrayType);
        for(auto &pair : landmarks->get_pairs())
        {
            rapidjson::Value pair_object(rapidjson::kArrayType);
            pair_object.PushBack(rapidjson::Value(pair.first), allocator);
            pair_object.PushBack(rapidjson::Value(pair.second), allocator);
            pair_array.PushBack(pair_object, allocator);
        }
        entry_object.AddMember("pairs", pair_array, allocator);

        // Add this landmarks object to the parent
        object_json.AddMember("HailoLandmarks", entry_object, allocator);
    }

    inline void encode_tile(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoTileROIPtr tile)
    {
        rapidjson::Value entry_object( rapidjson::kObjectType );

        // Prepare the json values of the feilds
        rapidjson::Value index( tile->get_index());
        rapidjson::Value layer( tile->get_layer());
        rapidjson::Value mode( tile->get_mode());
        rapidjson::Value overlap_x_axis( tile->get_overlap_x_axis());
        rapidjson::Value overlap_y_axis( tile->get_overlap_y_axis());

        // Add feilds to the tile entry
        entry_object.AddMember( "index", index, allocator );
        entry_object.AddMember( "layer", layer, allocator );
        entry_object.AddMember( "mode", mode, allocator );
        entry_object.AddMember( "overlap_x_axis", overlap_x_axis, allocator );
        entry_object.AddMember( "overlap_y_axis", overlap_y_axis, allocator );
        encode_bbox(entry_object, allocator, tile->get_bbox());

        // Recurse this object
        encode_hailo_objects_to_json(entry_object, allocator, tile);

        // Add this tile object to the parent
        object_json.AddMember("HailoTileROI", entry_object, allocator);
    }

    inline void encode_unique_id(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoUniqueIDPtr id)
    {
        rapidjson::Value entry_object( rapidjson::kObjectType );

        // Prepare the json values of the feilds
        rapidjson::Value uid( id->get_id());
        rapidjson::Value mode( id->get_mode());

        // Add feilds to the ID entry
        entry_object.AddMember( "unique_id", uid, allocator );
        entry_object.AddMember( "mode", mode, allocator );

        // Add this unique id object to the parent
        object_json.AddMember("HailoUniqueID", entry_object, allocator);
    }

    template <class T>
    rapidjson::Value encode_mask(rapidjson::Document::AllocatorType& allocator, T mask)
    {
        rapidjson::Value entry_object( rapidjson::kObjectType );

        // Prepare the json values of the feilds
        rapidjson::Value mask_width( mask->get_width() );
        rapidjson::Value mask_height( mask->get_height() );
        rapidjson::Value transparency( mask->get_transparency() );

        // Add feilds to the mask entry
        entry_object.AddMember( "mask_width", mask_width, allocator );
        entry_object.AddMember( "mask_height", mask_height, allocator );
        entry_object.AddMember( "transparency", transparency, allocator );

        rapidjson::Value data_array(rapidjson::kArrayType);
        auto data = mask->get_data();
        for (uint i=0; i < data.size(); i++)
            data_array.PushBack(rapidjson::Value(data[i]), allocator);
        entry_object.AddMember( "data", data_array, allocator );

        return entry_object;
    }

    inline void encode_depth_mask(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoDepthMaskPtr mask)
    {
        rapidjson::Value entry_object = encode_mask(allocator, mask);

        // Add this mask object to the parent
        object_json.AddMember("HailoDepthMask", entry_object, allocator);
    }

    inline void encode_class_mask(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoClassMaskPtr mask)
    {
        rapidjson::Value entry_object = encode_mask(allocator, mask);

        // Add this mask object to the parent
        object_json.AddMember("HailoClassMask", entry_object, allocator);
    }

    inline void encode_conf_class_mask(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoConfClassMaskPtr mask)
    {
        rapidjson::Value entry_object = encode_mask(allocator, mask);

        rapidjson::Value class_id( mask->get_class_id() );
        entry_object.AddMember( "class_id", class_id, allocator );

        // Add this mask object to the parent
        object_json.AddMember("HailoConfClassMask", entry_object, allocator);
    }

    inline void encode_matrix(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoMatrixPtr matrix)
    {
        rapidjson::Value entry_object( rapidjson::kObjectType );

        // Prepare the json values of the feilds
        rapidjson::Value width( matrix->width());
        rapidjson::Value height( matrix->height());
        rapidjson::Value features( matrix->features());

        // Add feilds to the matrix entry
        entry_object.AddMember( "width", width, allocator );
        entry_object.AddMember( "height", height, allocator );
        entry_object.AddMember( "features", features, allocator );

        rapidjson::Value data_array(rapidjson::kArrayType);
        std::vector<float> data = matrix->get_data();
        for (uint i=0; i < data.size(); i++)
            data_array.PushBack(rapidjson::Value(data[i]), allocator);
        entry_object.AddMember( "data", data_array, allocator );

        // Add this matrix object to the parent
        object_json.AddMember("HailoMatrix", entry_object, allocator);
    }

    inline rapidjson::Document encode_hailo_face_recognition_result(HailoMatrixPtr matrix, const char* name)
    {
        // Create the JSON DOM, get it's allocator
        rapidjson::Document document;
        document.SetObject();
        rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

        rapidjson::Value object_array(rapidjson::kArrayType);
        rapidjson::Value object_json( rapidjson::kObjectType );  // top level roi
        
        rapidjson::Value object_member(rapidjson::kObjectType);  //array member
        encode_matrix(object_member, allocator, matrix);
        object_array.PushBack(object_member, allocator);
        object_json.AddMember("Name", rapidjson::Value(name, sizeof(name)), allocator);
        object_json.AddMember("Embeddings", object_array, allocator);
        document.AddMember("FaceRecognition", object_json, allocator);

        return document;
    }

    inline void encode_hailo_objects_to_json(rapidjson::Value& object_json, rapidjson::Document::AllocatorType& allocator, HailoROIPtr roi)
    {
        rapidjson::Value object_array(rapidjson::kArrayType);
        for (auto obj : roi->get_objects())
        {
            rapidjson::Value object_member(rapidjson::kObjectType);  //array member
            switch (obj->get_type())
            {
                case HAILO_DETECTION:
                {
                    HailoDetectionPtr detection = std::dynamic_pointer_cast<HailoDetection>(obj);
                    encode_detection(object_member, allocator, detection);
                    object_array.PushBack(object_member, allocator);
                    break;
                }
                case HAILO_CLASSIFICATION:
                {
                    HailoClassificationPtr classification = std::dynamic_pointer_cast<HailoClassification>(obj);
                    encode_classification(object_member, allocator, classification);
                    object_array.PushBack(object_member, allocator);
                    break;
                }
                case HAILO_LANDMARKS:
                {
                    HailoLandmarksPtr landmarks = std::dynamic_pointer_cast<HailoLandmarks>(obj);
                    encode_landmarks(object_member, allocator, landmarks);
                    object_array.PushBack(object_member, allocator);
                    break;
                }
                case HAILO_TILE:
                {
                    HailoTileROIPtr tile = std::dynamic_pointer_cast<HailoTileROI>(obj);
                    encode_tile(object_member, allocator, tile);
                    object_array.PushBack(object_member, allocator);
                    break;
                }
                case HAILO_UNIQUE_ID:
                {
                    HailoUniqueIDPtr id = std::dynamic_pointer_cast<HailoUniqueID>(obj);
                    encode_unique_id(object_member, allocator, id);
                    object_array.PushBack(object_member, allocator);
                    break;
                }
                case HAILO_DEPTH_MASK:
                {
                    HailoDepthMaskPtr mask = std::dynamic_pointer_cast<HailoDepthMask>(obj);
                    encode_depth_mask(object_member, allocator, mask);
                    object_array.PushBack(object_member, allocator);
                    break;
                }
                case HAILO_CLASS_MASK:
                {
                    HailoClassMaskPtr mask = std::dynamic_pointer_cast<HailoClassMask>(obj);
                    encode_class_mask(object_member, allocator, mask);
                    object_array.PushBack(object_member, allocator);
                    break;
                }
                case HAILO_CONF_CLASS_MASK:
                {
                    HailoConfClassMaskPtr mask = std::dynamic_pointer_cast<HailoConfClassMask>(obj);
                    encode_conf_class_mask(object_member, allocator, mask);
                    object_array.PushBack(object_member, allocator);
                    break;
                }
                case HAILO_MATRIX:
                {
                    HailoMatrixPtr matrix = std::dynamic_pointer_cast<HailoMatrix>(obj);
                    encode_matrix(object_member, allocator, matrix);
                    object_array.PushBack(object_member, allocator);
                    break;
                }
                default:
                    // continue
                break;
            }
        }
        object_json.AddMember("SubObjects", object_array, allocator);
    }

    inline rapidjson::Document encode_hailo_roi(HailoROIPtr roi)
    {
        // Create the JSON DOM, get it's allocator
        rapidjson::Document document;
        document.SetObject();
        rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

        rapidjson::Value object_json( rapidjson::kObjectType );  // top level roi

        encode_bbox(object_json, allocator, roi->get_bbox());
        encode_hailo_objects_to_json(object_json, allocator, roi);
        document.AddMember("HailoROI", object_json, allocator);

        return document;
    }

}