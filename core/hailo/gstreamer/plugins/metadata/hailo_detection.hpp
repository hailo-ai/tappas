/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/**
 * @file hailo_detection.hpp
 * @authors MAD Team.
 **/

#ifndef _HAILO_DETECTION_HPP_
#define _HAILO_DETECTION_HPP_
#include <gst/video/video.h>
#include <memory>
#include <stdexcept>
#include <vector>
#include <map>
#include "tensor_meta.hpp"
#include "hailo_tensor.hpp"

struct DetectionObject
{
    float xmin, ymin, xmax, ymax, height, width, confidence;
    std::string label;
    int class_id;
    GstStructure *landmarks;
    GstStructure *segmentation_data;
    bool is_active;

    DetectionObject(float xmin, float ymin, float height, float width, float confidence, std::string label, int class_id = -1) : xmin(xmin), ymin(ymin), xmax(xmin + width), ymax(ymin + height), height(height), width(width), confidence(confidence), label(label), class_id(class_id),
                                                                                                                                 landmarks(gst_structure_new_empty("landmarks")), segmentation_data(gst_structure_new_empty("segmentation_data")), is_active(true)
    {
    }
    DetectionObject(GstStructure *structure) : landmarks(gst_structure_new_empty("landmarks")), segmentation_data(gst_structure_new_empty("segmentation_data")), is_active(true)
    {
        char *label_as_pointer = NULL;
        (void)gst_structure_get(structure, "x_min", G_TYPE_FLOAT, &xmin, "x_max", G_TYPE_FLOAT, &xmax, "y_min",
                                G_TYPE_FLOAT, &ymin, "y_max", G_TYPE_FLOAT, &ymax,
                                "class_id", G_TYPE_INT, &class_id, "label", G_TYPE_STRING, &label_as_pointer, "confidence", G_TYPE_FLOAT, &confidence, NULL);
        width = xmax - xmin;
        height = ymax - ymin;
        if (label_as_pointer != NULL)
        {
            label = std::string(label_as_pointer);
            g_free(label_as_pointer);
        }
        else
        {
            label = "";
        }
    }

    // default contructor:
    DetectionObject() : xmin(0.0), ymin(0.0), xmax(0.0), ymax(0.0), height(0.0), width(0.0), confidence(0.0), label(""), class_id(-1),
                        landmarks(gst_structure_new_empty("landmarks")), segmentation_data(gst_structure_new_empty("segmentation_data")), is_active(true)
    {
    }

    // Destructor
    ~DetectionObject()
    {
        if (is_active)
        {
            if (nullptr != landmarks)
            {
                destruct_structure(landmarks);
                landmarks = nullptr;
            }
            if (nullptr != segmentation_data)
            {
                destruct_structure(segmentation_data);
                segmentation_data = nullptr;
            }
        }
    }

    // Copy Constructor
    DetectionObject(const DetectionObject &other) : xmin(other.xmin), ymin(other.ymin), xmax(other.xmax), ymax(other.ymax), height(other.height), width(other.width),
                                                    confidence(other.confidence), label(other.label), class_id(other.class_id),
                                                    landmarks(nullptr), segmentation_data(nullptr), is_active(true)
    {
        if (nullptr != other.landmarks)
        {
            landmarks = gst_structure_copy(other.landmarks);
        }
        else
        {
            landmarks = gst_structure_new_empty("landmarks");
        }
        if (nullptr != other.segmentation_data)
        {
            segmentation_data = gst_structure_copy(other.segmentation_data);
        }
        else
        {
            segmentation_data = gst_structure_new_empty("segmentation_data");
        }
    }

    // Copy Assignment
    DetectionObject &operator=(const DetectionObject &other)
    {
        if (this != &other)
        {
            // Free the existing resources.
            if (nullptr != landmarks)
                destruct_structure(landmarks);
            if (nullptr != segmentation_data)
                destruct_structure(segmentation_data);

            // Copy Other
            xmin = other.xmin;
            xmax = other.xmax;
            ymin = other.ymin;
            ymax = other.ymax;
            width = other.width;
            height = other.height;
            class_id = other.class_id;
            confidence = other.confidence;
            label = other.label;
            landmarks = gst_structure_copy(other.landmarks);
            segmentation_data = gst_structure_copy(other.segmentation_data);
            is_active = true;
        }
        return *this;
    }

