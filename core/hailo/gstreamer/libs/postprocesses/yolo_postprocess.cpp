/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include <cmath>
#include <vector>
#include <algorithm>
#include "yolo_postprocess.hpp"
#include "yolo_output.hpp"
#include "common/common.hpp"
#include "common/detection.hpp"
#include "common/labels/coco_eighty.hpp"
#include "tensor_meta.hpp"

static const int DEFAULT_MAX_BOXES = 100;

class YoloPost : public DetectionPost
{
protected:
    static const int NUM_OF_OUTPUT_LAYERS = 3;
    std::vector<std::shared_ptr<YoloOutputLayer>> _layers;
    uint m_image_width;
    uint m_image_height;
    std::map<guint8, std::string> m_dataset;

public:
    YoloPost(HailoFramePtr frame,
             float detection_threshold = DEFAULT_DETECTION_THRESHOLD,
             float iou_threshold = DEFAULT_IOU_THRESHOLD,
             uint max_boxes = DEFAULT_MAX_BOXES)
        : DetectionPost(frame, max_boxes, detection_threshold, iou_threshold), m_dataset(common::coco_eighty){};

    YoloPost(HailoFramePtr frame,
             std::map<guint8, std::string> dataset,
             float detection_threshold = DEFAULT_DETECTION_THRESHOLD,
             float iou_threshold = DEFAULT_IOU_THRESHOLD,
             uint max_boxes = DEFAULT_MAX_BOXES)
        : DetectionPost(frame, max_boxes, detection_threshold, iou_threshold), m_dataset(dataset){};

    virtual std::vector<DetectionObject> decode()
    {
        for (auto layer : _layers)
        {
            extract_boxes(layer);
        }
        nms();
        return _objects;
    }
    /**
     * @brief Extract the boxes of generic yolo output layer.
     *
     * @param[in] image_size Network's input image width/height.
     * @param[in] thr Postprocess threshold.
     * @param[out] objects Reference to vector of detections.
     */
    void extract_boxes(std::shared_ptr<YoloOutputLayer> layer);
};

void YoloPost::extract_boxes(std::shared_ptr<YoloOutputLayer> layer)
{
    uint class_id = 0;
    float x, y, h, w, confidence, class_confidence = 0.0f;
    float xmin, ymin = 0.0f;
    for (uint row = 0; row < layer->_height; ++row)
    {
        for (uint col = 0; col < layer->_width; ++col)
        {
            for (uint anchor = 0; anchor < layer->NUM_ANCHORS; ++anchor)
            {
                confidence = layer->get_confidence(row, col, anchor);
                if (confidence < _detection_thr)
                    continue;
                std::tie(class_id, class_confidence) = layer->get_class(row, col, anchor);
                // Final confidence: box confidence * class probability
                confidence = confidence * class_confidence;
                if (confidence > _detection_thr)
                {
                    std::tie(x, y) = layer->get_center(row, col, anchor);
                    std::tie(w, h) = layer->get_shape(row, col, anchor, m_image_width, m_image_height);
                    // Get the top left corner of the object.
                    xmin = (x - (w / 2.0f));
                    ymin = (y - (h / 2.0f));
                    if (_objects.size() < _max_boxes)
                        _objects.push_back(DetectionObject(xmin, ymin, h, w, confidence, m_dataset[class_id], class_id));
                    else
                        return;
                }
            }
        }
    }
}

class Yolov5 : public YoloPost
{
public:
    int _anchors[NUM_OF_OUTPUT_LAYERS][YoloOutputLayer::NUM_ANCHORS * 2] =
        {{116, 90, 156, 198, 373, 326},
         {30, 61, 62, 45, 59, 119},
         {10, 13, 16, 30, 33, 23}};
    Yolov5(HailoFramePtr frame, float threshold)
        : YoloPost(frame, threshold), _tensors(frame->get_tensors())
    {
        yolov5_init();
    };
    Yolov5(HailoFramePtr frame,
           std::map<guint8, std::string> dataset,
           float threshold)
        : YoloPost(frame, dataset, threshold), _tensors(frame->get_tensors())
    {
        yolov5_init();
    };

