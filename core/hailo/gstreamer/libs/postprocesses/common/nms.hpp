/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once

#include "hailo_objects.hpp"
#include "hailo_common.hpp"
namespace common
{

    float iou_calc(const HailoBBox &box_1, const HailoBBox &box_2)
    {
        // Calculate IOU between two detection boxes
        const float width_of_overlap_area = std::min(box_1.xmax(), box_2.xmax()) - std::max(box_1.xmin(), box_2.xmin());
        const float height_of_overlap_area = std::min(box_1.ymax(), box_2.ymax()) - std::max(box_1.ymin(), box_2.ymin());
        const float positive_width_of_overlap_area = std::max(width_of_overlap_area, 0.0f);
        const float positive_height_of_overlap_area = std::max(height_of_overlap_area, 0.0f);
        const float area_of_overlap = positive_width_of_overlap_area * positive_height_of_overlap_area;
        const float box_1_area = (box_1.ymax() - box_1.ymin()) * (box_1.xmax() - box_1.xmin());
        const float box_2_area = (box_2.ymax() - box_2.ymin()) * (box_2.xmax() - box_2.xmin());
        // The IOU is a ratio of how much the boxes overlap vs their size outside the overlap.
        // Boxes that are similar will have a higher overlap threshold.
        return area_of_overlap / (box_1_area + box_2_area - area_of_overlap);
    }

    /**
     * @brief Perform IOU based NMS on a vector of NewHailoDetection objects
     * 
     * @param objects  -  std::vector<NewHailoDetection>
     *        The detections to perform NMS on.
     *
     * @param iou_thr  -  float
     *        Threshold for IOU filtration
     *
     * @param should_nms_cross_classes  -  bool
     *        If true, then apply NMS regardless of class differences. Default false.
     */
    void nms(std::vector<NewHailoDetection> &objects, const float iou_thr, bool should_nms_cross_classes = false)
    {
        // The network may propose multiple detections of similar size/score,
        // which are actually the same detection. We want to filter out the lesser
        // detections with a simple nms.
        for (uint index = 0; index < objects.size(); index++)
        {
            for (uint jindex = index + 1; jindex < objects.size(); jindex++)
            {
                if (should_nms_cross_classes || (objects[index].get_class_id() == objects[jindex].get_class_id()))
                {
                    // For each detection, calculate the IOU against each following detection.
                    float iou = iou_calc(objects[index].get_bbox(), objects[jindex].get_bbox());
                    // If the IOU is above threshold, then we have two similar detections,
                    // and want to delete the one.
                    if (iou >= iou_thr)
                    {
                        // The detections are arranged in highest score order,
                        // so we want to erase the latter detection.
                        objects.erase(objects.begin() + jindex);
                        jindex--; // Step back jindex since we just erased the current detection.
                    }
                }
            }
        }
    }

    /**
     * @brief Perform IOU based NMS on detection objects of HailoRoi
     * 
     * @param hailo_roi  -  HailoROIPtr
     *        The HailoROI contains detections to perform NMS on.
     *
     * @param iou_thr  -  float
     *        Threshold for IOU filtration
     *
     * @param should_nms_cross_classes  -  bool
     *        If true, then apply NMS regardless of class differences. Default false.
     */
    void nms(HailoROIPtr hailo_roi, const float iou_thr, bool should_nms_cross_classes = false)
    {
        // The network may propose multiple detections of similar size/score,
        // which are actually the same detection. We want to filter out the lesser
        // detections with a simple nms.

        std::vector<NewHailoDetectionPtr> objects = hailo_common::get_hailo_detections(hailo_roi);
        for (uint index = 0; index < objects.size(); index++)
        {
            for (uint jindex = index + 1; jindex < objects.size(); jindex++)
            {
                if (should_nms_cross_classes || (objects[index]->get_class_id() == objects[jindex]->get_class_id()))
                {
                    // For each detection, calculate the IOU against each following detection.
                    float iou = iou_calc(objects[index]->get_bbox(), objects[jindex]->get_bbox());
                    // If the IOU is above threshold, then we have two similar detections,
                    // and want to delete the one.
                    if (iou >= iou_thr)
                    {
                        // The detections are arranged in highest score order,
                        // so we want to erase the latter detection.
                        hailo_roi->remove_object(objects[jindex]);
                        objects.erase(objects.begin() + jindex);
                        jindex--; // Step back jindex since we just erased the current detection.
                    }
                }
            
            }
        }
    }
}