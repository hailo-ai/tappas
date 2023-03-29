
/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)

**/

#include <cmath>
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>

#include "centerpose.hpp"
#include "hailo_xtensor.hpp"
#include "common/tensors.hpp"
#include "common/math.hpp"
#include "common/nms.hpp"

#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"
#include "xtensor/xcontainer.hpp"
#include "xtensor/xeval.hpp"
#include "xtensor/xtensor.hpp"
#include "xtensor/xindex_view.hpp"
#include "xtensor/xio.hpp"
#include "xtensor/xmanipulation.hpp"
#include "xtensor/xmasked_view.hpp"
#include "xtensor/xoperation.hpp"
#include "xtensor/xpad.hpp"
#include "xtensor/xrandom.hpp"
#include "xtensor/xshape.hpp"
#include "xtensor/xsort.hpp"
#include "xtensor/xstrided_view.hpp"
#include "xtensor/xview.hpp"

//******************************************************************
// CENTERPOSE NETWORK SPECIFIC PARAMETERS
//******************************************************************
#define TOP_K 20
#define SCORE_THRESHOLD 0.5
#define IOU_THRESHOLD 0.45

// Output layer names for CenterposeRegnetx_16gf network
const char *CENTER_HEATMAP_OUTPUT_LAYER = "center_nms/ew_add2";
const char *CENTER_WIDTH_HEIGHT_OUTPUT_LAYER = "centerpose_regnetx_1_6gf_fpn/conv76";
const char *CENTER_OFFSET_OUTPUT_LAYER = "centerpose_regnetx_1_6gf_fpn/conv78";
const char *JOINT_HEATMAP_OUTPUT_LAYER = "joint_nms/ew_add2";
const char *JOINT_OFFSET_OUTPUT_LAYER = "centerpose_regnetx_1_6gf_fpn/conv80";
const char *JOINT_CENTER_OFFSET_OUTPUT_LAYER = "centerpose_regnetx_1_6gf_fpn/conv77";

const std::vector<std::pair<int, int>> centerpose_joint_pairs =
    {
        {0, 1}, {1, 3}, {0, 2}, {2, 4}, {5, 6}, {5, 7}, {7, 9}, {6, 8}, {8, 10}, {5, 11}, {6, 12}, {11, 12}, {11, 13}, {12, 14}, {13, 15}, {14, 16}};

/**
 * @brief This function maps column-wise array indices to true flattened indices.
 *
 * @param array_to_map  the array to to map
 * @param row_wise_indices  indices in row-wise method
 * @return xt::xarray<int> flattened indices
 */
xt::xarray<int> map_to_flat_indices(const xt::xarray<int> &array_to_map, const xt::xarray<int> &row_wise_indices)
{
    /*
          Example:
          test = {{6, 2, 8, 0, 9, 3, 5, 4, 7, 1},
                 {16,19,11,17,12,13,15,14,10,18}};
          ind  = {{0, 1, 2},
                  {9, 8, 7}}
          This function would convert ind to: {{0,  1,  2},
                                              {19, 18, 17}}
          So that when keeping by flattened arrays, you get:
          xt::view(xt::flatten(test), xt::keep(xt::flatten(ind))) = {6, 2, 8, 18, 10, 14}
      */
    int num_rows = array_to_map.shape(0);
    int cols_per_row = array_to_map.shape(1);
    int index_cols = row_wise_indices.shape(1);
    // Prepare an incrementing array that steps from 0 --> number of rows
    xt::xarray<int> pre_tile = xt::arange<int>(0, num_rows).reshape({num_rows, 1});
    // Tile the array to number of columns in the index array, then multiple by the number of columns.
    // This way we increment each row by row# * num_columns
    xt::xarray<int> row_inds = xt::tile(pre_tile, {1, index_cols}) * cols_per_row + row_wise_indices;

    // Return a flattened view
    return std::move(xt::flatten(row_inds));
}

/**
 * @brief Top K function
 *
 * @param array array
 * @param k take k best results
 * @return xt::xarray<int>
 */