    // Move Constructor
    DetectionObject(DetectionObject &&other) noexcept : xmin(other.xmin), ymin(other.ymin), xmax(other.xmax), ymax(other.ymax), height(other.height), width(other.width),
                                                        confidence(other.confidence), label(std::move(other.label)), class_id(other.class_id),
                                                        landmarks(nullptr), segmentation_data(nullptr), is_active(true)
    {
        this->landmarks = other.landmarks;
        this->segmentation_data = other.segmentation_data;
        other.landmarks = nullptr;
        other.segmentation_data = nullptr;
        other.is_active = false;
    }

    // Move Assignment
    DetectionObject &operator=(DetectionObject &&other) noexcept
    {
        if (this != &other)
        {
            // Free the existing resources.
            if (nullptr != landmarks)
                destruct_structure(landmarks);
            if (nullptr != segmentation_data)
                destruct_structure(segmentation_data);

            // Copy Other
            xmin = other.xmin;
            xmax = other.xmax;
            ymin = other.ymin;
            ymax = other.ymax;
            width = other.width;
            height = other.height;
            class_id = other.class_id;
            confidence = other.confidence;
            label = other.label;
            landmarks = other.landmarks;
            segmentation_data = other.segmentation_data;
            is_active = true;

            // Free other's resources
            other.landmarks = nullptr;
            other.segmentation_data = nullptr;
            other.is_active = false;
        }
        return *this;
    }

    // GstStructure destruction helper
    void destruct_structure(GstStructure *structure)
    {
        gst_structure_remove_all_fields(structure);
        gst_structure_free(structure);
    }

    // Overload comparison operators
    bool operator<(const DetectionObject &s2) const
    {
        return this->confidence < s2.confidence;
    }

    bool operator>(const DetectionObject &s2) const
    {
        return this->confidence > s2.confidence;
    }
};

class HailoDetection
{
public:
    /**
     * @brief Construct RegionOfInterest instance from GstVideoRegionOfInterestMeta. After this, RegionOfInterest will
     * obtain all tensors (detection & inference results) from GstVideoRegionOfInterestMeta
     * @param meta GstVideoRegionOfInterestMeta containing bounding box information and tensors
     */
    HailoDetection(GstVideoRegionOfInterestMeta *meta) : _meta(meta)
    {
        if (nullptr == _meta)
            throw std::invalid_argument("HailoDetection: meta is nullptr");
    }
    /**
     * @brief Check if this Tensor is detection Tensor (contains detection results)
     * @return True if tensor contains detection results, False otherwise
     */
    bool is_detection(GstStructure *structure) const
    {
        const char *det = "detection";
        const char *name = gst_structure_get_name(structure);
        if (!name)
            throw std::runtime_error("Structure doesn't have name");
        int comp = strcmp(name, det);
        return comp == 0;
    }
    DetectionObject get()
    {
        GstStructure *st = gst_video_region_of_interest_meta_get_param(_meta, "detection");
        DetectionObject detection = DetectionObject(st);
        return detection;
    }
    GstStructure *get_landmarks()
    {
        GstStructure *st = gst_video_region_of_interest_meta_get_param(_meta, "landmarks");
        return st;
    }
    GstStructure *get_segmentation_data()
    {
        GstStructure *st = gst_video_region_of_interest_meta_get_param(_meta, "segmentation_data");
        return st;
    }
    /**
     * @brief GstVideoRegionOfInterestMeta containing fields filled with detection result (produced by gvadetect element
     * in Gstreamer pipeline) and all the additional tensors, describing detection and other inference results (produced
     * by gvainference, gvadetect, gvaclassify in Gstreamer pipeline)
     */
    GstVideoRegionOfInterestMeta *_meta;
};
using HailoDetectionPtr = std::shared_ptr<HailoDetection>;

#endif /* _HAILO_DETECTION_HPP_ */
