/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <gst/gst.h>
#include <iostream>
#include <iterator>
#include <list>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "common/common.hpp"
#include "common/labels/yolact_twenty.hpp"
#include "yolact_postprocess.hpp"
#include "hailo_detection.hpp"
#include "tensor_meta.hpp"
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

//******************************************************************
// YOLACT NETWORK SPECIFIC PARAMETERS
//******************************************************************
#define SCORE_THRESHOLD 0.5
#define NMS_THRESHOLD 0.5

// Output layer names for yolact_regnetx_800mf_fpn_20classes network
const char *PROTO_LAYER = "yolact_regnetx_800mf_20classes/conv86";
// Set 0
const char *BBOX_0 = "yolact_regnetx_800mf_20classes/conv81";
const char *MASK_0 = "yolact_regnetx_800mf_20classes/conv83";
const char *CONF_0 = "yolact_regnetx_800mf_20classes/conv82";
// Set 1
const char *BBOX_1 = "yolact_regnetx_800mf_20classes/conv73";
const char *MASK_1 = "yolact_regnetx_800mf_20classes/conv75";
const char *CONF_1 = "yolact_regnetx_800mf_20classes/conv74";
// Set 2
const char *BBOX_2 = "yolact_regnetx_800mf_20classes/conv61";
const char *MASK_2 = "yolact_regnetx_800mf_20classes/conv63";
const char *CONF_2 = "yolact_regnetx_800mf_20classes/conv62";
// Set 3
const char *BBOX_3 = "yolact_regnetx_800mf_20classes/conv64";
const char *MASK_3 = "yolact_regnetx_800mf_20classes/conv66";
const char *CONF_3 = "yolact_regnetx_800mf_20classes/conv65";
// Set 4
const char *BBOX_4 = "yolact_regnetx_800mf_20classes/conv67";
const char *MASK_4 = "yolact_regnetx_800mf_20classes/conv69";
const char *CONF_4 = "yolact_regnetx_800mf_20classes/conv68";

//******************************************************************
// SETUP - ANCHOR EXTRACTION
//******************************************************************
xt::xarray<float> get_anchors(const gint image_size)
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

//******************************************************************
// MATH - HELPER FUNCTIONS
//******************************************************************
// Gets the maximum of the columns of each row, skipping class 0 (background)
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

// Selects indices who's score is above a given threshold
std::vector<int> filter(float *data, const int size, const float threshold)
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

// Compute tensor dot product along specified axes for arrays.
// In this case along axis 2 of the first matrix and axis 0 of the second.
xt::xarray<float> dot_product_axis_2(xt::xarray<float, xt::layout_type::row_major> matrix_1,
                                     xt::xarray<float, xt::layout_type::row_major> matrix_2)
{
    uint axis_length = matrix_1.shape(2);
    if (axis_length != matrix_2.shape(0))
    {
        throw std::invalid_argument("dot_product_axis_2 error: axis don't match!");
    }

    float row_sum;
    xt::xarray<float>::shape_type shape = {matrix_1.shape(0), matrix_1.shape(1)};
    xt::xarray<float, xt::layout_type::row_major> product_matrix(shape);
    for (uint i = 0; i < matrix_1.shape(0); ++i)
    {
        for (uint j = 0; j < matrix_1.shape(1); ++j)
        {
            row_sum = 0.0;
            for (uint k = 0; k < axis_length; ++k)
            {
                row_sum += matrix_1(i, j, k) * matrix_2(k);
            }
            product_matrix(i, j) = row_sum;
        }
    }
    return product_matrix;
}

