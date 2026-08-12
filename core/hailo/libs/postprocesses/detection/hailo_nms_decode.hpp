/**
 * Copyright (c) 2021-2026 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#ifndef _POSTPROCESSES_DETECTION_HAILO_NMS_DECODE_HPP_
#define _POSTPROCESSES_DETECTION_HAILO_NMS_DECODE_HPP_

#include <vector>
#include <string>
#include <iostream>
#include <cstring>
#include "hailo_objects.hpp"
#include "common/structures.hpp"
#include "common/nms.hpp"
#include "common/labels/coco_ninety.hpp"
#include "common/labels/coco_visdrone.hpp"

static const int DEFAULT_MAX_BOXES = 100;
static const float DEFAULT_THRESHOLD = 0.4f;

class HailoNMSDecode
{
private:
    HailoTensorPtr m_nms_output_tensor;
    std::map<uint8_t, std::string> m_labels_dict;
    float m_detection_thr;
    uint32_t m_max_boxes;
    bool m_filter_by_score;
    const hailo_tensor_nms_shape_t m_nms_shape;

    common::hailo_bbox_float32_t dequantize_hailo_bbox(const auto *bbox_struct)
    {
        common::hailo_bbox_float32_t dequant_bbox = {
            .y_min = m_nms_output_tensor->fix_scale(bbox_struct->y_min),
            .x_min = m_nms_output_tensor->fix_scale(bbox_struct->x_min),
            .y_max = m_nms_output_tensor->fix_scale(bbox_struct->y_max),
            .x_max = m_nms_output_tensor->fix_scale(bbox_struct->x_max),
            .score = m_nms_output_tensor->fix_scale(bbox_struct->score)};

        return dequant_bbox;
    }

    void parse_bbox_to_detection_object(auto dequant_bbox, uint32_t class_index,
        std::vector<HailoDetection> &objects)
    {
        float confidence = CLAMP(dequant_bbox.score, 0.0f, 1.0f);
        if (!m_filter_by_score || (dequant_bbox.score > m_detection_thr)) {
            float32_t w = 0.0f, h = 0.0f;
            std::tie(w, h) = get_shape(&dequant_bbox);
            objects.push_back(HailoDetection(
                HailoBBox(dequant_bbox.x_min, dequant_bbox.y_min, w, h),
                class_index, m_labels_dict[class_index], confidence));
        }
    }

    std::pair<float, float> get_shape(auto *bbox_struct)
    {
        float32_t w = bbox_struct->x_max - bbox_struct->x_min;
        float32_t h = bbox_struct->y_max - bbox_struct->y_min;
        return {w, h};
    }

public:
    HailoNMSDecode(HailoTensorPtr tensor, const std::map<uint8_t, std::string> &labels_dict,
        float detection_thr = DEFAULT_THRESHOLD, uint32_t max_boxes = DEFAULT_MAX_BOXES,
        bool filter_by_score = false):
            m_nms_output_tensor(tensor), m_labels_dict(labels_dict),
            m_detection_thr(detection_thr), m_max_boxes(max_boxes),
            m_filter_by_score(filter_by_score), m_nms_shape(tensor->nms_shape())
    {
        if (!tensor->format().is_nms)
            throw std::invalid_argument("Output tensor " + m_nms_output_tensor->name() + " is not an NMS type");
    };

    template <typename T, typename BBoxType>
    std::vector<HailoDetection> decode()
    {
        /*
        NMS output decode method
        ------------------------

        decodes the nms buffer received from the output tensor of the network.
        returns a vector of DetectonObject filtered by the detection threshold.

        The data is sorted by the number of the classes.
        for each class - first comes the number of boxes in the class, then the boxes one after the other,
        each box contains x_min, y_min, x_max, y_max and score (uint16_t\float32 each) and can be casted
        to common::hailo_bbox_t struct (5*uint16_t).
        means that a frame size of one class is sizeof(bbox_count) + bbox_count * sizeof(common::hailo_bbox_t).
        and the actual size of the data is (frame size of one class)*number of classes.

        If the data comes after quantization - so dequantization to float32 is needed.

        As an example - quantized data buffer of a frame that contains a person and two dogs:
        (person class id = 1, dog class id = 18)

        1 107 96 143 119 172 0 0 0 0 0 0 0 0 0 0 0 0 0
        0 0 2 123 124 140 150 92 112 125 138 147 91 0 0
        0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
        0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
        0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0

        taking the dogs as example - 2 123 124 140 150 92 112 125 138 147 91
        can be splitted to two different boxes
        common::hailo_bbox_t st_1 = 123 124 140 150 92
        common::hailo_bbox_t st_2 = 112 125 138 147 91
        now after dequntization of st_1 - we get common::hailo_bbox_float32_t:
        ymin = 0.551805 xmin = 0.389635 ymax = 0.741805 xmax = 0.561974 score = 0.95
        */

        if (!m_nms_output_tensor)
            return std::vector<HailoDetection>{};

        std::vector<HailoDetection> objects;
        objects.reserve(m_max_boxes);
        uint32_t max_bboxes_per_class = m_nms_shape.max_bboxes_per_class;
        uint32_t num_of_classes = m_nms_shape.number_of_classes;
        size_t buffer_offset = 0;
        uint8_t *buffer = m_nms_output_tensor->data();
        for (size_t class_id = 0; class_id < num_of_classes; class_id++) {
            float32_t bbox_count = 0;
            memcpy(&bbox_count, buffer + buffer_offset, sizeof(bbox_count));
            buffer_offset += sizeof(bbox_count);

            if (0 == bbox_count)
                continue;
            if (max_bboxes_per_class < bbox_count)
                throw std::runtime_error("Runtime error - Got more than the maximum bboxes per class in the nms buffer");

            for (size_t bbox_index = 0; bbox_index < static_cast<uint32_t>(bbox_count); bbox_index++) {
                if (std::is_same<T, uint16_t>::value) {
                    auto *bbox = reinterpret_cast<common::hailo_bbox_float32_t *>(&buffer[buffer_offset]);
                    parse_bbox_to_detection_object(*bbox, class_id + 1, objects);
                    buffer_offset += sizeof(common::hailo_bbox_float32_t);
                } else {
                    auto *bbox_struct = reinterpret_cast<BBoxType *>(&buffer[buffer_offset]);
                    parse_bbox_to_detection_object(*bbox_struct, class_id + 1, objects);
                    buffer_offset += sizeof(BBoxType);
                }
            }
        }
        return objects;
    }
};

inline std::vector<HailoDetection> nms_decode(HailoTensorPtr tensor,
    const std::map<uint8_t, std::string> &labels,
    float detection_thr = 0.4f,
    uint32_t max_boxes = 100,
    bool filter_by_score = false)
{
    auto post = HailoNMSDecode(tensor, labels, detection_thr, max_boxes, filter_by_score);
    return post.decode<float32_t, common::hailo_bbox_float32_t>();
}

inline void nms_decode_to_roi(HailoROIPtr roi, const std::string &layer_name,
    const std::map<uint8_t, std::string> &labels,
    float detection_thr = 0.4f,
    uint32_t max_boxes = 100,
    bool filter_by_score = false)
{
    if (!roi->has_tensors()) {
        return;
    }
    auto detections = nms_decode(roi->get_tensor(layer_name), labels, detection_thr, max_boxes, filter_by_score);
    hailo_common::add_detections(roi, detections);
}

#endif  // _POSTPROCESSES_DETECTION_HAILO_NMS_DECODE_HPP_
