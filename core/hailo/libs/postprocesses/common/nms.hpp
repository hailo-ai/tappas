/**
 * Copyright (c) 2021-2026 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#ifndef _POSTPROCESSES_COMMON_NMS_HPP_
#define _POSTPROCESSES_COMMON_NMS_HPP_

#include <algorithm>
#include "hailo_objects.hpp"
#include "hailo_common.hpp"

namespace common
{

    float iou_calc(const HailoBBox &box_1, const HailoBBox &box_2);

    /**
     * @brief Generic IOU-based NMS on any container whose elements wrap a HailoDetection.
     *
     * @param objects           Vector of elements to perform NMS on (modified in-place).
     * @param get_detection     Accessor: given an element, returns HailoDetection& for reading/writing confidence.
     * @param iou_thr           IOU threshold for suppression.
     * @param should_nms_cross_classes  If true, suppress across different classes.
     */
    template <typename T, typename GetDetection>
    void nms(std::vector<T> &objects, GetDetection get_detection,
        const float iou_thr, bool should_nms_cross_classes = false)
    {
        std::sort(objects.begin(), objects.end(),
            [&](T &a, T &b) {
                return get_detection(a).get_confidence() > get_detection(b).get_confidence();
            });

        for (uint32_t i = 0; i < objects.size(); i++) {
            if (0.0f != get_detection(objects[i]).get_confidence()) {
                for (uint32_t j = i + 1; j < objects.size(); j++) {
                    if ((should_nms_cross_classes ||
                            (get_detection(objects[i]).get_class_id() == get_detection(objects[j]).get_class_id())) &&
                        (0.0f != get_detection(objects[j]).get_confidence())) {
                        float iou = iou_calc(
                            get_detection(objects[i]).get_bbox(),
                            get_detection(objects[j]).get_bbox());
                        if (iou >= iou_thr) {
                            get_detection(objects[j]).set_confidence(0.0f);
                        }
                    }
                }
            }
        }
        objects.erase(
            std::remove_if(objects.begin(), objects.end(),
                [&](T &obj) {
                    return 0.0f == get_detection(obj).get_confidence();
                }),
            objects.end());
    }

    /**
     * @brief IOU-based NMS on a vector of HailoDetection objects (convenience overload).
     */
    inline void nms(std::vector<HailoDetection> &objects, const float iou_thr,
        bool should_nms_cross_classes = false)
    {
        nms(objects, [](HailoDetection &d) -> HailoDetection& { return d; },
            iou_thr, should_nms_cross_classes);
    }

    /**
     * @brief Filter items by score threshold
     *
     * @param items     -  std::vector<T>
     *        The items to filter.
     *
     * @param score_fn  -  ScoreFn
     *        A callable that returns the score of an item.
     *
     * @param threshold -  float
     *        Minimum score to keep an item.
     *
     * @return std::vector<T>  Items with score >= threshold.
     */
    template <typename T, typename ScoreFn>
    std::vector<T> threshold_filter(const std::vector<T> &items, ScoreFn score_fn, float threshold)
    {
        std::vector<T> filtered;
        for (const auto &item : items) {
            if (threshold <= score_fn(item)) {
                filtered.push_back(item);
            }
        }
        return filtered;
    }

}

#endif  // _POSTPROCESSES_COMMON_NMS_HPP_