//******************************************************************
// DETECTION & DECODING
//******************************************************************
void decode_masks(const std::vector<DetectionObject> &objects, const xt::xarray<float> &proto)
{
    /*
    Decode the mask coefficients of Yolact into a format that makes sense.
    */
    xt::xarray<float>::shape_type mask_shape = {proto.shape(0), proto.shape(1)};

    int proto_width = proto.shape(0);
    int proto_height = proto.shape(1);

    // For each detection object, crop the
    int xmin, ymin, xmax, ymax;
    gsize coefficients_size;
    for (auto &instance : objects)
    {
        // Gather the detection bounds for this instance,
        // they are relative scale so multiply by proto size
        xmin = CLAMP(instance.xmin * proto_width, 0, proto_width);
        xmax = CLAMP(instance.xmax * proto_width, 0, proto_width);
        ymin = CLAMP(instance.ymin * proto_height, 0, proto_height);
        ymax = CLAMP(instance.ymax * proto_height, 0, proto_height);

        // Crop a view of the proto layer of just this instance's detection area
        auto cropped_proto = xt::view(proto, xt::range(ymin, ymax), xt::range(xmin, xmax));

        // Gather the instance mask coefficients which we stored as "segmentation_data"
        auto coeffs_struct = instance.segmentation_data;                                                 // First get the coefficients gststructure
        const GValue *coefficients_gvalue = gst_structure_get_value(coeffs_struct, "mask_coefficients"); // Get the g value from the gststructure
        GVariant *coefficients_variant = g_value_get_variant(coefficients_gvalue);                       // From the g value extract the variant
        // From the variant extract the uint8 array
        uint8_t *coefficients_raw = (guint8 *)g_variant_get_fixed_array(coefficients_variant, &coefficients_size, 1);

        // Adapt the variant array into an xtensor array
        size_t size = coefficients_size / 4;   // There 4 bytes per element, so divide by 4
        std::vector<std::size_t> shape = {32}; // The coefficients are 1D
        xt::xarray<float, xt::layout_type::row_major> coefficients = xt::adapt((float *)coefficients_raw, size, xt::no_ownership(), shape);

        // Calculate a matrix multiplication of the instance's coefficients and the cropped proto layer
        auto cropped_mask = dot_product_axis_2(cropped_proto, coefficients);

        // Calculate the sigmoid of the mask
        common::sigmoid(cropped_mask.data(), cropped_mask.size());

        // Add the mask to the object meta
        auto mask_data = cropped_mask.data(); // Get the internal pointer to where the data is stored
        common::copy_data_to_structure(instance.segmentation_data, "mask", (void *)mask_data, cropped_mask.size() * 4);
        common::copy_int_to_structure(instance.segmentation_data, "mask_height", cropped_mask.shape(0));
        common::copy_int_to_structure(instance.segmentation_data, "mask_width", cropped_mask.shape(1));
    }
}

xt::xarray<float> decode_boxes(const xt::xarray<float> &locations, const xt::xarray<float> &priors)
{
    /*
    Decode predicted bbox coordinates using the same scheme
    employed by Yolov2: https://arxiv.org/pdf/1612.08242.pdf

      b_x = (sigmoid(pred_x) - .5) / conv_w + prior_x
      b_y = (sigmoid(pred_y) - .5) / conv_h + prior_y
      b_w = prior_w * exp(loc_w)
      b_h = prior_h * exp(loc_h)

    Note that loc is inputed as [(s(x)-.5)/conv_w, (s(y)-.5)/conv_h, w, h]
    while priors are inputed as [x, y, w, h] where each coordinate
    is relative to size of the image (even sigmoid(x)). We do this
    in the network by dividing by the 'cell size', which is just
    the size of the convouts.

    Also note that prior_x and prior_y are center coordinates which
    is why we have to subtract .5 from sigmoid(pred_x and pred_y).

    Args:
      - loc:    The predicted bounding boxes of size [num_priors, 4]
      - priors: The priorbox coords with size [num_priors, 4]

    Returns: A tensor of decoded relative coordinates in point form
             form with size [num_priors, 4]

    **NOTE: priors = anchors
    */
    xt::xarray<float> variances = {0.1, 0.2};
    // Calculate the x and y
    auto boxes_xy = xt::view(priors, xt::all(), xt::range(_, 2)) + (xt::view(locations, xt::all(), xt::range(_, 2)) * variances(0) * xt::view(priors, xt::all(), xt::range(2, _)));
    // Calculate the width and height
    auto boxes_wh = xt::view(priors, xt::all(), xt::range(2, _)) * xt::exp(xt::view(locations, xt::all(), xt::range(2, _)) * variances(1));

    // Concatenate the parameters together
    auto boxes = xt::concatenate(xt::xtuple(boxes_xy - (boxes_wh / 2),
                                            boxes_wh),
                                 1);
    return boxes;
}

