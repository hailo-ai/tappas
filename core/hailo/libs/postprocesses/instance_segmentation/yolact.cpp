/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <iterator>
#include <list>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"
#include "xtensor/xbuilder.hpp"
#include "xtensor/xcontainer.hpp"
#include "xtensor/xeval.hpp"
#include "xtensor/xtensor.hpp"
#include "xtensor/xindex_view.hpp"
#include "xtensor/xio.hpp"
#include "xtensor/xmanipulation.hpp"
#include "xtensor/xmath.hpp"
#include <xtensor/xnoalias.hpp>
#include "xtensor/xoperation.hpp"
#include "xtensor/xpad.hpp"
#include "xtensor/xrandom.hpp"
#include "xtensor/xshape.hpp"
#include "xtensor/xsort.hpp"
#include "xtensor/xstrided_view.hpp"
#include "xtensor/xview.hpp"

#include "common/labels/coco_eighty.hpp"
#include "common/labels/yolact_twenty.hpp"
#include "common/math.hpp"
#include "common/nms.hpp"
#include "common/tensors.hpp"
#include "yolact.hpp"
#include "mask_decoding.hpp"

// yolact_networks_specific parameters
#define SCORE_THRESHOLD 0.5
#define NMS_THRESHOLD 0.5
#define IMAGE_SIZE 512

// Supported Networks
enum network_type
{
    YOLACT1_6,
    YOLACT800,
    YOLACT20CLASSES
};

/**
 * @brief Extract the anchors
 *
 * @param image_size original height/width of image before inference
 * @return xt::xarray<float> anchors
 */
xt::xarray<float> get_anchors(const int image_size)
{
    // Prepare needed variables
    float x, y;
    uint row_index = 0;
    xt::xarray<uint> conv_layer = {64, 32, 16, 8, 4};
    xt::xarray<uint> scale_steps = {24, 48, 96, 192, 384};

    // Calculate the shape of the anchors in advanced to cut concatination time and memory.
    uint num_rows = xt::sum<int>(xt::pow(conv_layer, 2) * 9)(0);
    std::vector<std::size_t> shape = {num_rows, 4};
    xt::xarray<float, xt::layout_type::row_major> anchors(shape);

    // Prepare the scales.
    std::vector<xt::xarray<float>> scales = {};
    for (uint i = 0; i < scale_steps.shape(0); i++)
    {
        scales.push_back({scale_steps(i) * (float)pow(2, (0 / 3.0)),
                          scale_steps(i) * (float)pow(2, (1 / 3.0)),
                          scale_steps(i) * (float)pow(2, (2 / 3.0))});
    }

    // Calculate each anchor.
    for (uint index = 0; index < conv_layer.shape(0); index++)
    {
        std::vector<xt::xarray<float>> anchor_data = {};
        xt::xarray<float> anchor_data_layer;
        for (uint j = 0; j < conv_layer(index); j++)
        {
            for (uint i = 0; i < conv_layer(index); i++)
            {
                x = (i + 0.5) / conv_layer(index);
                y = (j + 0.5) / conv_layer(index);
                for (float scale : scales[index])
                {
                    for (float ar : {1.0, 0.5, 2.0})
                    {
                        ar = sqrt(ar);
                        anchors(row_index, 0) = x;
                        anchors(row_index, 1) = y;
                        anchors(row_index, 2) = scale * ar / image_size;
                        anchors(row_index, 3) = scale / ar / image_size;
                        row_index++;
                    }
                }
            }
        }
    }
    return anchors;
}

/**
 * @brief Gets the maximum of the columns of each row, skipping class 0 (background)
 *
 * @param data data to iterate
 * @param size size of data
 * @param num_rows number of rows in data
 * @param num_cols number of columns in data
 * @return xt::xarray<float> max values
 */
xt::xarray<float> amax_row(float *data, const int size, const int num_rows, const int num_cols)
{
    xt::xarray<float>::shape_type shape = {(long unsigned int)num_rows};
    xt::xarray<float> max_values(shape);
    for (int i = 0; i < size; i += num_cols)
    {
        // We always start one over to skip class 0 (background)
        max_values(i / num_cols) = *std::max_element(data + i + 1, data + i + num_cols);
    }
    return max_values;
}

