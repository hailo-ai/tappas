/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include <cmath>
#include <vector>
#include <algorithm>
#include "yolo_postprocess.hpp"
#include "yolo_output.hpp"
#include "common/nms.hpp"
#include "common/labels/coco_eighty.hpp"

static const int DEFAULT_MAX_BOXES = 100;
constexpr float DEFAULT_IOU_THRESHOLD = 0.45f;
constexpr float DEFAULT_DETECTION_THRESHOLD = 0.35f;

class YoloPost
{
protected:
    static const int NUM_OF_OUTPUT_LAYERS = 3;
    std::vector<std::shared_ptr<YoloOutputLayer>> _layers;
    uint _max_boxes;
    float _detection_thr;
    float _iou_thr;
    uint m_image_width;
    uint m_image_height;
    std::map<uint8_t, std::string> m_dataset;

public:
    int _anchors[NUM_OF_OUTPUT_LAYERS][YoloOutputLayer::NUM_ANCHORS * 2] =
        {{116, 90, 156, 198, 373, 326},
         {30, 61, 62, 45, 59, 119},
         {10, 13, 16, 30, 33, 23}};
    virtual ~YoloPost() = default;
    YoloPost(float detection_threshold = DEFAULT_DETECTION_THRESHOLD,
             float iou_threshold = DEFAULT_IOU_THRESHOLD,
             uint max_boxes = DEFAULT_MAX_BOXES)
        : _max_boxes(max_boxes), _detection_thr(detection_threshold),
          _iou_thr(iou_threshold), m_dataset(common::coco_eighty){};

    YoloPost(std::map<uint8_t, std::string> dataset,
             float detection_threshold = DEFAULT_DETECTION_THRESHOLD,
             float iou_threshold = DEFAULT_IOU_THRESHOLD,
             uint max_boxes = DEFAULT_MAX_BOXES)
        : _max_boxes(max_boxes), _detection_thr(detection_threshold),
          _iou_thr(iou_threshold), m_dataset(dataset){};

    std::vector<NewHailoDetection> decode()
    {
        std::vector<NewHailoDetection> objects;
        objects.reserve(_max_boxes);
        for (auto layer : _layers)
        {
            extract_boxes(layer, objects);
        }
        common::nms(objects, _iou_thr);
        return objects;
    }
    /**
     * @brief Extract the boxes of generic yolo output layer.
     *
     * @param[in] image_size Network's input image width/height.
     * @param[in] thr Postprocess threshold.
     * @param[out] objects Reference to vector of detections.
     */
    void extract_boxes(std::shared_ptr<YoloOutputLayer> layer,
                       std::vector<NewHailoDetection> &objects);
};

void YoloPost::extract_boxes(std::shared_ptr<YoloOutputLayer> layer,
                             std::vector<NewHailoDetection> &objects)
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
                    if (objects.size() < _max_boxes)
                        objects.push_back(NewHailoDetection(HailoBBox(xmin, ymin, w, h), class_id, m_dataset[class_id], confidence));
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
    Yolov5(HailoROIPtr roi, float threshold)
        : YoloPost(threshold), _tensors(roi->get_tensors())
    {
        yolov5_init();
    };
    Yolov5(HailoROIPtr roi,
           std::map<uint8_t, std::string> dataset,
           float threshold)
        : YoloPost(dataset, threshold), _tensors(roi->get_tensors())
    {
        yolov5_init();
    };

    Yolov5(HailoROIPtr roi,
           std::map<uint8_t, std::string> dataset,
           float threshold,
           int max_boxes)
        : YoloPost(dataset, threshold, DEFAULT_IOU_THRESHOLD, max_boxes), _tensors(roi->get_tensors())
    {
        yolov5_init();
    };
    void yolov5_init()
    {
        if (_tensors.size() > 0)
        {
            sort(_tensors.begin(), _tensors.end(),
                 [](const NewHailoTensorPtr &a, const NewHailoTensorPtr &b)
                 { return a->size() < b->size(); });

            m_image_width = _tensors[0]->width() * 32;
            m_image_height = _tensors[0]->height() * 32;
            _layers.reserve(_tensors.size());
            for (std::size_t i = 0; i < _tensors.size(); i++)
            {
                _layers.push_back(std::make_shared<Yolov5OL>(_tensors[i], _anchors[i]));
            }
        }
    }

    virtual ~Yolov5() = default;