void detect_instances(std::vector<DetectionObject> &objects,
                      auto &all_anchors,
                      auto &all_scores,
                      auto &all_boxes,
                      auto &all_masks,
                      const gint image_size,
                      const float score_threshold)
{
    // First get the max score of each row
    xt::xarray<float, xt::layout_type::row_major> max_scores = amax_row(all_scores.data(), all_scores.size(), all_scores.shape(0), all_scores.shape(1));

    // Filter out any detection below threshold
    std::vector<int> indices_above_threshold = filter(max_scores.data(), max_scores.size(), score_threshold);

    // Keep only the indices above thresholds from our arrays
    auto scores = xt::view(all_scores, xt::keep(indices_above_threshold), xt::drop(0));
    auto boxes = xt::view(all_boxes, xt::keep(indices_above_threshold), xt::all());
    auto masks = xt::view(all_masks, xt::keep(indices_above_threshold), xt::all());
    auto anchors = xt::view(all_anchors, xt::keep(indices_above_threshold), xt::all());

    // Decode the detection boxes
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
        confidence = scores(index, class_index - 1); // Decrement class_index since scores excludes class 0 (background)

        // Create the detection object
        DetectionObject detected_instance = DetectionObject(xmin, ymin, h, w, confidence, common::yolact_twenty[class_index], class_index);

        // We want to append the mask coefficients of each detection as a gst_structure
        xt::xarray<float> mask_coefficients = xt::view(tanned_masks, xt::keep(index), xt::all()); // We are only interested in the coefficients of this instance
        auto mask_data = mask_coefficients.data();                                                // Get the internal pointer to where the data is stored
        common::copy_data_to_structure(detected_instance.segmentation_data, "mask_coefficients", (void *)mask_data, mask_coefficients.size() * 4);

        // Push the detected instance into our objects vector
        objects.emplace_back(std::move(detected_instance));
    }
}