/**
 * @brief Select indices who's score are above a given threshold
 *
 * @param data data
 * @param size size of data
 * @param threshold threshold of data
 * @return std::vector<int> the indices
 */
std::vector<int> filter_scores(float *data, const int size, const float threshold)
{
    std::vector<int> indices;
    for (int i = 0; i < size; i++)
    {
        if (data[i] > threshold)
        {
            indices.emplace_back(i);
        }
    }
    return indices;
}
/**
 * @brief decode_boxes from the priors and locations
 *
 * @param locations The predicted bounding boxes of size [num_priors, 4]
 * @param anchors The anchor box coords with size [num_priors, 4]
 * @return xt::xarray<float> A tensor of decoded relative coordinates in point form
             form with size [num_anchor, 4]
 */
xt::xarray<float> decode_boxes(const xt::xarray<float> &locations, const xt::xarray<float> &anchors)
{
    /*
    Decode predicted bbox coordinates using the same scheme
    employed by Yolov2: https://arxiv.org/pdf/1612.08242.pdf

      b_x = (sigmoid(pred_x) - .5) / conv_w + anchor_x
      b_y = (sigmoid(pred_y) - .5) / conv_h + anchor_y
      b_w = anchor_w * exp(loc_w)
      b_h = anchor_h * exp(loc_h)

    Note that loc is inputed as [(s(x)-.5)/conv_w, (s(y)-.5)/conv_h, w, h]
    while anchors are inputed as [x, y, w, h] where each coordinate
    is relative to size of the image (even sigmoid(x)). We do this
    in the network by dividing by the 'cell size', which is just
    the size of the convouts.

    Also note that anchor_x and anchor_y are center coordinates which
    is why we have to subtract .5 from sigmoid(pred_x and pred_y).

    */
    xt::xarray<float> variances = {0.1, 0.2};
    // Calculate the x and y
    auto boxes_xy = xt::view(anchors, xt::all(), xt::range(_, 2)) + (xt::view(locations, xt::all(), xt::range(_, 2)) * variances(0) * xt::view(anchors, xt::all(), xt::range(2, _)));
    // Calculate the width and height
    auto boxes_wh = xt::view(anchors, xt::all(), xt::range(2, _)) * xt::exp(xt::view(locations, xt::all(), xt::range(2, _)) * variances(1));

    // Concatenate the parameters together
    auto boxes = xt::concatenate(xt::xtuple(boxes_xy - (boxes_wh / 2), boxes_wh), 1);
    return boxes;
}

/**
 * @brief
 *
 * @param objects empty vector of HailoDetection
 * @param all_anchors the anchors
 * @param all_scores the scores
 * @param all_boxes the boxes data
 * @param all_masks the masks data
 * @param score_threshold treshold to filter out low scores
 */
void detect_instances(std::vector<HailoDetection> &objects,
                      auto &all_anchors,
                      auto &all_scores,
                      auto &all_boxes,
                      auto &all_masks,
                      const float score_threshold, network_type network)
{
    // First get the max score of each row
    xt::xarray<float, xt::layout_type::row_major> max_scores = amax_row(all_scores.data(), all_scores.size(), all_scores.shape(0), all_scores.shape(1));

    // Filter out any detection below threshold
    std::vector<int> indices_above_threshold = filter_scores(max_scores.data(), max_scores.size(), score_threshold);

    // Keep only the indices above thresholds from our arrays
    auto scores = xt::view(all_scores, xt::keep(indices_above_threshold), xt::drop(0));
    auto boxes = xt::view(all_boxes, xt::keep(indices_above_threshold), xt::all());
    auto masks = xt::view(all_masks, xt::keep(indices_above_threshold), xt::all());
    auto anchors = xt::view(all_anchors, xt::keep(indices_above_threshold), xt::all());

    // Decode the detection+ boxes
    xt::xarray<float> decoded_boxes = decode_boxes(boxes, anchors);

    // Take the hyperbolic tangent of each element in the mask
    xt::xarray<float> tanned_masks = xt::tanh(masks);

    // Encode the detection objects
    int class_index;
    float confidence, w, h, xmin, ymin = 0.0f;
    for (uint index = 0; index < indices_above_threshold.size(); index++)
    {
        // Get the box parameters for this box
        xmin = decoded_boxes(index, 0);
        ymin = decoded_boxes(index, 1);
        w = decoded_boxes(index, 2);
        h = decoded_boxes(index, 3);

        // We add 1 to class_index since we excluded class 0 (background)
        class_index = xt::argmax(xt::row(scores, index))(0) + 1;
        std::string label;
        if (network == network_type::YOLACT20CLASSES)
        {
            label = common::yolact_twenty[class_index];
        }
        else
        {
            label = common::coco_eighty[class_index];
        }
        confidence = scores(index, class_index - 1);                                                           // Decrement class_index since scores excludes class 0 (background)
        xt::xarray<float> mask_coefficients = xt::squeeze(xt::view(tanned_masks, xt::keep(index), xt::all())); // We are only interested in the coefficients of this instance

        HailoBBox bbox(xmin, ymin, w, h);
        HailoDetection detected_instance(bbox, class_index, label, confidence);

        std::vector<float> data(mask_coefficients.shape(0));
        memcpy(data.data(), mask_coefficients.data(), sizeof(float) * mask_coefficients.shape(0));

        detected_instance.add_object((std::make_shared<HailoMatrix>(data, mask_coefficients.shape(0), 1)));
        objects.push_back(detected_instance);
    }
}