private:
    /* data */
    std::vector<NewHailoTensorPtr> _tensors;
};

class YoloSplitted : public YoloPost
{
public:
    virtual ~YoloSplitted() = default;

    YoloSplitted(HailoROIPtr roi, std::map<uint8_t, std::string> dataset, float threshold)
        : YoloPost(dataset, threshold), _roi(roi){};

    YoloSplitted(HailoROIPtr roi, float threshold)
        : YoloPost(threshold), _roi(roi){};

protected:
    /* data */
    HailoROIPtr _roi;
};

class Yolov3 : public YoloSplitted
{
public:
    Yolov3(HailoROIPtr roi, float threshold)
        : YoloSplitted(roi, threshold)
    {
        if (roi->has_tensors())
        {
            m_image_width = _roi->get_tensor("yolov3_gluon/conv83_centers")->width() * 32;
            m_image_height = _roi->get_tensor("yolov3_gluon/conv83_centers")->height() * 32;
            _layers.push_back(std::make_shared<Yolov3OL>(_roi->get_tensor("yolov3_gluon/conv83_centers"), _roi->get_tensor("yolov3_gluon/conv83_scales"),
                                                         _roi->get_tensor("yolov3_gluon/conv83_obj"), _roi->get_tensor("yolov3_gluon/conv83_probs"),
                                                         _anchors[0]));
            _layers.push_back(std::make_shared<Yolov3OL>(_roi->get_tensor("yolov3_gluon/conv91_centers"), _roi->get_tensor("yolov3_gluon/conv91_scales"),
                                                         _roi->get_tensor("yolov3_gluon/conv91_obj"), _roi->get_tensor("yolov3_gluon/conv91_probs"),
                                                         _anchors[1]));
            _layers.push_back(std::make_shared<Yolov3OL>(_roi->get_tensor("yolov3_gluon/conv98_centers"), _roi->get_tensor("yolov3_gluon/conv98_scales"),
                                                         _roi->get_tensor("yolov3_gluon/conv98_obj"), _roi->get_tensor("yolov3_gluon/conv98_probs"),
                                                         _anchors[2]));
        }
    };
    virtual ~Yolov3() = default;
};

class Yolov4 : public YoloSplitted
{
public:
    int _anchors[NUM_OF_OUTPUT_LAYERS][YoloOutputLayer::NUM_ANCHORS * 2] =
        {{142, 110, 192, 243, 359, 401},
         {36, 75, 76, 55, 72, 146},
         {12, 16, 19, 36, 40, 28}};
    Yolov4(HailoROIPtr roi, float threshold)
        : YoloSplitted(roi, threshold)
    {
        if (_roi->has_tensors())
        {
            m_image_width = _roi->get_tensor("yolov4_leaky/conv110_centers")->width() * 32;
            m_image_height = _roi->get_tensor("yolov4_leaky/conv110_centers")->height() * 32;
            _layers.push_back(std::make_shared<Yolov4OL>(_roi->get_tensor("yolov4_leaky/conv110_centers"), _roi->get_tensor("yolov4_leaky/conv110_scales"),
                                                         _roi->get_tensor("yolov4_leaky/conv110_obj"), _roi->get_tensor("yolov4_leaky/conv110_probs"),
                                                         _anchors[0]));
            _layers.push_back(std::make_shared<Yolov4OL>(_roi->get_tensor("yolov4_leaky/conv103_centers"), _roi->get_tensor("yolov4_leaky/conv103_scales"),
                                                         _roi->get_tensor("yolov4_leaky/conv103_obj"), _roi->get_tensor("yolov4_leaky/conv103_probs"),
                                                         _anchors[1]));
            _layers.push_back(std::make_shared<Yolov4OL>(_roi->get_tensor("yolov4_leaky/conv95_centers"), _roi->get_tensor("yolov4_leaky/conv95_scales"),
                                                         _roi->get_tensor("yolov4_leaky/conv95_obj"), _roi->get_tensor("yolov4_leaky/conv95_probs"),
                                                         _anchors[2]));
        }
    };
    virtual ~Yolov4() = default;
};