xt::xarray<int> nd_topk(xt::xarray<float> &array, const int k)
{
    // Since top_k partitions on a flattened array (xt::xnone()),
    // for arrays of more than 1 row we must topk for each row.
    int rows = array.shape(0);
    int cols = array.shape(1);
    xt::xarray<int> indices = xt::empty<int>({rows, k}); // prepare an empty array to fill
    for (int index = 0; index < rows; index++)
    {
        // For each row, get the topk of the row, and add cols*index to get true flat indices
        xt::xarray<uint8_t> expanded_row = xt::expand_dims(xt::row(array, index), 0);
        xt::row(indices, index) = xt::flatten(common::top_k(expanded_row, k)) + (cols * index);
    }
    return indices;
}

/**
 * @brief get top k centers
 *
 * @param scores output tensors of scores
 * @param k take k best scores and ignore the others
 * @return std::pair<xt::xarray<int>, xt::xarray<uint8_t>> pair of indices of scores and scores
 */
std::pair<xt::xarray<int>, xt::xarray<uint8_t>> top_k_centers(HailoTensorPtr scores, const int k)
{
    // Adapt the tensor into an xarray of proper shape and size
    auto xscores = common::get_xtensor(scores);

    // Get the indices of the top k scoring cells
    int size = scores->size();
    xt::xarray<uint8_t> xscores_view = xt::reshape_view(xscores, {1, size});
    xt::xarray<int> topk_score_indices = common::top_k(xscores_view, k);
    topk_score_indices = xt::flatten(topk_score_indices);

    // We want a flattened view of the scores to sort by
    auto flat_scores = xt::flatten(xscores);

    // Using the top k indices, get the top k scores
    auto topk_scores = xt::view(flat_scores, xt::keep(topk_score_indices));

    // Return the top scores and their indices
    return std::pair<xt::xarray<int>, xt::xarray<uint8_t>>(std::move(topk_score_indices), std::move(topk_scores));
}

/**
 * @brief get top k joints
 *
 * @param joint_scores output tensors of scores
 * @param k take k best scores and ignore the others
 * @return std::pair<xt::xarray<int>, xt::xarray<uint8_t>> pair of indices of scores and scores
 */
std::pair<xt::xarray<int>, xt::xarray<uint8_t>> top_k_joints(HailoTensorPtr joint_scores, const int k)
{
    // Adapt the tensor into an xarray of proper shape and size
    auto xjoint_scores = common::get_xtensor(joint_scores);
    // Transpose the joints so that we lead by joint class {17, 160, 160} instead of {160, 160, 17}
    auto transposed_scores = xt::transpose(xjoint_scores, {2, 0, 1});
    // Create a reshape view that we can sort by {17, 160, 160} --> {17, 25600}
    xt::xarray<float> scores_to_sort = xt::reshape_view(transposed_scores,
                                                        {joint_scores->features(), joint_scores->width() * joint_scores->height()});

    // Get the indices of the top k scoring cells
    auto topk_score_indices = nd_topk(scores_to_sort, k);
    auto topk_score_indices_flat = xt::flatten(topk_score_indices);

    // Using the top k indices, get the top k scores
    // We use a reshaped view of the scores from {17, 25600} to {435200} so that when we take using topk_score_indices,
    // we take {340} (17 * 20) instead of {17, 340}. We return a reshaped view that reformats back to proper {17, 20}.
    auto scores_to_sort_flat = xt::flatten(scores_to_sort);
    auto topk_scores_flat = xt::view(scores_to_sort_flat, xt::keep(topk_score_indices_flat));
    auto topk_scores = xt::reshape_view(topk_scores_flat, {(int)joint_scores->features(), k});

    // Return the top scores and their indices
    return std::pair<xt::xarray<int>, xt::xarray<uint8_t>>(std::move(topk_score_indices), std::move(topk_scores));
}

/**
 * @brief Extract features from tensor
 *
 * @param tensor output tensor
 * @param indices indices to keep
 * @return xt::xarray<uint8_t> features
 */