/**
 * @brief Perform Instance Segmentation postprocess
 *
 * @param tensors output tensors by name
 * @param anchors the anchors
 * @param num_classes number of classes in yolact
 * @param image_size height/width of the input to the inference
 * @param score_threshold treshold to filter out low scores
 * @param nms_threshold threshold to fitler out detected bboxes that are too similar in order to remove duplicates
 * @return std::vector<HailoDetection> final detections vector with mask to each detection
 */
std::vector<HailoDetection> instance_segmentation_post(std::map<std::string, HailoTensorPtr> tensors,
                                                       const xt::xarray<float> &anchors,
                                                       const int num_classes,
                                                       const int image_size,
                                                       const float score_threshold,
                                                       const float nms_threshold,
                                                       network_type network)
{
    std::string PROTO_LAYER, BBOX_0, MASK_0, CONF_0, BBOX_1, MASK_1, CONF_1, BBOX_2, MASK_2, CONF_2, BBOX_3, MASK_3, CONF_3, BBOX_4, MASK_4, CONF_4;
    std::string NETWORK_GROUP;
    if (network == network_type::YOLACT20CLASSES)
    {
        NETWORK_GROUP = "yolact_regnetx_800mf_20classes";
        // Output layer names for yolact_regnetx_800mf_fpn_20classes network
        PROTO_LAYER = NETWORK_GROUP + "/conv86";
        // Set 0
        BBOX_0 = NETWORK_GROUP + "/conv81";
        MASK_0 = NETWORK_GROUP + "/conv83";
        CONF_0 = NETWORK_GROUP + "/conv82";
        // Set 1
        BBOX_1 = NETWORK_GROUP + "/conv74";
        MASK_1 = NETWORK_GROUP + "/conv76";
        CONF_1 = NETWORK_GROUP + "/conv75";
        // Set 2
        BBOX_2 = NETWORK_GROUP + "/conv63";
        MASK_2 = NETWORK_GROUP + "/conv65";
        CONF_2 = NETWORK_GROUP + "/conv64";
        // Set 3
        BBOX_3 = NETWORK_GROUP + "/conv66";
        MASK_3 = NETWORK_GROUP + "/conv68";
        CONF_3 = NETWORK_GROUP + "/conv67";
        // Set 4
        BBOX_4 = NETWORK_GROUP + "/conv69";
        MASK_4 = NETWORK_GROUP + "/conv71";
        CONF_4 = NETWORK_GROUP + "/conv70";
    }
    else if (network == network_type::YOLACT1_6)
    {
        NETWORK_GROUP = "yolact_regnetx_1_6gf";

        // output layers for yolact_regnetx_1_6gf
        PROTO_LAYER = NETWORK_GROUP + "/conv92";
        // Set 0
        BBOX_0 = NETWORK_GROUP + "/conv87";
        MASK_0 = NETWORK_GROUP + "/conv89";
        CONF_0 = NETWORK_GROUP + "/conv88";
        // Set 1
        BBOX_1 = NETWORK_GROUP + "/conv79";
        MASK_1 = NETWORK_GROUP + "/conv81";
        CONF_1 = NETWORK_GROUP + "/conv80";
        // Set 2
        BBOX_2 = NETWORK_GROUP + "/conv67";
        MASK_2 = NETWORK_GROUP + "/conv69";
        CONF_2 = NETWORK_GROUP + "/conv68";
        // Set 3
        BBOX_3 = NETWORK_GROUP + "/conv70";
        MASK_3 = NETWORK_GROUP + "/conv72";
        CONF_3 = NETWORK_GROUP + "/conv71";
        // Set 4
        BBOX_4 = NETWORK_GROUP + "/conv73";
        MASK_4 = NETWORK_GROUP + "/conv75";
        CONF_4 = NETWORK_GROUP + "/conv74";
    }
    else if (network == network_type::YOLACT800)
    {
        NETWORK_GROUP = "yolact_regnetx_800mf";
        // // Output layer names for yolact_regnetx_800mf_fpn
        PROTO_LAYER = NETWORK_GROUP + "/conv86";
        // Set 0
        BBOX_0 = NETWORK_GROUP + "/conv81";
        MASK_0 = NETWORK_GROUP + "/conv83";
        CONF_0 = NETWORK_GROUP + "/conv82";
        // Set 1
        BBOX_1 = NETWORK_GROUP + "/conv73";
        MASK_1 = NETWORK_GROUP + "/conv75";
        CONF_1 = NETWORK_GROUP + "/conv74";
        // Set 2
        BBOX_2 = NETWORK_GROUP + "/conv61";
        MASK_2 = NETWORK_GROUP + "/conv63";
        CONF_2 = NETWORK_GROUP + "/conv62";
        // Set 3
        BBOX_3 = NETWORK_GROUP + "/conv64";
        MASK_3 = NETWORK_GROUP + "/conv66";
        CONF_3 = NETWORK_GROUP + "/conv65";
        // Set 4
        BBOX_4 = NETWORK_GROUP + "/conv67";
        MASK_4 = NETWORK_GROUP + "/conv69";
        CONF_4 = NETWORK_GROUP + "/conv68";
    }
    else
    {
        throw std::invalid_argument("Network is not supported");
    }

    std::vector<HailoDetection> objects; // The detection meta we will eventually return

    // tensors gathering
    xt::xarray<float, xt::layout_type::row_major> proto = common::dequantize(common::get_xtensor(tensors[PROTO_LAYER]), tensors[PROTO_LAYER]->vstream_info().quant_info.qp_scale, tensors[PROTO_LAYER]->vstream_info().quant_info.qp_zp);

    // Set 0
    auto bbox_0 = common::dequantize(common::get_xtensor(tensors[BBOX_0]), tensors[BBOX_0]->vstream_info().quant_info.qp_scale, tensors[BBOX_0]->vstream_info().quant_info.qp_zp);
    auto mask_0 = common::dequantize(common::get_xtensor(tensors[MASK_0]), tensors[MASK_0]->vstream_info().quant_info.qp_scale, tensors[MASK_0]->vstream_info().quant_info.qp_zp);
    auto conf_0 = common::dequantize(common::get_xtensor(tensors[CONF_0]), tensors[CONF_0]->vstream_info().quant_info.qp_scale, tensors[CONF_0]->vstream_info().quant_info.qp_zp);
    // Set 1
    auto bbox_1 = common::dequantize(common::get_xtensor(tensors[BBOX_1]), tensors[BBOX_1]->vstream_info().quant_info.qp_scale, tensors[BBOX_1]->vstream_info().quant_info.qp_zp);
    auto mask_1 = common::dequantize(common::get_xtensor(tensors[MASK_1]), tensors[MASK_1]->vstream_info().quant_info.qp_scale, tensors[MASK_1]->vstream_info().quant_info.qp_zp);
    auto conf_1 = common::dequantize(common::get_xtensor(tensors[CONF_1]), tensors[CONF_1]->vstream_info().quant_info.qp_scale, tensors[CONF_1]->vstream_info().quant_info.qp_zp);
    // Set 2
    auto bbox_2 = common::dequantize(common::get_xtensor(tensors[BBOX_2]), tensors[BBOX_2]->vstream_info().quant_info.qp_scale, tensors[BBOX_2]->vstream_info().quant_info.qp_zp);
    auto mask_2 = common::dequantize(common::get_xtensor(tensors[MASK_2]), tensors[MASK_2]->vstream_info().quant_info.qp_scale, tensors[MASK_2]->vstream_info().quant_info.qp_zp);
    auto conf_2 = common::dequantize(common::get_xtensor(tensors[CONF_2]), tensors[CONF_2]->vstream_info().quant_info.qp_scale, tensors[CONF_2]->vstream_info().quant_info.qp_zp);
    // Set 3
    auto bbox_3 = common::dequantize(common::get_xtensor(tensors[BBOX_3]), tensors[BBOX_3]->vstream_info().quant_info.qp_scale, tensors[BBOX_3]->vstream_info().quant_info.qp_zp);
    auto mask_3 = common::dequantize(common::get_xtensor(tensors[MASK_3]), tensors[MASK_3]->vstream_info().quant_info.qp_scale, tensors[MASK_3]->vstream_info().quant_info.qp_zp);
    auto conf_3 = common::dequantize(common::get_xtensor(tensors[CONF_3]), tensors[CONF_3]->vstream_info().quant_info.qp_scale, tensors[CONF_3]->vstream_info().quant_info.qp_zp);
    // Set 4
    auto bbox_4 = common::dequantize(common::get_xtensor(tensors[BBOX_4]), tensors[BBOX_4]->vstream_info().quant_info.qp_scale, tensors[BBOX_4]->vstream_info().quant_info.qp_zp);
    auto mask_4 = common::dequantize(common::get_xtensor(tensors[MASK_4]), tensors[MASK_4]->vstream_info().quant_info.qp_scale, tensors[MASK_4]->vstream_info().quant_info.qp_zp);
    auto conf_4 = common::dequantize(common::get_xtensor(tensors[CONF_4]), tensors[CONF_4]->vstream_info().quant_info.qp_scale, tensors[CONF_4]->vstream_info().quant_info.qp_zp);

    // Reshape and stack the boxes
    auto bbox_0_reshaped = xt::reshape_view(bbox_0, {(int)bbox_0.shape(0) * (int)bbox_0.shape(1) * ((int)bbox_0.shape(2) / 4), 4});
    auto bbox_1_reshaped = xt::reshape_view(bbox_1, {(int)bbox_1.shape(0) * (int)bbox_1.shape(1) * ((int)bbox_1.shape(2) / 4), 4});
    auto bbox_2_reshaped = xt::reshape_view(bbox_2, {(int)bbox_2.shape(0) * (int)bbox_2.shape(1) * ((int)bbox_2.shape(2) / 4), 4});
    auto bbox_3_reshaped = xt::reshape_view(bbox_3, {(int)bbox_3.shape(0) * (int)bbox_3.shape(1) * ((int)bbox_3.shape(2) / 4), 4});
    auto bbox_4_reshaped = xt::reshape_view(bbox_4, {(int)bbox_4.shape(0) * (int)bbox_4.shape(1) * ((int)bbox_4.shape(2) / 4), 4});
    auto all_boxes = xt::concatenate(xt::xtuple(bbox_0_reshaped, bbox_1_reshaped, bbox_2_reshaped, bbox_3_reshaped, bbox_4_reshaped));

    // Reshape and stack the scores
    xt::xarray<float> conf_0_reshaped = xt::reshape_view(conf_0, {(int)conf_0.shape(0) * (int)conf_0.shape(1) * ((int)conf_0.shape(2) / num_classes), num_classes});
    xt::xarray<float> conf_1_reshaped = xt::reshape_view(conf_1, {(int)conf_1.shape(0) * (int)conf_1.shape(1) * ((int)conf_1.shape(2) / num_classes), num_classes});
    xt::xarray<float> conf_2_reshaped = xt::reshape_view(conf_2, {(int)conf_2.shape(0) * (int)conf_2.shape(1) * ((int)conf_2.shape(2) / num_classes), num_classes});
    xt::xarray<float> conf_3_reshaped = xt::reshape_view(conf_3, {(int)conf_3.shape(0) * (int)conf_3.shape(1) * ((int)conf_3.shape(2) / num_classes), num_classes});
    xt::xarray<float> conf_4_reshaped = xt::reshape_view(conf_4, {(int)conf_4.shape(0) * (int)conf_4.shape(1) * ((int)conf_4.shape(2) / num_classes), num_classes});
    xt::xarray<float> t2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    xt::xarray<float, xt::layout_type::row_major> confidences = xt::concatenate(xt::xtuple(conf_0_reshaped, conf_1_reshaped, conf_2_reshaped, conf_3_reshaped, conf_4_reshaped));

    // Compute softmax on these scores
    common::softmax_2D(confidences.data(), confidences.shape(0), confidences.shape(1));

    // Reshape and stack the masks
    auto mask_0_reshaped = xt::reshape_view(mask_0, {(int)mask_0.shape(0) * (int)mask_0.shape(1) * ((int)mask_0.shape(2) / 32), 32});
    auto mask_1_reshaped = xt::reshape_view(mask_1, {(int)mask_1.shape(0) * (int)mask_1.shape(1) * ((int)mask_1.shape(2) / 32), 32});
    auto mask_2_reshaped = xt::reshape_view(mask_2, {(int)mask_2.shape(0) * (int)mask_2.shape(1) * ((int)mask_2.shape(2) / 32), 32});
    auto mask_3_reshaped = xt::reshape_view(mask_3, {(int)mask_3.shape(0) * (int)mask_3.shape(1) * ((int)mask_3.shape(2) / 32), 32});
    auto mask_4_reshaped = xt::reshape_view(mask_4, {(int)mask_4.shape(0) * (int)mask_4.shape(1) * ((int)mask_4.shape(2) / 32), 32});
    auto all_masks = xt::concatenate(xt::xtuple(mask_0_reshaped, mask_1_reshaped, mask_2_reshaped, mask_3_reshaped, mask_4_reshaped));

    detect_instances(objects, anchors, confidences, all_boxes, all_masks, score_threshold, network);

    common::nms(objects, nms_threshold);

    decode_masks(objects, proto);

    // Return the objects
    return objects;
}

