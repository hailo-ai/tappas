/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include <cmath>
#include <algorithm>
#include <iostream>
#include "detection.hpp"

DetectionPost::DetectionPost(HailoFramePtr frame, uint max_boxes, float detection_threshold, float iou_threshold)
    : _frame(frame), _detection_thr(detection_threshold), _iou_thr(iou_threshold), _max_boxes(max_boxes)
{
    _objects.reserve(_max_boxes);
};
/**
 * @brief calculate overlap of 2 Detections.
 *
 * @param box_1
 * @param box_2
 * @return float
 */
float DetectionPost::iou_calc(const DetectionObject &box_1, const DetectionObject &box_2)
{
    const float width_of_overlap_area = std::min(box_1.xmax, box_2.xmax) - std::max(box_1.xmin, box_2.xmin);
    const float height_of_overlap_area = std::min(box_1.ymax, box_2.ymax) - std::max(box_1.ymin, box_2.ymin);
    const float positive_width_of_overlap_area = std::max(width_of_overlap_area, 0.0f);
    const float positive_height_of_overlap_area = std::max(height_of_overlap_area, 0.0f);
    const float area_of_overlap = positive_width_of_overlap_area * positive_height_of_overlap_area;
    const float box_1_area = (box_1.ymax - box_1.ymin) * (box_1.xmax - box_1.xmin);
    const float box_2_area = (box_2.ymax - box_2.ymin) * (box_2.xmax - box_2.xmin);
    return area_of_overlap / (box_1_area + box_2_area - area_of_overlap);
}
/**
 * @brief perform nms operations on a vector of detections.
 *
 * @param box_1
 * @param box_2
 * @return float
 */
void DetectionPost::nms()
{
    if (_objects.size() > 0)
    {
        float detection_thr = _detection_thr;
        std::sort(_objects.begin(), _objects.end(), std::greater<DetectionObject>());
        for (unsigned int i = 0; i < _objects.size(); ++i)
        {
            if (_objects[i].confidence <= detection_thr)
                continue;
            for (unsigned int j = i + 1; j < _objects.size(); ++j)
            {

                if ((should_nms_cross_classes || _objects[i].class_id == _objects[j].class_id) && _objects[j].confidence >= detection_thr)
                {
                    if (iou_calc(_objects[i], _objects[j]) >= _iou_thr)
                    {
                        _objects[j].confidence = 0;
                    }
                }
            }
        }
        _objects.erase(std::remove_if(_objects.begin(), _objects.end(),
                                      [detection_thr](DetectionObject obj)
                                      { return obj.confidence < detection_thr; }),
                       _objects.end());
    }
}