xt::xarray<uint8_t> gather_features_from_tensor(HailoTensorPtr tensor, xt::xarray<int> &indices)
{
    // Adapt the tensor into an xarray of proper shape and size:
    auto xtensor = common::get_xtensor(tensor);

    // Extract the top k keypoints using the given indices:
    // Use a reshaped view of the given tensor so that features can be gathered
    auto transposed_tensor = xt::reshape_view(xtensor, {tensor->width() * tensor->height(), tensor->features()});

    // Gather the features using the given indices
    auto features = xt::view(transposed_tensor, xt::keep(indices), xt::all());

    return features;
}

/**
 * @brief Box and keypoint extraction
 *
 * @param scores array of scores
 * @param center_offsets array of center offsets
 * @param center_wh centernet boxes width height
 * @param cell_x_indices array of indices x of the cells
 * @param cell_y_indices array of indices y of the cells
 * @param score_threshold threshold for score filtering
 * @param image_size image width/height (the netowork input tensor is square)
 * @return xt::xarray<float>
 */
xt::xarray<float> build_boxes_centerpose(xt::xarray<float> &scores,
                                         xt::xarray<float> &center_offsets,
                                         xt::xarray<float> &center_wh,
                                         xt::xarray<int> &cell_x_indices,
                                         xt::xarray<int> &cell_y_indices,
                                         const float score_threshold,
                                         const int image_size)
{
    // Here we need to calculate the min and max of the box. The cell index + offset gives the real
    // center of the box, then subtracting half of the width/height will get the xmin/ymin.
    xt::xarray<float> xmin = (cell_x_indices + xt::col(center_offsets, 0) - (xt::col(center_wh, 0) * 0.5));
    xt::xarray<float> ymin = (cell_y_indices + xt::col(center_offsets, 1) - (xt::col(center_wh, 1) * 0.5));
    xt::xarray<float> xmax = xmin + xt::col(center_wh, 0);
    xt::xarray<float> ymax = ymin + xt::col(center_wh, 1);

    // Stack the box parameters along axis 1, ending with an array of shape { k, 4 }
    xt::xarray<float> detection_boxes = xt::stack(xt::xtuple(xmin, ymin, xmax, ymax), 1);
    return detection_boxes;
}

/**
 * @brief Detection/landmarks encoding
 *
 * @param objects vector of the detected instances
 * @param scores scores array
 * @param detection_boxes boxes detected
 * @param center_wh centernet boxes width height
 * @param keypoints keypoints
 * @param score_threshold threshold for score filtering
 * @param max_detections max number of best results
 * @param image_size image width/height (the netowork input tensor is square)
 */
void encode_boxes_centerpose(std::vector<HailoDetection> &objects,
                             xt::xarray<float> &scores,
                             xt::xarray<float> &detection_boxes,
                             xt::xarray<float> &center_wh,
                             xt::xarray<float> &keypoints,
                             const float score_threshold,
                             const int max_detections,
                             const int image_size)
{
    // The detection meta will hold the following items:
    float confidence, w, h, xmin, ymin = 0.0f;
    std::string label = "person";
    xt::xarray<float> wh_scaled = center_wh / image_size;
    // Iterate over our top k results
    for (int index = 0; index < max_detections; index++)
    {
        confidence = scores(index);
        // If the confidence is below our threshold, then skip it
        if (confidence < score_threshold)
            continue;

        w = wh_scaled(index, 0); // Box width, relative to image size
        h = wh_scaled(index, 1); // Box height, relative to image size
        // The xmin and ymin we can take form the detection_boxes
        xmin = detection_boxes(index, 0) / image_size;
        ymin = detection_boxes(index, 1) / image_size;

        // Once all parameters are calculated, push them into the meta
        // Class = 1 since centerpose only detects people
        HailoDetection detected_pose(HailoBBox(xmin, ymin, w, h), -1, label, confidence);
        // We want to package the keypoints with their detection box, so extract the ones for this index and make them relative to the image size
        xt::xarray<float> joint_keypoints = xt::view(keypoints, xt::keep(index), xt::all(), xt::all());

        // Change the scale of the keypoints to be non relative to the frame size
        // Each grid cell is 4x4 pixels large - means that the real frame size is output layer size multiply by 4
        xt::xarray<float> scaled_keypoints = xt::zeros<float>(joint_keypoints.shape());
        xt::view(scaled_keypoints, xt::all(), xt::all(), 0) = xt::view(joint_keypoints, xt::all(), xt::all(), 0) / (image_size * 4);
        xt::view(scaled_keypoints, xt::all(), xt::all(), 1) = xt::view(joint_keypoints, xt::all(), xt::all(), 1) / (image_size * 4);
        xt::view(scaled_keypoints, xt::all(), xt::all(), 2) = xt::view(joint_keypoints, xt::all(), xt::all(), 2);

        hailo_common::add_landmarks_to_detection(detected_pose, "centerpose", xt::squeeze(scaled_keypoints), score_threshold, centerpose_joint_pairs);

        objects.emplace_back(std::move(detected_pose)); // Push the detection to the objects vector
    }
}