class TinyYolov4LicensePlates : public YoloSplitted
{
public:
    int _anchors[NUM_OF_OUTPUT_LAYERS][YoloOutputLayer::NUM_ANCHORS * 2] =
        {81, 82, 135, 169, 344, 319, 10, 14, 23, 27, 37, 58};

    TinyYolov4LicensePlates(HailoROIPtr roi, std::map<uint8_t, std::string> dataset, float threshold)
        : YoloSplitted(roi, dataset, threshold)
    {
        yolov4_init();
    };

    TinyYolov4LicensePlates(HailoROIPtr roi, float threshold)
        : YoloSplitted(roi, threshold)
    {
        yolov4_init();
    };

    void yolov4_init()
    {
        if (_roi->has_tensors())
        {
            m_image_width = _roi->get_tensor("tiny_yolov4_license_plates/conv19_centers")->width() * 32;
            m_image_height = _roi->get_tensor("tiny_yolov4_license_plates/conv19_centers")->height() * 32;
            _layers.push_back(std::make_shared<Yolov4OL>(_roi->get_tensor("tiny_yolov4_license_plates/conv19_centers"), _roi->get_tensor("tiny_yolov4_license_plates/conv19_scales"),
                                                         _roi->get_tensor("tiny_yolov4_license_plates/conv19_obj"), _roi->get_tensor("tiny_yolov4_license_plates/conv19_probs"),
                                                         _anchors[0]));
            _layers.push_back(std::make_shared<Yolov4OL>(_roi->get_tensor("tiny_yolov4_license_plates/conv21_centers"), _roi->get_tensor("tiny_yolov4_license_plates/conv21_scales"),
                                                         _roi->get_tensor("tiny_yolov4_license_plates/conv21_obj"), _roi->get_tensor("tiny_yolov4_license_plates/conv21_probs"),
                                                         _anchors[1]));
        }
    }
    virtual ~TinyYolov4LicensePlates() = default;
};

class YoloX : public YoloPost
{
public:
    YoloX(HailoROIPtr roi, float threshold)
        : YoloPost(threshold), _roi(roi)
    {
        if (_roi->has_tensors())
        {
            m_image_width = _roi->get_tensor("yolox_l_leaky/conv130")->width() * 32;
            m_image_height = _roi->get_tensor("yolox_l_leaky/conv130")->height() * 32;
            _layers.push_back(std::make_shared<YoloXOL>(_roi->get_tensor("yolox_l_leaky/conv130"), _roi->get_tensor("yolox_l_leaky/conv131"),
                                                        _roi->get_tensor("yolox_l_leaky/conv129")));
            _layers.push_back(std::make_shared<YoloXOL>(_roi->get_tensor("yolox_l_leaky/conv113"), _roi->get_tensor("yolox_l_leaky/conv114"),
                                                        _roi->get_tensor("yolox_l_leaky/conv112")));
            _layers.push_back(std::make_shared<YoloXOL>(_roi->get_tensor("yolox_l_leaky/conv95"), _roi->get_tensor("yolox_l_leaky/conv96"),
                                                        _roi->get_tensor("yolox_l_leaky/conv94")));
        }
    };
    ~YoloX() = default;

protected:
    /* data */
    HailoROIPtr _roi;
};

