/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#ifndef _HAILO_DETECTION_POST_HPP_
#define _HAILO_DETECTION_POST_HPP_
#include <vector>
#include "hailo_frame.hpp"
#include "hailo_detection.hpp"
constexpr float DEFAULT_IOU_THRESHOLD = 0.45f;
constexpr float DEFAULT_DETECTION_THRESHOLD = 0.35f;

class DetectionPost
{
protected:
    HailoFramePtr _frame;
    std::vector<DetectionObject> _objects;
    float _detection_thr;
    float _iou_thr;
    uint _max_boxes;
    float iou_calc(const DetectionObject &box_1, const DetectionObject &box_2);
    void nms();

public:
    DetectionPost(HailoFramePtr frame, uint max_boxes, float detection_threshold = DEFAULT_DETECTION_THRESHOLD, float iou_threshold = DEFAULT_IOU_THRESHOLD);
    //~DetectionPost();
    virtual std::vector<DetectionObject> decode() = 0;
    virtual ~DetectionPost() = default;
    bool should_nms_cross_classes = false;
};

#endif /*_HAILO_DETECTION_POST_HPP_ */