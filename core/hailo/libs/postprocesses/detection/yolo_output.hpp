/**
 * Copyright (c) 2021-2026 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#ifndef _POSTPROCESSES_DETECTION_YOLO_OUTPUT_HPP_
#define _POSTPROCESSES_DETECTION_YOLO_OUTPUT_HPP_

#include "hailo_objects.hpp"
#include "common/math.hpp"
#include <iostream>

/**
 * @brief Base class to represent OutputLayer of Yolo networks.
 *
 */
class YoloOutputLayer
{
public:
    static const uint32_t NUM_ANCHORS = 3;
    static const uint32_t NUM_CENTERS = 2;
    static const uint32_t NUM_SCALES = 2;
    static const uint32_t NUM_CONF = 1;
    static const uint32_t CONF_CHANNEL_OFFSET = NUM_CENTERS + NUM_SCALES;
    static const uint32_t CLASS_CHANNEL_OFFSET = CONF_CHANNEL_OFFSET + NUM_CONF;
    YoloOutputLayer(
        uint32_t width,
        uint32_t height,
        uint32_t num_of_classes,
        std::vector<int> anchors,
        bool perform_sigmoid,
        int label_offset,
        bool is_uint16,
        HailoTensorPtr tensor = nullptr):
            _width(width),
            _height(height),
            _num_classes(num_of_classes),
            _anchors(anchors),
            label_offset(label_offset),
            _perform_sigmoid(perform_sigmoid),
            _is_uint16(is_uint16),
            _tensor(tensor)
        {};
    virtual ~YoloOutputLayer() = default;

    uint32_t _width;
    uint32_t _height;
    uint32_t _num_classes;
    std::vector<int> _anchors;
    int label_offset;

    /**
     * @brief Get the class object
     *
     * @param row
     * @param col
     * @param anchor
     * @return std::pair<uint32_t, float> class id and class probability.
     */
    std::pair<uint32_t, float> get_class(uint32_t row, uint32_t col, uint32_t anchor);
    /**
     * @brief Get the confidence object
     *
     * @param row
     * @param col
     * @param anchor
     * @return float
     */
    virtual float get_confidence(uint32_t row, uint32_t col, uint32_t anchor);
    /**
     * @brief Get the center object
     *
     * @param row
     * @param col
     * @param anchor
     * @return std::pair<float, float> pair of x,y of the center of this prediction.
     */
    virtual std::pair<float, float> get_center(uint32_t row, uint32_t col, uint32_t anchor) = 0;
    /**
     * @brief Get the shape object
     *
     * @param row
     * @param col
     * @param anchor
     * @param image_width
     * @param image_height
     * @return std::pair<float, float> pair of w,h of the shape of this prediction.
     */
    virtual std::pair<float, float> get_shape(uint32_t row, uint32_t col, uint32_t anchor, uint32_t image_width, uint32_t image_height) = 0;

protected:
    bool _perform_sigmoid;
    bool _is_uint16;
    HailoTensorPtr _tensor;
    /**
     * @brief Get the class channel object
     *
     * @param anchor
     * @param channel
     * @return uint32_t
     */
    virtual uint32_t get_class_prob(uint32_t row, uint32_t col, uint32_t anchor, uint32_t class_id);
    /**
     * @brief Get the class conf object
     *
     * @param prob_max
     * @return float
     */
    virtual float get_class_conf(uint32_t prob_max);
    static uint32_t num_classes(uint32_t channels)
    {
        return (channels / NUM_ANCHORS) - CLASS_CHANNEL_OFFSET;
    }
};

class Yolov3OL : public YoloOutputLayer
{
public:
    Yolov3OL(HailoTensorPtr tensor,
             std::vector<int> anchors,
             bool perform_sigmoid,
             int label_offset,
             bool is_uint16) : YoloOutputLayer(tensor->width(),
                                               tensor->height(),
                                               num_classes(tensor->features()),
                                               anchors,
                                               perform_sigmoid,
                                               label_offset,
                                               is_uint16,
                                               tensor){};

    virtual std::pair<float, float> get_center(uint32_t row, uint32_t col, uint32_t anchor);
    virtual std::pair<float, float> get_shape(uint32_t row, uint32_t col, uint32_t anchor, uint32_t image_width, uint32_t image_height);
};

