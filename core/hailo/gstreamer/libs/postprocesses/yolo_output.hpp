/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#ifndef _HAILO_YOLOV5_OL_HPP_
#define _HAILO_YOLOV5_OL_HPP_
#include "hailo_frame.hpp"
#include "hailo_detection.hpp"

/**
 * @brief Base class to represent OutputLayer of Yolo networks.
 *
 */
class YoloOutputLayer
{
public:
    static const uint NUM_ANCHORS = 3;
    YoloOutputLayer(uint width, uint height, uint num_of_classes, int *anchors)
        : _width(width), _height(height), _num_classes(num_of_classes), _anchors(anchors){};
    virtual ~YoloOutputLayer() = default;

    uint _width;
    uint _height;
    uint _num_classes;
    int *_anchors;
    /**
     * @brief Get the class object
     *
     * @param row
     * @param col
     * @param anchor
     * @return std::pair<uint, float> class id and class probability.
     */
    std::pair<uint, float> get_class(uint row, uint col, uint anchor);
    /**
     * @brief Get the confidence object
     *
     * @param row
     * @param col
     * @param anchor
     * @return float
     */
    virtual float get_confidence(uint row, uint col, uint anchor) = 0;
    /**
     * @brief Get the center object
     *
     * @param row
     * @param col
     * @param anchor
     * @return std::pair<float, float> pair of x,y of the center of this prediction.
     */
    virtual std::pair<float, float> get_center(uint row, uint col, uint anchor) = 0;
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
    virtual std::pair<float, float> get_shape(uint row, uint col, uint anchor, uint image_width, uint image_height) = 0;

protected:
    /**
     * @brief Get the class channel object
     *
     * @param anchor
     * @param channel
     * @return uint
     */
    virtual uint get_class_prob(uint row, uint col, uint anchor, uint class_id) = 0;
    /**
     * @brief Get the class conf object
     *
     * @param prob_max
     * @return float
     */
    virtual float get_class_conf(uint prob_max) = 0;
};

class YoloSplittedOutputLayer : public YoloOutputLayer
{
public:
    YoloSplittedOutputLayer(HailoTensorPtr center,
                            HailoTensorPtr scale,
                            HailoTensorPtr obj,
                            HailoTensorPtr cls,
                            int *anchors,
                            bool perform_sigmoid)
        : YoloOutputLayer(cls->width, cls->height, (uint)(cls->channels / NUM_ANCHORS), anchors),
          _center(center), _scale(scale), _obj(obj), _cls(cls), _perform_sigmoid(perform_sigmoid){};
    virtual float get_confidence(uint row, uint col, uint anchor);
    virtual uint get_class_prob(uint row, uint col, uint anchor, uint channel);
    virtual float get_class_conf(uint prob_max);
    virtual std::pair<float, float> get_shape(uint row, uint col, uint anchor, uint image_width, uint image_height);

protected:
    HailoTensorPtr _center;
    HailoTensorPtr _scale;
    HailoTensorPtr _obj;
    HailoTensorPtr _cls;
    bool _perform_sigmoid;
    float sigmoid(float x);
};

class Yolov3OL : public YoloSplittedOutputLayer
{
public:
    Yolov3OL(HailoTensorPtr center, HailoTensorPtr scale,
             HailoTensorPtr obj, HailoTensorPtr cls, int *anchors)
        : YoloSplittedOutputLayer(center, scale, obj, cls, anchors, true){};
    virtual std::pair<float, float> get_center(uint row, uint col, uint anchor);
};

class Yolov4OL : public YoloSplittedOutputLayer
{
public:
    const float SCALE_XY = 1.05f;
    Yolov4OL(HailoTensorPtr center,
             HailoTensorPtr scale,
             HailoTensorPtr obj,
             HailoTensorPtr cls,
             int *anchors) : YoloSplittedOutputLayer(center, scale, obj, cls, anchors, false){};
    virtual std::pair<float, float> get_center(uint row, uint col, uint anchor);
};

class Yolov5OL : public YoloOutputLayer
{
public:
    static const uint NUM_CENTERS = 2;
    static const uint NUM_SCALES = 2;
    static const uint NUM_CONF = 1;
    static const uint CONF_CHANNEL_OFFSET = NUM_CENTERS + NUM_SCALES;
    static const uint CLASS_CHANNEL_OFFSET = CONF_CHANNEL_OFFSET + NUM_CONF;
    Yolov5OL(HailoTensorPtr tensor,
             int *anchors)
        : YoloOutputLayer(tensor->width, tensor->height, num_classes(tensor->channels), anchors), _tensor(tensor)
    {
        _anchor_size = _tensor->channels / NUM_ANCHORS;
    };

    /**
     * @brief return the output layer's number of classes.
     *
     * @param channels the number of features of the tensor.
     * @return uint
     */
    static uint num_classes(uint channels)
    {
        return (channels / NUM_ANCHORS) - CLASS_CHANNEL_OFFSET;
    }

    virtual float get_confidence(uint row, uint col, uint anchor);
    virtual float get_class_conf(uint prob_max);
    virtual uint get_class_prob(uint row, uint col, uint anchor, uint channel);
    virtual std::pair<float, float> get_center(uint row, uint col, uint anchor);
    virtual std::pair<float, float> get_shape(uint row, uint col, uint anchor, uint image_width, uint image_height);

private:
    HailoTensorPtr _tensor;
    uint _anchor_size;
};

#endif