class YoloXX : public YoloPost
{
public:
    YoloXX(HailoROIPtr roi, float threshold)
        : YoloPost(threshold), _roi(roi)
    {
        if (_roi->has_tensors())
        {
            m_image_width = _roi->get_tensor("yolox_x_leaky/conv154")->width() * 32;
            m_image_height = _roi->get_tensor("yolox_x_leaky/conv154")->height() * 32;
            _layers.push_back(std::make_shared<YoloXOL>(_roi->get_tensor("yolox_x_leaky/conv154"), _roi->get_tensor("yolox_x_leaky/conv155"),
                                                        _roi->get_tensor("yolox_x_leaky/conv153")));
            _layers.push_back(std::make_shared<YoloXOL>(_roi->get_tensor("yolox_x_leaky/conv135"), _roi->get_tensor("yolox_x_leaky/conv136"),
                                                        _roi->get_tensor("yolox_x_leaky/conv134")));
            _layers.push_back(std::make_shared<YoloXOL>(_roi->get_tensor("yolox_x_leaky/conv115"), _roi->get_tensor("yolox_x_leaky/conv116"),
                                                        _roi->get_tensor("yolox_x_leaky/conv114")));
        }
    };
    ~YoloXX() = default;

protected:
    /* data */
    HailoROIPtr _roi;
};

void yolov5_no_persons(HailoROIPtr roi)
{
    float thr = 0.35;
    auto post = Yolov5(roi, thr);
    auto detections = post.decode();
    int person_class_id = 1;
    detections.erase(std::remove_if(detections.begin(), detections.end(),
                                    [person_class_id](NewHailoDetection obj)
                                    { return obj.get_class_id() == person_class_id; }),
                     detections.end());
    hailo_common::add_detections(roi, detections);
}

void yolov5_counter(HailoROIPtr roi)
{
    uint max_boxes = 500;
    static std::map<uint8_t, std::string> peppers = {{0, "unlabeled"}, {1, "orange"}, {2, "red"}, {3, "yellow"}};
    float thr = 0.35f;
    auto post = Yolov5(roi, peppers, thr, max_boxes);
    // TODO: nms cross classes.
    auto detections = post.decode();
    hailo_common::add_detections(roi, detections);
}

void yolov5_vehicles_only(HailoROIPtr roi)
{
    static std::map<uint8_t, std::string> vehicle_labels = {{0, "unlabeled"}, {1, "car"}};
    float thr = 0.35f;
    auto post = Yolov5(roi, vehicle_labels, thr);
    auto detections = post.decode();
    hailo_common::add_detections(roi, detections);
}

void yolov5(HailoROIPtr roi)
{
    float thr = 0.35f;
    auto post = Yolov5(roi, thr);
    auto detections = post.decode();
    hailo_common::add_detections(roi, detections);
}

void yolov3(HailoROIPtr roi)
{
    float thr = 0.35f;
    auto post = Yolov3(roi, thr);
    auto detections = post.decode();
    hailo_common::add_detections(roi, detections);
}

void yolov4(HailoROIPtr roi)
{
    float thr = 0.35f;
    auto post = Yolov4(roi, thr);
    auto detections = post.decode();
    hailo_common::add_detections(roi, detections);
}

void tiny_yolov4_license_plates(HailoROIPtr roi)
{
    float thr = 0.35f;
    static std::map<uint8_t, std::string> lp_labels = {{0, "unlabeled"}, {1, "license_plate"}};
    auto post = TinyYolov4LicensePlates(roi, lp_labels, thr);
    auto detections = post.decode();
    hailo_common::add_detections(roi, detections);
}
void yolox(HailoROIPtr roi)
{
    float thr = 0.35f;
    auto post = YoloX(roi, thr);
    auto detections = post.decode();
    hailo_common::add_detections(roi, detections);
}

void yoloxx(HailoROIPtr roi)
{
    float thr = 0.35f;
    auto post = YoloXX(roi, thr);
    auto detections = post.decode();
    hailo_common::add_detections(roi, detections);
}

void filter(HailoROIPtr roi)
{
    yolov5(roi);
}
