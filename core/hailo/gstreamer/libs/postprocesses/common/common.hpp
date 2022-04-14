/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#ifndef _HAILO_COMMON_HPP_
#define _HAILO_COMMON_HPP_

#include <gst/gst.h>
#include "hailo_detection.hpp"
#include "hailo_frame.hpp"
#include "tensor_meta.hpp"
#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"
#include "xtensor/xeval.hpp"
#include "xtensor/xsort.hpp"
#include "xtensor/xview.hpp"
#include "xtensor/xio.hpp"

namespace common
{

    xt::xarray<float> dequantize(const xt::xarray<uint16_t> &input, const float &qp_scale, const float &qp_zp)
    {
        // Rescale the input using the given scale and zero-point
        xt::xarray<float> rescaled_data = (input - qp_zp) * qp_scale;
        return rescaled_data;
    }

    //-------------------------------
    // COMMON TRANSFORMS
    //-------------------------------
    xt::xarray<float> dequantize(const xt::xarray<uint8_t> &input, const float &qp_scale, const float &qp_zp)
    {
        // Rescale the input using the given scale and zero-point
        auto rescaled_data = (input - qp_zp) * qp_scale;
        return rescaled_data;
    }

    xt::xarray<uint8_t> xtensor_from_HailoTensor(HailoTensorPtr &tensor)
    {
        // Adapt a HailoTensorPtr to an xarray (quantized)
        std::vector<std::size_t> shape = {tensor->width, tensor->height, tensor->channels};
        int size = tensor->width * tensor->height * tensor->channels;
        xt::xarray<uint8_t> xtensor = xt::adapt(tensor->data, size, xt::no_ownership(), shape);
        return xtensor;
    }

    //-------------------------------
    // COMMON META ROUTINES
    //-------------------------------
    const void copy_data_to_structure(GstStructure *structure, std::string field_name, const void *data_buffer, int size)
    {
        // Copy the given buffer to the given gst structure
        GVariant *data_variant = g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE, data_buffer, size, 1);
        std::string size_name = field_name + "_size";
        gst_structure_set(structure,
                          field_name.c_str(), G_TYPE_VARIANT, data_variant,
                          size_name.c_str(), G_TYPE_INT, size, NULL);
    }

    const void copy_int_to_structure(GstStructure *structure, std::string field_name, int size)
    {
        // Copy the given buffer to the given gst structure
        gst_structure_set(structure, field_name.c_str(), G_TYPE_INT, size, NULL);
    }

    const void update_frame(HailoFramePtr hailo_frame, std::vector<DetectionObject> &detections)
    {
        // Add each detection to the hailo_frame, then clear the tensors.
        for (auto &det : detections)
        {
            hailo_frame->add_detection(det);
        }
    }

    //-------------------------------
    // COMMON FILTERS
    //-------------------------------
    xt::xarray<int> top_k(xt::xarray<uint8_t> &data, const int k)
    {
        // First we negate the array so that we sort in descending order.
        auto descending_order_array = xt::eval(-data);
        // Partition the array so the k "smallest" items are first (argpartition returns this as an index array).
        // Since we negated the values first, we are actually partitioning the largest values.
        xt::xarray<int> krange = xt::arange<int>(0, k);
        xt::xarray<int> index_array = xt::argpartition(descending_order_array, krange, xt::xnone());

        // We only want the k first indices, so make a view that "takes" these.
        auto topk_index_array = xt::view(xt::reshape_view(index_array, data.shape()), xt::all(), xt::range(0, k));
        return topk_index_array;
    }

    float iou_calc(const DetectionObject &box_1, const DetectionObject &box_2)
    {
        // Calculate IOU between two detection boxes
        const float width_of_overlap_area = std::min(box_1.xmax, box_2.xmax) - std::max(box_1.xmin, box_2.xmin);
        const float height_of_overlap_area = std::min(box_1.ymax, box_2.ymax) - std::max(box_1.ymin, box_2.ymin);
        const float positive_width_of_overlap_area = std::max(width_of_overlap_area, 0.0f);
        const float positive_height_of_overlap_area = std::max(height_of_overlap_area, 0.0f);
        const float area_of_overlap = positive_width_of_overlap_area * positive_height_of_overlap_area;
        const float box_1_area = (box_1.ymax - box_1.ymin) * (box_1.xmax - box_1.xmin);
        const float box_2_area = (box_2.ymax - box_2.ymin) * (box_2.xmax - box_2.xmin);
        // The IOU is a ratio of how much the boxes overlap vs their size outside the overlap.
        // Boxes that are similar will have a higher overlap threshold.
        return area_of_overlap / (box_1_area + box_2_area - area_of_overlap);
    }

    void nms(std::vector<DetectionObject> &objects, const float iou_thr, bool should_nms_cross_classes = false)
    {
        // The network may propose multiple detections of similar size/score,
        // which are actually the same detection. We want to filter out the lesser
        // detections with a simple nms.
        for (uint index = 0; index < objects.size(); index++)
        {
            for (uint jindex = index + 1; jindex < objects.size(); jindex++)
            {
                if (should_nms_cross_classes || (objects[index].class_id == objects[jindex].class_id))
                {
                    // For each detection, calculate the IOU against each following detection.
                    float iou = iou_calc(objects[index], objects[jindex]);
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

    xt::xarray<float> softmax_xtensor(xt::xarray<float> &scores)
    {
        // Compute softmax values for each sets of scores in x.
        auto maxes = xt::amax(scores, -1);
        xt::xarray<float> e_scores = xt::exp(scores - xt::expand_dims(maxes, 2));
        return std::move(e_scores / xt::expand_dims(xt::sum(e_scores, -1), 2));
    }

    void softmax_1D(float *data, const int size)
    {
        float sum = 0;
        for (int i = 0; i < size; i++)
            sum += std::exp(data[i]);
        for (int i = 0; i < size; i++)
            data[i] = std::exp(data[i]) / sum;
    }

    void softmax_2D(float *data, const int num_rows, const int num_cols)
    {
        int size = num_rows * num_cols;
        for (int i = 0; i < size; i += num_cols)
            softmax_1D(&data[i], num_cols);
    }

    void softmax_3D(float *data, const int num_rows, const int num_cols, const int num_features)
    {
        int size = num_rows * num_cols * num_features;
        for (int i = 0; i < size; i += num_cols * num_rows)
            softmax_2D(&data[i], num_rows, num_cols);
    }

    void sigmoid(float *data, const int size)
    {
        for (int i = 0; i < size; i++)
            data[i] = 1.0f / (1.0f + std::exp(-1.0 * data[i]));
    }
}
#endif /*_HAILO_COMMON_HPP_ */