/**
 * @brief call the post process and add the detections to the roi
 *
 * @param roi the region of interest
 */
void yolact(HailoROIPtr roi, network_type network, void *params_void_ptr)
{
    YolactParams *params = reinterpret_cast<YolactParams *>(params_void_ptr);
    int num_classes;
    if (network == network_type::YOLACT20CLASSES)
    {
        num_classes = 20; // real_number_of_classes + 1 for background
    }
    else
    {
        num_classes = 81;
    }
    const xt::xarray<float> anchors = params->anchors;
    std::map<std::string, HailoTensorPtr> tensors = roi->get_tensors_by_name();
    std::vector<HailoDetection> detections = instance_segmentation_post(tensors, anchors, num_classes, IMAGE_SIZE,
                                                                        SCORE_THRESHOLD, NMS_THRESHOLD, network);
    hailo_common::add_detections(roi, detections);
}

void yolact_20_classes(HailoROIPtr roi, void *params_void_ptr)
{
    yolact(roi, network_type::YOLACT20CLASSES, params_void_ptr);
}

void yolact800mf(HailoROIPtr roi, void *params_void_ptr)
{
    yolact(roi, network_type::YOLACT800, params_void_ptr);
}

void yolact1_6gf(HailoROIPtr roi, void *params_void_ptr)
{
    yolact(roi, network_type::YOLACT1_6, params_void_ptr);
}

/**
 * @brief default filter function
 *
 * @param roi the region of interest
 */
void filter(HailoROIPtr roi, void *params_void_ptr)
{
    yolact(roi, network_type::YOLACT1_6, params_void_ptr);
}

YolactParams *init(const std::string config_path, const std::string function_name)
{
    YolactParams *params = new YolactParams(get_anchors(IMAGE_SIZE));
    return params;
}

void free_resources(void *params_void_ptr)
{
    YolactParams *params = reinterpret_cast<YolactParams *>(params_void_ptr);
    delete params;
}