    Yolov5(HailoFramePtr frame,
           std::map<guint8, std::string> dataset,
           float threshold,
           int max_boxes)
        : YoloPost(frame, dataset, threshold, DEFAULT_IOU_THRESHOLD, max_boxes), _tensors(frame->get_tensors())
    {
        yolov5_init();
    };
    void yolov5_init()
    {
        if (_tensors.size() > 0)
        {
            sort(_tensors.begin(), _tensors.end(), sort_tensors_by_size);
            m_image_width = _tensors[0]->width * 32;
            m_image_height = _tensors[0]->height * 32;
            _layers.reserve(_tensors.size());
            for (std::size_t i = 0; i < _tensors.size(); i++)
            {
                _layers.push_back(std::make_shared<Yolov5OL>(_tensors[i], _anchors[i]));
            }
        }
    }

    ~Yolov5() = default;

private:
    /* data */
    std::vector<HailoTensorPtr> _tensors;
    static bool sort_tensors_by_size(HailoTensorPtr i, HailoTensorPtr j) { return (i->width < j->width); }
};

class YoloSplitted : public YoloPost
{
public:
    int _anchors[NUM_OF_OUTPUT_LAYERS][YoloOutputLayer::NUM_ANCHORS * 2] =
        {116, 90, 156, 198, 373, 326,
         30, 61, 62, 45, 59, 119,
         10, 13, 16, 30, 33, 23};

    YoloSplitted(HailoFramePtr frame, float threshold)
        : YoloPost(frame, threshold), _tensors(frame->get_tensors_by_name())
    {
        _layers.reserve(_tensors.size() / 4);
    };

protected:
    /* data */
    std::map<std::string, HailoTensorPtr> _tensors;
};

class Yolov3 : public YoloSplitted
{
public:
    Yolov3(HailoFramePtr frame, float threshold)
        : YoloSplitted(frame, threshold)
    {
        if (_tensors.size() > 0)
        {
            m_image_width = _tensors["yolov3_gluon/conv83_centers"]->width * 32;
            m_image_height = _tensors["yolov3_gluon/conv83_centers"]->height * 32;
            _layers.push_back(std::make_shared<Yolov3OL>(_tensors["yolov3_gluon/conv83_centers"], _tensors["yolov3_gluon/conv83_scales"],
                                                         _tensors["yolov3_gluon/conv83_obj"], _tensors["yolov3_gluon/conv83_probs"],
                                                         _anchors[0]));
            _layers.push_back(std::make_shared<Yolov3OL>(_tensors["yolov3_gluon/conv91_centers"], _tensors["yolov3_gluon/conv91_scales"],
                                                         _tensors["yolov3_gluon/conv91_obj"], _tensors["yolov3_gluon/conv91_probs"],
                                                         _anchors[1]));
            _layers.push_back(std::make_shared<Yolov3OL>(_tensors["yolov3_gluon/conv98_centers"], _tensors["yolov3_gluon/conv98_scales"],
                                                         _tensors["yolov3_gluon/conv98_obj"], _tensors["yolov3_gluon/conv98_probs"],
                                                         _anchors[2]));
        }
    };
    ~Yolov3() = default;
};

