/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
// General includes
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iterator>
#include <string>
#include <tuple>
#include <vector>

// Hailo includes
#include "common/math.hpp"
#include "hailo_objects.hpp"
#include "common/tensors.hpp"
#include "common/nms.hpp"
#include "common/labels/coco_eighty.hpp"
#include "nanodet.hpp"

// xtensor includes
#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"
#include "xtensor/xtensor.hpp"
#include "xtensor/xio.hpp"
#include "xtensor/xsort.hpp"
#include "xtensor/xview.hpp"
using namespace xt::placeholders;

#define SCORE_THRESHOLD 0.5
#define IOU_THRESHOLD 0.6
#define NUM_CLASSES 80

/**
 * @brief Split the raw output tensors into boxes and scores
 * 
 * @param tensors  -  std::vector<HailoTensorPtr>
 *        The network output tensors
 * 
 * @param num_classes  -  int
 *        Number of classes
 * 
 * @param regression_length  -  int
 *        Regression length of anchors
 *  
 * @return std::pair<std::vector<xt::xarray<float>>, xt::xarray<float>> 
 *         The separated score and box vectors
 */
std::pair<std::vector<xt::xarray<float>>, xt::xarray<float>> get_boxes_and_scores(std::vector<HailoTensorPtr> &tensors,
                                                                                  int num_classes,
                                                                                  int regression_length)
{
    // The boxes we will return
    std::vector<xt::xarray<float>> boxes(tensors.size());
    
    // Prepare the scores xarray at the size we will fill in in-place
    int total_scores = 0;
    for (uint i=0; i < tensors.size(); i++) { total_scores += tensors[i]->width() * tensors[i]->height(); }
    std::vector<size_t> shape = { (long unsigned int)total_scores, (long unsigned int)num_classes };
    xt::xarray<float> scores(shape);
    int view_index = 0;

    for (uint i=0; i < tensors.size(); i++)
    {
        // Extract and dequantize the layer
        auto layer = common::dequantize(common::get_xtensor(tensors[i]), tensors[i]->vstream_info().quant_info.qp_scale, tensors[i]->vstream_info().quant_info.qp_zp);
        int num_proposals = layer.shape(0)*layer.shape(1);

        // From the layer extract the scores
        auto layer_scores = xt::view(layer, xt::all(), xt::all(), xt::range(_, num_classes));
        xt::view(scores, xt::range(view_index, view_index + num_proposals), xt::all()) = xt::reshape_view(layer_scores, {num_proposals, num_classes});
        view_index += num_proposals;

        // From the layer extract the bounding boxes
        auto layer_boxes = xt::view(layer, xt::all(), xt::all(), xt::range(num_classes, _));
        boxes[i] = xt::reshape_view(layer_boxes, {num_proposals, 4, regression_length + 1});
    }

    return std::pair<std::vector<xt::xarray<float>>, xt::xarray<float>>( boxes, scores );
}

/**
 * @brief Decodes the box tensors into HailoDetections
 * 
 * @param raw_boxes  -  std::vector<xt::xarray<float>>
 *        The unprocessed box tensors
 * 
 * @param scores  -  xt::xarray<float>
 *        The stacked and processed (sigmoid) scores
 * 
 * @param network_dims  -  std::vector<int>
 *        The input dimensions of the network ex: {416,416}
 * 
 * @param strides  -  std::vector<int>
 *        The strides of each layer
 * 
 * @param regression_length  -  int
 *        Regression length of anchors
 * 
 * @return std::vector<HailoDetection> 
 *         The vector of detected objects before NMS
 */