/**
 * @brief centerpose post process
 *
 * @param roi region of interest
 * @param k take only best k results
 * @param score_threshold threshold for score filterig
 * @param iou_thr threshold for IOU calculation in NMS
 * @return std::vector<HailoDetection> the detected objects
 */
std::vector<HailoDetection> centerpose_postprocess(HailoROIPtr roi,
                                                      const int k,
                                                      const float score_threshold,
                                                      const float iou_thr)
{
    std::vector<HailoDetection> objects; // The detection meta we will eventually return
    // Extract the 6 output tensors:
    // Center heatmap tensor with scaling and offset tensors for person detection
    HailoTensorPtr center_heatmap = roi->get_tensor(CENTER_HEATMAP_OUTPUT_LAYER);
    HailoTensorPtr center_width_height = roi->get_tensor(CENTER_WIDTH_HEIGHT_OUTPUT_LAYER);
    HailoTensorPtr center_offset = roi->get_tensor(CENTER_OFFSET_OUTPUT_LAYER);
    // Joint heatmap and offset tensors for joint detection
    HailoTensorPtr joint_heatmap = roi->get_tensor(JOINT_HEATMAP_OUTPUT_LAYER);
    HailoTensorPtr joint_offset = roi->get_tensor(JOINT_OFFSET_OUTPUT_LAYER);
    // Joint center offset tensor for secondary joint detection
    HailoTensorPtr joint_center_offset = roi->get_tensor(JOINT_CENTER_OFFSET_OUTPUT_LAYER);

    // detection box encoding
    // From the center_heatmap tensor, we want to extract the top k centers with the highest score
    auto top_scores = top_k_centers(center_heatmap, k);                                  // Returns both the top scores and their indices
    xt::xarray<int> topk_score_indices = top_scores.first;                               // Separate out the top score indices
    xt::xarray<uint8_t> topk_scores = top_scores.second;                                 // Separate out the top scores
    xt::xarray<int> topk_scores_y_index = topk_score_indices / center_heatmap->height(); // Find the y index of the cells
    xt::xarray<int> topk_scores_x_index = topk_score_indices % center_heatmap->width();  // Find the x index of the cells

    // With the top k indices in hand, we can now extract the corresponding center offsets and widths/heights
    auto topk_center_offset = gather_features_from_tensor(center_offset, topk_score_indices);   // Use the top k indices from earlier
    auto topk_center_wh = gather_features_from_tensor(center_width_height, topk_score_indices); // Use the top k indices from earlier

    // Now that we have our top k features, we can rescale them to dequantize
    xt::xarray<float> topk_scores_rescaled = common::dequantize(topk_scores,
                                                                center_heatmap->vstream_info().quant_info.qp_scale, center_heatmap->vstream_info().quant_info.qp_zp);
    xt::xarray<float> topk_center_offset_rescaled = common::dequantize(topk_center_offset,
                                                                       center_offset->vstream_info().quant_info.qp_scale, center_offset->vstream_info().quant_info.qp_zp);

    xt::xarray<float> topk_center_wh_rescaled = common::dequantize(topk_center_wh,
                                                                   center_width_height->vstream_info().quant_info.qp_scale, center_width_height->vstream_info().quant_info.qp_zp);

    const int image_size = center_heatmap->width(); // We want the boxes to be of relative size to the original image
    // Build up the detection boxes
    auto bboxes = build_boxes_centerpose(topk_scores_rescaled,
                                         topk_center_offset_rescaled,
                                         topk_center_wh_rescaled,
                                         topk_scores_x_index, topk_scores_y_index, score_threshold, image_size);

    // Joinf keypoint decoding

    // From the joint_center_offset tensor, we want to extract the top k keypoints
    const int num_joints = joint_center_offset->features() / 2;                                 // Get the number of joints
    auto topk_keypoints = gather_features_from_tensor(joint_center_offset, topk_score_indices); // Use the top k indices from earlier

    // From the joint_heatmap tensor, we want to extract the top k joints with the highest score
    auto top_k_joint_heatmap = top_k_joints(joint_heatmap, k);                  // Returns both the top scores and their indices
    xt::xarray<int> topk_joint_heatmap_indices = top_k_joint_heatmap.first;     // Separate out the top score indices
    xt::xarray<uint8_t> topk_joint_heatmap_scores = top_k_joint_heatmap.second; // Separate out the top scores
    // The indices are in respect to an array of shape {435200}, so we will need to calculate the proper (x,y)
    topk_joint_heatmap_indices = topk_joint_heatmap_indices % (joint_heatmap->width() * joint_heatmap->height());
    xt::xarray<int> topk_joints_y_index = topk_joint_heatmap_indices / joint_heatmap->width(); // Find the y index of the cells
    xt::xarray<int> topk_joints_x_index = topk_joint_heatmap_indices % joint_heatmap->width(); // Find the x index of the cells

    // With the top k joint indices in hand, we can now extract the corresponding joint center offsets
    // Use the top k joint indices from earlier, use a 1 dimensional reshaped view though so we get all 340 offsets
    // then reshape view back to {17, 20, 2}
    xt::xarray<int> flattened_joint_indices = xt::flatten(topk_joint_heatmap_indices);
    xt::xarray<uint8_t> topk_joint_offset = xt::reshape_view(gather_features_from_tensor(joint_offset, flattened_joint_indices),
                                                             {num_joints, k, 2});

    // Now that we have our top k joints, we can rescale them to dequantize
    xt::xarray<float> topk_joint_score_rescaled = common::dequantize(topk_joint_heatmap_scores,
                                                                     joint_heatmap->vstream_info().quant_info.qp_scale, joint_heatmap->vstream_info().quant_info.qp_zp);
    xt::xarray<float> topk_keypoints_rescaled = common::dequantize(topk_keypoints,
                                                                   joint_center_offset->vstream_info().quant_info.qp_scale, joint_center_offset->vstream_info().quant_info.qp_zp);

    // These are just the offsets within the 160x160 grid, we still need to add the indices
    // Current shape of topk_keypoints_rescaled is { 20, 34 }, so we need to reshape to --> { 20, 17, 2 }
    topk_keypoints_rescaled = xt::reshape_view(topk_keypoints_rescaled, {k, num_joints, 2});      // Reshape from {20, 34}
    auto topk_keypoints_x = xt::view(topk_keypoints_rescaled, xt::all(), xt::all(), xt::keep(0)); // Extract the x offsets
    auto topk_keypoints_y = xt::view(topk_keypoints_rescaled, xt::all(), xt::all(), xt::keep(1)); // Extract the y offsets
    // Add the x and y indices, reshape and tile so they are in shape --> {20, 17, 1}
    topk_keypoints_x += xt::tile(xt::reshape_view(topk_scores_x_index, {k, 1, 1}), {1, num_joints, 1});
    topk_keypoints_y += xt::tile(xt::reshape_view(topk_scores_y_index, {k, 1, 1}), {1, num_joints, 1});
    xt::xarray<float> keypoints = xt::stack(xt::xtuple(topk_keypoints_x, topk_keypoints_y), 2); // Stack x and y together
    // Stacking adds a new dim, so reshape to { 20, 17, 2 }
    keypoints = xt::reshape_view(keypoints, {k, num_joints, 2});

    // We need to prepare the center_offset_keypoints into this shape as well
    // Reshape the mask from {num_joints, k} --> {num_joints, k, 1} so that we can stack it later
    topk_joint_score_rescaled = xt::reshape_view(topk_joint_score_rescaled, {num_joints, k, 1});
    auto transposed_joint_scores = xt::transpose(topk_joint_score_rescaled, {1, 0, 2});

    // As of now the keypoints are still in 160x160 grid space. Now that we have chosen the right keypoints,
    // we want to transform them into the original image dimensions. We do this through an affine transformation.
    // Luckily, since the image sizes are always constant (centerpose network dimensions to 160x160 grid space),
    // our affine vector is the same every time. Namely: {{4.0, 0.0, 0.0},
    //                                                    {0.0, 4.0, 0.0}}
    // This means we do not need to recalculate the vector every time, and can even skip the dot product
    // in favor of faster scalar multiplication.
    keypoints *= 4.0;

    xt::xarray<float> keypoints_with_scores = xt::concatenate(xt::xtuple(keypoints, transposed_joint_scores), 2);

    //-------------------------------
    // RESULTS ENCODING
    //-------------------------------

    // Encode the individual boxes/keypoints and package them into the meta
    encode_boxes_centerpose(objects,
                            topk_scores_rescaled,
                            bboxes,
                            topk_center_wh_rescaled,
                            keypoints_with_scores,
                            score_threshold, k, image_size);

    // Perform nms to throw out similar detections
    common::nms(objects, iou_thr);

    return objects;
}