class TinyYolov4OL : public YoloOutputLayer
{
public:
    const float SCALE_XY = 1.05f;
    TinyYolov4OL(HailoTensorPtr tensor,
                 std::vector<int> anchors,
                 bool perform_sigmoid,
                 int label_offset,
                 bool is_uint16) : YoloOutputLayer(tensor->width(),
                                                   tensor->height(),
                                                   num_classes(tensor->features()),
                                                   anchors,
                                                   perform_sigmoid,
                                                   label_offset,
                                                   is_uint16,
                                                   tensor){};
    virtual std::pair<float, float> get_center(uint32_t row, uint32_t col, uint32_t anchor);
    virtual std::pair<float, float> get_shape(uint32_t row, uint32_t col, uint32_t anchor, uint32_t image_width, uint32_t image_height);
};

class Yolov4OL : public YoloOutputLayer
{
public:
    const float SCALE_XY = 1.05f;
    Yolov4OL(HailoTensorPtr center,
             HailoTensorPtr scale,
             HailoTensorPtr obj,
             HailoTensorPtr cls,
             std::vector<int> anchors,
             int label_offset,
             bool perform_sigmoid,
             bool is_uint16) : YoloOutputLayer(cls->width(),
                                                     cls->height(),
                                                     static_cast<uint32_t>(cls->features() / NUM_ANCHORS),
                                                     anchors,
                                                     perform_sigmoid,
                                                     label_offset,
                                                     is_uint16),
                                     _center(center),
                                     _scale(scale),
                                     _obj(obj),
                                     _cls(cls){};
    virtual std::pair<float, float> get_center(uint32_t row, uint32_t col, uint32_t anchor);
    virtual float get_confidence(uint32_t row, uint32_t col, uint32_t anchor);
    virtual uint32_t get_class_prob(uint32_t row, uint32_t col, uint32_t anchor, uint32_t channel);
    virtual float get_class_conf(uint32_t prob_max);
    virtual std::pair<float, float> get_shape(uint32_t row, uint32_t col, uint32_t anchor, uint32_t image_width, uint32_t image_height);

protected:
    HailoTensorPtr _center;
    HailoTensorPtr _scale;
    HailoTensorPtr _obj;
    HailoTensorPtr _cls;
};

class Yolov5OL : public YoloOutputLayer
{
public:
    Yolov5OL(HailoTensorPtr tensor,
             std::vector<int> anchors,
             bool perform_sigmoid,
             int label_offset,
             bool is_uint16) : YoloOutputLayer(tensor->width(),
                                               tensor->height(),
                                               num_classes(tensor->features()),
                                               anchors,
                                               false,
                                               label_offset,
                                               is_uint16,
                                               tensor){};
    virtual std::pair<float, float> get_center(uint32_t row, uint32_t col, uint32_t anchor);
    virtual std::pair<float, float> get_shape(uint32_t row, uint32_t col, uint32_t anchor, uint32_t image_width, uint32_t image_height);
};

class YoloXOL : public YoloOutputLayer
{
public:
    static const uint32_t NUM_ANCHORS = 1;
    YoloXOL(HailoTensorPtr bbox,
            HailoTensorPtr obj,
            HailoTensorPtr cls,
            int label_offset,
            bool is_uint16) : YoloOutputLayer(cls->width(),
                                                cls->height(),
                                                static_cast<uint32_t>(cls->features()),
                                                {},
                                                false,
                                                label_offset,
                                                is_uint16),
                                _bbox(bbox),
                                _obj(obj),
                                _cls(cls){};
    virtual float get_confidence(uint32_t row, uint32_t col, uint32_t anchor);
    virtual uint32_t get_class_prob(uint32_t row, uint32_t col, uint32_t anchor, uint32_t channel);
    virtual float get_class_conf(uint32_t prob_max);
    virtual std::pair<float, float> get_center(uint32_t row, uint32_t col, uint32_t anchor);
    virtual std::pair<float, float> get_shape(uint32_t row, uint32_t col, uint32_t anchor, uint32_t image_width, uint32_t image_height);

protected:
    HailoTensorPtr _bbox;
    HailoTensorPtr _obj;
    HailoTensorPtr _cls;
};

#endif  // _POSTPROCESSES_DETECTION_YOLO_OUTPUT_HPP_