std::vector<HailoDetection> decode_boxes(std::vector<xt::xarray<float>> raw_boxes,
                                            xt::xarray<float> scores,
                                            std::vector<int> network_dims,
                                            std::vector<int> strides,
                                            int regression_length)
{
    int strided_width, strided_height, class_index;
    std::vector<HailoDetection> detections;
    int instance_index = 0;
    float confidence = 0.0;
    std::string label;
    for (uint i=0; i < raw_boxes.size(); i++)
    {
        strided_width = network_dims[0] / strides[i];
        strided_height = network_dims[1] / strides[i];

        // Create a meshgrid of the proper strides
        xt::xarray<int> grid_x = xt::arange(0, strided_width);
        xt::xarray<int> grid_y = xt::arange(0, strided_height);
        auto mesh = xt::meshgrid(grid_x, grid_y);
        grid_x = std::get<1>(mesh);
        grid_y = std::get<0>(mesh);

        // Use the meshgrid to build up box center prototypes
        auto ct_row = (xt::flatten(grid_y) + 0.5) * strides[i];
        auto ct_col = (xt::flatten(grid_x) + 0.5) * strides[i];
        auto centers = xt::stack(xt::xtuple(ct_col, ct_row, ct_col, ct_row), 1);

        // Box distribution to distance
        auto regression_distance =  xt::reshape_view(xt::arange(0, regression_length + 1), {1, 1, regression_length + 1});
        common::softmax_3D(raw_boxes[i].data(), raw_boxes[i].shape(0), raw_boxes[i].shape(1), raw_boxes[i].shape(2));
        auto box_distance = raw_boxes[i] * regression_distance;
        xt::xarray<float> reduced_distances = xt::sum(box_distance, {2});
        auto strided_distances = reduced_distances * strides[i];

        // Decode boxes
        auto distance_view1 = xt::view(strided_distances, xt::all(), xt::range(_, 2)) * -1;
        auto distance_view2 = xt::view(strided_distances, xt::all(), xt::range(2, _));
        auto distance_view = xt::concatenate(xt::xtuple(distance_view1, distance_view2), 1);
        auto decoded_boxes = centers + distance_view;

        for (uint j=0; j < decoded_boxes.shape(0); j++)
        {
            HailoBBox bbox(decoded_boxes(j, 0) / network_dims[0],
                           decoded_boxes(j, 1) / network_dims[1],
                           (decoded_boxes(j, 2) - decoded_boxes(j, 0)) / network_dims[0],
                           (decoded_boxes(j, 3) - decoded_boxes(j, 1)) / network_dims[1]);
            class_index = xt::argmax(xt::row(scores, instance_index))(0);
            confidence = scores(instance_index, class_index);
            instance_index++;
            if (confidence < SCORE_THRESHOLD)
                continue;

            label = common::coco_eighty[class_index + 1];
            HailoDetection detected_instance(bbox, class_index, label, confidence);
            detections.push_back(detected_instance);
        }
    }
    return detections;
}

/**
 * @brief Perform nanodet style postprocessing, returning finalized HailoDetections
 * 
 * @param tensors  -  std::vector<HailoTensorPtr>
 *        The network output tensors
 * 
 * @param network_dims  -  std::vector<int>
 *        The input dimensions of the network ex: {416,416}
 * 
 * @param strides  -  std::vector<int>
 *        The strides of each layer
 * 
 * @param regression_length  -  int
 *        Regression length of anchors
 * 
 * @param num_classes  -  int
 *        Number of classes
 * 
 * @return std::vector<HailoDetection> 
 *         The finalized vector of detected objects
 */
std::vector<HailoDetection> nanodet_postprocess(std::vector<HailoTensorPtr> &tensors,
                                                   std::vector<int> network_dims,
                                                   std::vector<int> strides,
                                                   int regression_length,
                                                   int num_classes)
{
    std::vector<HailoDetection> detections;
    if (tensors.size() == 0)
    {
        return detections;
    }

    auto boxes_and_scores = get_boxes_and_scores(tensors, num_classes, regression_length);
    std::vector<xt::xarray<float>> raw_boxes = boxes_and_scores.first;
    xt::xarray<float> scores = boxes_and_scores.second;

    // Calculate the sigmoid of the scores
    common::sigmoid(scores.data(), scores.size());

    // Decode the boxes
    detections = decode_boxes(raw_boxes, scores, network_dims, strides, regression_length);

    // Filter with NMS
    common::nms(detections, IOU_THRESHOLD, true);

    return detections;
}

/**
 * @brief nanodet_repvgg postprocess
 *        Provides network specific paramters
 * 
 * @param roi  -  HailoROIPtr
 *        The roi that contains the ouput tensors
 */
void nanodet_repvgg(HailoROIPtr roi)
{
    // anchor params
    int regression_length = 10;
    std::vector<int> strides = {8, 16, 32};
    std::vector<int> network_dims = {416, 416};

    std::vector<HailoTensorPtr> tensors = roi->get_tensors();
    std::vector<HailoDetection> detections = nanodet_postprocess(tensors, network_dims, strides, regression_length, NUM_CLASSES);
    hailo_common::add_detections(roi, detections);
}

//******************************************************************
//  DEFAULT FILTER
//******************************************************************
void filter(HailoROIPtr roi)
{
    nanodet_repvgg(roi);
}