class Yolov4 : public YoloSplitted
{
public:
    Yolov4(HailoFramePtr frame, float threshold)
        : YoloSplitted(frame, threshold)
    {
        if (_tensors.size() > 0)
        {
            m_image_width = _tensors["yolov4_leaky/conv110_centers"]->width * 32;
            m_image_height = _tensors["yolov4_leaky/conv110_centers"]->height * 32;
            _layers.push_back(std::make_shared<Yolov4OL>(_tensors["yolov4_leaky/conv110_centers"], _tensors["yolov4_leaky/conv110_scales"],
                                                         _tensors["yolov4_leaky/conv110_obj"], _tensors["yolov4_leaky/conv110_probs"],
                                                         _anchors[0]));
            _layers.push_back(std::make_shared<Yolov4OL>(_tensors["yolov4_leaky/conv103_centers"], _tensors["yolov4_leaky/conv103_scales"],
                                                         _tensors["yolov4_leaky/conv103_obj"], _tensors["yolov4_leaky/conv103_probs"],
                                                         _anchors[1]));
            _layers.push_back(std::make_shared<Yolov4OL>(_tensors["yolov4_leaky/conv95_centers"], _tensors["yolov4_leaky/conv95_scales"],
                                                         _tensors["yolov4_leaky/conv95_obj"], _tensors["yolov4_leaky/conv95_probs"],
                                                         _anchors[2]));
        }
    };
    ~Yolov4() = default;
};

class TinyYolov4 : public YoloSplitted
{
public:
    TinyYolov4(HailoFramePtr frame, float threshold)
        : YoloSplitted(frame, threshold)
    {
        if (_tensors.size() > 0)
        {
            m_image_width = _tensors["tiny_yolov4_license_plates/conv19_centers"]->width * 32;
            m_image_height = _tensors["tiny_yolov4_license_plates/conv19_centers"]->height * 32;
            _layers.push_back(std::make_shared<Yolov4OL>(_tensors["tiny_yolov4_license_plates/conv19_centers"], _tensors["tiny_yolov4_license_plates/conv19_scales"],
                                                         _tensors["tiny_yolov4_license_plates/conv19_obj"], _tensors["tiny_yolov4_license_plates/conv19_probs"],
                                                         _anchors[0]));
            _layers.push_back(std::make_shared<Yolov4OL>(_tensors["tiny_yolov4_license_plates/conv21_centers"], _tensors["tiny_yolov4_license_plates/conv21_scales"],
                                                         _tensors["tiny_yolov4_license_plates/conv21_obj"], _tensors["tiny_yolov4_license_plates/conv21_probs"],
                                                         _anchors[1]));
        }
    };
    ~TinyYolov4() = default;
};


void yolov5_no_persons(HailoFramePtr hailo_frame)
{
    float thr = 0.35;
    auto post = Yolov5(hailo_frame, thr);
    auto detections = post.decode();
    gint person_class_id = 1;
    detections.erase(std::remove_if(detections.begin(), detections.end(),
                                    [person_class_id](DetectionObject obj)
                                    { return obj.class_id == person_class_id; }),
                     detections.end());
    common::update_frame(hailo_frame, detections);
}

void yolov5_counter(HailoFramePtr hailo_frame)
{
    uint max_boxes = 500;
    static std::map<guint8, std::string> peppers = {{0, "unlabeled"}, {1, "orange"}, {2, "red"}, {3, "yellow"}};
    float thr = 0.35;
    auto post = Yolov5(hailo_frame, peppers, thr, max_boxes);
    post.should_nms_cross_classes = true;
    auto detections = post.decode();
    common::update_frame(hailo_frame, detections);
}

void yolov5(HailoFramePtr hailo_frame)
{
    float thr = 0.35;
    auto post = Yolov5(hailo_frame, thr);
    auto detections = post.decode();
    common::update_frame(hailo_frame, detections);
}

void yolov3(HailoFramePtr hailo_frame)
{
    gfloat thr = 0.35;
    auto post = Yolov3(hailo_frame, thr);
    auto detections = post.decode();
    common::update_frame(hailo_frame, detections);
}

void yolov4(HailoFramePtr hailo_frame)
{
    gfloat thr = 0.35;
    auto post = Yolov4(hailo_frame, thr);
    auto detections = post.decode();
    common::update_frame(hailo_frame, detections);
}

void tinyyolov4(HailoFramePtr hailo_frame)
{
    gfloat thr = 0.35;
    auto post = TinyYolov4(hailo_frame, thr);
    auto detections = post.decode();
    common::update_frame(hailo_frame, detections);
}

void filter(HailoFramePtr hailo_frame)
{
    yolov5(hailo_frame);
}
