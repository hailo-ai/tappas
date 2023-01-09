/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include <vector>
#include <string>
#include "mobilenet_ssd.hpp"
#include "hailo_objects.hpp"
#include "common/structures.hpp"
#include "common/nms.hpp"
#include "common/labels/coco_ninety.hpp"
#include "common/labels/coco_visdrone.hpp"

static const int DEFAULT_MAX_BOXES = 100;
static const float DEFAULT_MOBILENET_THRESHOLD = 0.4;
static const std::string DEFAULT_NMS_OUTPUT_LAYER = "ssd_mobilenet_v1/nms1";

class MobilenetSSDPost
{
private:
    HailoTensorPtr _nms_output_tensor;
    std::map<uint8_t, std::string> labels_dict;
    float _detection_thr;
    uint _max_boxes;
    const hailo_vstream_info_t _vstream_info;

    common::hailo_bbox_float32_t dequantize_hailo_bbox(const common::hailo_bbox_t *bbox_struct)
    {
        // Dequantization of common::hailo_bbox_t (uint16_t) to common::hailo_bbox_float32_t (float32_t)
        common::hailo_bbox_float32_t dequant_bbox = {
            .y_min = _nms_output_tensor->fix_scale(bbox_struct->y_min),
            .x_min = _nms_output_tensor->fix_scale(bbox_struct->x_min),
            .y_max = _nms_output_tensor->fix_scale(bbox_struct->y_max),
            .x_max = _nms_output_tensor->fix_scale(bbox_struct->x_max),
            .score = _nms_output_tensor->fix_scale(bbox_struct->score)};

        return dequant_bbox;
    }

    void parse_bbox_to_detection_object(common::hailo_bbox_t *bbox_struct, uint32_t class_index, std::vector<HailoDetection>& _objects)
    {
        // dequantize common::hailo_bbox_t
        common::hailo_bbox_float32_t dequant_bbox = dequantize_hailo_bbox(bbox_struct);
        float confidence = CLAMP(dequant_bbox.score, 0.0f, 1.0f);
        // filter score by detection threshold.
        if (dequant_bbox.score > _detection_thr)
        {
            float32_t w, h = 0.0f;
            // parse width and height of the box
            std::tie(w, h) = get_shape(&dequant_bbox);
            // create new detection object and add it to the vector of detections
            _objects.push_back(HailoDetection(HailoBBox(dequant_bbox.x_min, dequant_bbox.y_min, w, h), class_index, labels_dict[class_index], confidence));
        }
    }

    std::pair<float, float> get_shape(common::hailo_bbox_float32_t *bbox_struct)
    {
        float32_t w = bbox_struct->x_max - bbox_struct->x_min;
        float32_t h = bbox_struct->y_max - bbox_struct->y_min;
        return std::pair<float, float>(w, h);
    }

public:
    MobilenetSSDPost(HailoTensorPtr tensor, std::map<uint8_t, std::string> &labels_dict, float detection_thr = DEFAULT_MOBILENET_THRESHOLD, uint max_boxes = DEFAULT_MAX_BOXES)
        : _nms_output_tensor(tensor), labels_dict(labels_dict), _detection_thr(detection_thr), _max_boxes(max_boxes), _vstream_info(tensor->vstream_info())
    {
        // making sure that the network's output is indeed an NMS type, by checking the order type value included in the metadata
        if (HAILO_FORMAT_ORDER_HAILO_NMS != _vstream_info.format.order)
            throw std::invalid_argument("Output tensor " + _nms_output_tensor->name() + " is not an NMS type");
    };

    std::vector<HailoDetection> decode()
    {
        /*
                    NMS output decode method
                    decodes the nms buffer received from the output tensor of the network.
                    returns a vector of DetectonObject filtered by the detection threshold.

                    The data is sorted by the number of the classes.
                    for each class - first comes the number of boxes in the class, then the boxes one after the other,
                    each box contains x_min, y_min, x_max, y_max and score (uint16_t each) and can be casted to common::hailo_bbox_t struct (5*uint16_t).
                    means that a frame size of one class is sizeof(bbox_count) + bbox_count * sizeof(common::hailo_bbox_t).
                    and the actual size of the data is (frame size of one class)*number of classes.

                    The data comes after quantization - so dequantization to float32 is needed.

                    As an example - data buffer of a frame that contains a person and two dogs:
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

        //_nms_output_tensor - pointer to the output tensor's buffer of the network.

        std::vector<HailoDetection> _objects;
        if (!_nms_output_tensor)
            return _objects;

        _objects.reserve(_max_boxes);
        uint8_t *src_ptr = _nms_output_tensor->data();
        uint32_t actual_frame_size = 0;

        uint32_t num_of_classes = _vstream_info.nms_shape.number_of_classes;
        uint32_t max_bboxes_per_class = _vstream_info.nms_shape.max_bboxes_per_class;
        for (uint32_t class_index = 1; class_index <= num_of_classes; class_index++)
        {
            uint16_t bbox_count = *reinterpret_cast<const uint16_t *>(src_ptr + actual_frame_size);
            if (bbox_count > max_bboxes_per_class)
                throw std::runtime_error(("Runtime error - Got more than the maximun bboxes per class in the nms buffer"));

            if (bbox_count > 0)
            {
                uint8_t *class_ptr = src_ptr + actual_frame_size + sizeof(bbox_count);
                // iterate over the boxes and parse each box to common::hailo_bbox_t
                for (uint8_t box_index = 0; box_index < bbox_count; box_index++)
                {
                    common::hailo_bbox_t *bbox_struct = (common::hailo_bbox_t *)(class_ptr + (box_index * sizeof(common::hailo_bbox_t)));
                    parse_bbox_to_detection_object(bbox_struct, class_index, _objects);
                }
            }
            // calculate the frame size of the class - sums up the size of the output during iteration
            uint16_t class_frame_size = static_cast<uint16_t>(sizeof(bbox_count) + bbox_count * sizeof(common::hailo_bbox_t));
            actual_frame_size += class_frame_size;
        }

        return _objects;
    }
};

void mobilenet_ssd(HailoROIPtr roi)
{
    auto post = MobilenetSSDPost(roi->get_tensor(DEFAULT_NMS_OUTPUT_LAYER), common::coco_ninety_classes);
    auto detections = post.decode();
    hailo_common::add_detections(roi, detections);
}

void mobilenet_ssd_merged(HailoROIPtr roi)
{
    auto post = MobilenetSSDPost(roi->get_tensor("ssd_mobilenet_v1_no_alls/nms1"), common::coco_ninety_classes);
    auto detections = post.decode();
    hailo_common::add_detections(roi, detections);
}

void mobilenet_ssd_visdrone(HailoROIPtr roi)
{
    auto post = MobilenetSSDPost(roi->get_tensor("ssd_mobilenet_v1_visdrone/nms1"), common::coco_visdrone_classes);
    auto detections = post.decode();
    hailo_common::add_detections(roi, detections);
}

void filter(HailoROIPtr roi)
{
    mobilenet_ssd(roi);
}