//******************************************************************
// INSTANCE SEGMENTATION POSTPROCESS
//******************************************************************
std::vector<DetectionObject> instance_segmentation_post(std::map<std::string, HailoTensorPtr> tensors,
                                                        const xt::xarray<float> &anchors,
                                                        const gint num_classes,
                                                        const gint image_size,
                                                        const float score_threshold,
                                                        const float nms_threshold)
{
    std::vector<DetectionObject> objects; // The detection meta we will eventually return

    //-------------------------------
    // TENSOR GATHERING
    //-------------------------------
    // Proto layer
    xt::xarray<float, xt::layout_type::row_major> proto = common::dequantize(common::xtensor_from_HailoTensor(tensors[PROTO_LAYER]), tensors[PROTO_LAYER]->qp_scale, tensors[PROTO_LAYER]->qp_zp);
    // Set 0
    auto bbox_0 = common::dequantize(common::xtensor_from_HailoTensor(tensors[BBOX_0]), tensors[BBOX_0]->qp_scale, tensors[BBOX_0]->qp_zp);
    auto mask_0 = common::dequantize(common::xtensor_from_HailoTensor(tensors[MASK_0]), tensors[MASK_0]->qp_scale, tensors[MASK_0]->qp_zp);
    auto conf_0 = common::dequantize(common::xtensor_from_HailoTensor(tensors[CONF_0]), tensors[CONF_0]->qp_scale, tensors[CONF_0]->qp_zp);
    // Set 1
    auto bbox_1 = common::dequantize(common::xtensor_from_HailoTensor(tensors[BBOX_1]), tensors[BBOX_1]->qp_scale, tensors[BBOX_1]->qp_zp);
    auto mask_1 = common::dequantize(common::xtensor_from_HailoTensor(tensors[MASK_1]), tensors[MASK_1]->qp_scale, tensors[MASK_1]->qp_zp);
    auto conf_1 = common::dequantize(common::xtensor_from_HailoTensor(tensors[CONF_1]), tensors[CONF_1]->qp_scale, tensors[CONF_1]->qp_zp);
    // Set 2
    auto bbox_2 = common::dequantize(common::xtensor_from_HailoTensor(tensors[BBOX_2]), tensors[BBOX_2]->qp_scale, tensors[BBOX_2]->qp_zp);
    auto mask_2 = common::dequantize(common::xtensor_from_HailoTensor(tensors[MASK_2]), tensors[MASK_2]->qp_scale, tensors[MASK_2]->qp_zp);
    auto conf_2 = common::dequantize(common::xtensor_from_HailoTensor(tensors[CONF_2]), tensors[CONF_2]->qp_scale, tensors[CONF_2]->qp_zp);
    // Set 3
    auto bbox_3 = common::dequantize(common::xtensor_from_HailoTensor(tensors[BBOX_3]), tensors[BBOX_3]->qp_scale, tensors[BBOX_3]->qp_zp);
    auto mask_3 = common::dequantize(common::xtensor_from_HailoTensor(tensors[MASK_3]), tensors[MASK_3]->qp_scale, tensors[MASK_3]->qp_zp);
    auto conf_3 = common::dequantize(common::xtensor_from_HailoTensor(tensors[CONF_3]), tensors[CONF_3]->qp_scale, tensors[CONF_3]->qp_zp);
    // Set 4
    auto bbox_4 = common::dequantize(common::xtensor_from_HailoTensor(tensors[BBOX_4]), tensors[BBOX_4]->qp_scale, tensors[BBOX_4]->qp_zp);
    auto mask_4 = common::dequantize(common::xtensor_from_HailoTensor(tensors[MASK_4]), tensors[MASK_4]->qp_scale, tensors[MASK_4]->qp_zp);
    auto conf_4 = common::dequantize(common::xtensor_from_HailoTensor(tensors[CONF_4]), tensors[CONF_4]->qp_scale, tensors[CONF_4]->qp_zp);

    //-------------------------------
    // FEATURE STACKING
    //-------------------------------
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

    //-------------------------------
    // INSTANCE DETECTION
    //-------------------------------
    detect_instances(objects, anchors, confidences, all_boxes, all_masks, image_size, score_threshold);

    //-------------------------------
    // NMS
    //-------------------------------
    common::nms(objects, nms_threshold);

    //-------------------------------
    // MASK DECODING
    //-------------------------------
    decode_masks(objects, proto);

    // Return the objects
    return objects;
}

//******************************************************************
//  YOLACT POSTPROCESS
//******************************************************************
void yolact(HailoFramePtr hailo_frame)
{
    // Network specific parameters
    const gint image_size = hailo_frame->width;
    const gint num_classes = 20;

    // Calculate the anchors based on the image size.
    const xt::xarray<float> anchors = get_anchors(image_size);

    // Get the output layers from the hailo frame.
    auto tensors = hailo_frame->get_tensors_by_name();

    // Extract the detection objects using the given parameters.
    std::vector<DetectionObject> detections = instance_segmentation_post(tensors, anchors, num_classes, image_size,
                                                                         SCORE_THRESHOLD, NMS_THRESHOLD);

    // Update the frame with the found detections.
    common::update_frame(hailo_frame, detections);
}

//******************************************************************
//  DEFAULT FILTER
//******************************************************************
void filter(HailoFramePtr hailo_frame)
{
    yolact(hailo_frame);
}