/**
 * @brief Perform post process and add the detected objects to the roi object
 *
 * @param roi region of interest
 */
void centerpose(HailoROIPtr roi)
{
    if (roi->has_tensors())
    {
        auto detections = centerpose_postprocess(roi, TOP_K, SCORE_THRESHOLD, IOU_THRESHOLD);

        // Update the roi with the found detections.
        hailo_common::add_detections(roi, detections);
    }
}

/**
 * @brief A function for centerpose_416 network, layer names may change from the default
 *
 * @param roi region of interest
 */
void centerpose_416(HailoROIPtr roi)
{
    CENTER_HEATMAP_OUTPUT_LAYER = "center_nms/ew_add1";
    CENTER_WIDTH_HEIGHT_OUTPUT_LAYER = "centerpose_repvgg_a0/conv37";
    CENTER_OFFSET_OUTPUT_LAYER = "centerpose_repvgg_a0/conv39";
    JOINT_HEATMAP_OUTPUT_LAYER = "joint_nms/ew_add1";
    JOINT_OFFSET_OUTPUT_LAYER = "centerpose_repvgg_a0/conv41";
    JOINT_CENTER_OFFSET_OUTPUT_LAYER = "centerpose_repvgg_a0/conv38";
    centerpose(roi);
}

/**
 * @brief A function for merged networks, layer names may change from the default
 *
 * @param roi region of interest
 */
void centerpose_merged(HailoROIPtr roi)
{
    CENTER_HEATMAP_OUTPUT_LAYER = "center_nms/ew_add1";
    CENTER_WIDTH_HEIGHT_OUTPUT_LAYER = "centerpose_repvgg_a0_no_alls/conv37";
    CENTER_OFFSET_OUTPUT_LAYER = "centerpose_repvgg_a0_no_alls/conv39";
    JOINT_HEATMAP_OUTPUT_LAYER = "joint_nms/ew_add1";
    JOINT_OFFSET_OUTPUT_LAYER = "centerpose_repvgg_a0_no_alls/conv41";
    JOINT_CENTER_OFFSET_OUTPUT_LAYER = "centerpose_repvgg_a0_no_alls/conv38";
    centerpose(roi);
}

void filter(HailoROIPtr roi)
{
    centerpose(roi);
}
