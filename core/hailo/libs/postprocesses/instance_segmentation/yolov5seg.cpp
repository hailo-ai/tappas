#include "yolov5seg.hpp"
#include "xtensor/xsort.hpp"
#include "xtensor/xpad.hpp"
#include "hailo_common.hpp"
#include "common/tensors.hpp"
#include "common/nms.hpp"
#include "common/labels/coco_eighty.hpp"
#include "mask_decoding.hpp"
#include <thread>
#include <future>
#include <iterator>

// the net returns 32 values representing the mask coefficients, and 4 values representing the box coordinates
#define MASK_CO 32
#define BOX_CO 4

/*
 * @brief Creates the grid and the anchor grid that will be used for each decoding
 *
 * @param anchors xarray, initialized in creation of Yolov5segParams
 * @param stride
 * @param nx shape[0] of the branch
 * @param ny shape[1] of the branch
 * @param num_anchors is the number of anchors per branch / 2
 */
std::tuple<xt::xarray<float>, xt::xarray<float>> make_grid(xt::xarray<float> &anchors, const int stride, const int nx, const int ny, const int num_anchors)
{
    xt::xarray<int> x = xt::arange(nx);
    xt::xarray<int> y = xt::arange(ny);
    auto mesh = xt::meshgrid(y, x);
    auto yv = std::get<0>(mesh);
    auto xv = std::get<1>(mesh);
    // making grid
    auto stack = xt::stack(xt::xtuple(xv, yv), 2);
    stack.reshape({1, ny, nx, 2});
    auto grid = xt::broadcast(stack, {num_anchors, ny, nx, 2}) - 0.5;
    // making anchor grid
    anchors *= stride;
    anchors.reshape({num_anchors, 1, 1, 2});
    auto anchor_grid = xt::broadcast(anchors, {num_anchors, ny, nx, 2});
    return std::tuple<xt::xarray<float>, xt::xarray<float>>(std::move(grid), std::move(anchor_grid));
}

/*
 * @brief Returns a vector of indices, of detections that have: is_object * max(class_confidence) > score_threshold
 *
 * @param all_scores an xview with the confidence scores for each class, for each detection
 * @param all_is_object an xview with the confidence that this detection is an object, for each detection
 * @param score_threshold float
 */
std::vector<int> filter_above_threshold(auto &all_scores, auto &all_is_object, const float score_threshold)
{
    std::vector<int> indices;
    int this_index;
    float conf;
    for (uint i = 0; i < all_is_object.size(); i++)
    {
        // first check if the object parameter is bigger than threshold
        if (all_is_object(i, 0) > score_threshold)
        {
            this_index = xt::argmax(xt::row(all_scores, i))(0) + 1;
            conf = all_scores(i, this_index - 1);
            // now check if confidence of the class with highest score * object is bigger than threshold
            if (conf > score_threshold)
            {
                indices.emplace_back(i);
            }
        }
    }
    return indices;
}

/*
 * @brief Gets the xviews of the decoded and filtered results, and adds them to a vector of HailoDetections
 *
 * @param size int representing amount of detections
 * @param boxes xview of (x, y, w, h), x and y are the center of the box
 * @param is_object an xview with the confidence that this detection is an object, for each detection
 * @param scores an xview with the confidence scores for each class, for each detection
 * @param masks an xview with 32 coefficients representing a mask per detection
 * @param objects a vecor of HailoDetections, to which the detections will be added
 *  */
void create_hailo_detections(const uint size, auto &boxes, auto &is_object, auto &scores, auto &masks, std::vector<HailoDetection> &objects)
{
    int class_index;
    float confidence, w, h, x, y = 0.0;
    for (uint index = 0; index < size; index++)
    {
        // Get the box parameters for this box
        x = (boxes(index, 0));
        y = (boxes(index, 1));
        w = (boxes(index, 2));
        h = (boxes(index, 3));
        // x and y represented center of box, so they need to be changed to left bottom corner
        HailoBBox bbox(x - w / 2, y - h / 2, w, h);
        // calculate label and confidence
        class_index = xt::argmax(xt::row(scores, index))(0) + 1;
        std::string label = common::coco_eighty[class_index];
        confidence = scores(index, class_index - 1) * is_object(index, 0); // Decrement class_index since scores excludes class 0 (background)
        // create mask
        xt::xarray<float> mask_coefficients = xt::squeeze(xt::view(masks, xt::keep(index), xt::all()));
        HailoDetection detected_instance(bbox, class_index, label, confidence);
        std::vector<float> data(mask_coefficients.shape(0));
        memcpy(data.data(), mask_coefficients.data(), sizeof(float) * mask_coefficients.shape(0));
        // create the detection itself
        detected_instance.add_object((std::make_shared<HailoMatrix>(data, mask_coefficients.shape(0), 1)));
        objects.push_back(detected_instance);
    }
    return;
}

/*
 * @brief Does the decoding and the filtering for the output, and adds the results to the HailoDetections vector
 *
 *  */
std::vector<HailoDetection> yolov5_decoding(xt::xarray<float> &output, const int stride, xt::xarray<float> &anchors, xt::xarray<float> &grid, xt::xarray<float> &anchor_grid, const int num_anchors, const float score_threshold)
{
    int h = output.shape()[0];
    int w = output.shape()[1];
    int num_classes = (output.shape()[2] / 3) - BOX_CO - 1 - MASK_CO;
    auto reshaped_output = xt::reshape_view(output, {h, w, num_anchors, BOX_CO + 1 + num_classes + MASK_CO});
    auto new_output = xt::transpose(reshaped_output, {2, 0, 1, 3});
    auto xy = xt::view(new_output, xt::all(), xt::all(), xt::all(), xt::range(_, 2));
    auto wh = xt::view(new_output, xt::all(), xt::all(), xt::all(), xt::range(2, 4));
    auto conf = xt::view(new_output, xt::all(), xt::all(), xt::all(), xt::range(4, 4 + num_classes + 1));
    auto mask = xt::view(new_output, xt::all(), xt::all(), xt::all(), xt::range(4 + num_classes + 1, _));
    // decoding
    auto new_xy = (xtensor_sigmoid(xy) * 2 + grid) * stride;
    auto new_wh = xt::square(xtensor_sigmoid(wh) * 2) * anchor_grid;
    auto new_conf = xtensor_sigmoid(conf);
    auto out = xt::concatenate(xt::xtuple(new_xy, new_wh, new_conf, mask), 3);
    // organize as number of detections x 117 (coordinates, is_object, classes confidence, mask)
    auto all_decoded = xt::reshape_view(out, {num_anchors * h * w, BOX_CO + 1 + num_classes + MASK_CO});
    // filter out detections with confidence above threshold
    auto all_is_object = xt::view(all_decoded, xt::all(), xt::range(4, 5));
    auto all_scores = xt::view(all_decoded, xt::all(), xt::range(5, 85));
    std::vector<int> indices = filter_above_threshold(all_scores, all_is_object, score_threshold);
    auto boxes = xt::view(all_decoded, xt::keep(indices), xt::range(_, 4)) / 640;
    auto is_object = xt::view(all_is_object, xt::keep(indices), xt::all());
    auto scores = xt::view(all_scores, xt::keep(indices), xt::all());
    auto masks = xt::view(all_decoded, xt::keep(indices), xt::range(85, _));
    // create HailoDetections for the NMS and the mask decoding
    std::vector<HailoDetection> objects;
    create_hailo_detections(indices.size(), boxes, is_object, scores, masks, objects);
    return objects;
}

/*
 * @brief Does dequantize and decoding for each output seperately
 *
 *  */
std::vector<HailoDetection> post_per_branch(std::string branch_name, const int index, std::map<std::string, HailoTensorPtr> tensors, std::vector<xt::xarray<float>> anchor_list, std::vector<int> stride_list, const float iou_threshold, const float score_threshold, std::vector<xt::xarray<float>> grids, std::vector<xt::xarray<float>> anchor_grids, const int num_anchors)
{
    auto output = common::dequantize(common::get_xtensor_uint16(tensors[branch_name]), tensors[branch_name]->vstream_info().quant_info.qp_scale, tensors[branch_name]->vstream_info().quant_info.qp_zp);
    return yolov5_decoding(output, stride_list[index], anchor_list[index], grids[index], anchor_grids[index], num_anchors, score_threshold);
}

/*
 * @brief Does dequantize and decoding for each output, and then calls nms and decode masks
 *
 *  */
std::vector<HailoDetection> yolov5seg_post(auto &tensors, auto &anchor_list, auto &stride_list, const float iou_threshold, const float score_threshold, auto &grids, auto &anchor_grids, const int num_anchors)
{
    auto proto_tensor = common::dequantize(common::get_xtensor(tensors["yolov5n_seg/conv63"]), tensors["yolov5n_seg/conv63"]->vstream_info().quant_info.qp_scale, tensors["yolov5n_seg/conv63"]->vstream_info().quant_info.qp_zp);

    // run the postprocess for each branch seperately
    std::future<std::vector<HailoDetection>> t2 = std::async(post_per_branch, "yolov5n_seg/conv48", 2, tensors, anchor_list, stride_list, iou_threshold, score_threshold, grids, anchor_grids, num_anchors);
    std::future<std::vector<HailoDetection>> t1 = std::async(post_per_branch, "yolov5n_seg/conv55", 1, tensors, anchor_list, stride_list, iou_threshold, score_threshold, grids, anchor_grids, num_anchors);
    std::future<std::vector<HailoDetection>> t0 = std::async(post_per_branch, "yolov5n_seg/conv61", 0, tensors, anchor_list, stride_list, iou_threshold, score_threshold, grids, anchor_grids, num_anchors);
    std::vector<HailoDetection> d2 = t2.get();
    std::vector<HailoDetection> d1 = t1.get();
    std::vector<HailoDetection> d0 = t0.get();

    // concatenate all detections
    std::vector<HailoDetection> all_detections;
    all_detections.reserve(d0.size() + d1.size() + d2.size());
    all_detections.insert(all_detections.end(), d0.begin(), d0.end());
    all_detections.insert(all_detections.end(), d1.begin(), d1.end());
    all_detections.insert(all_detections.end(), d2.begin(), d2.end());

    common::nms(all_detections, iou_threshold);
    decode_masks(all_detections, proto_tensor);
    return all_detections;
}

Yolov5segParams *init(const std::string config_path, const std::string function_name)
{
    Yolov5segParams *params = new Yolov5segParams();
    std::vector<int> outputs_size = params->outputs_size;
    std::vector<xt::xarray<float>> anchors = params->anchors;
    std::vector<int> strides = params->strides;
    std::vector<xt::xarray<float>> grids;
    std::vector<xt::xarray<float>> anchor_grids;
    int num_anchors = 0;
    // create grid and anchor grid
    for (uint index = 0; index < outputs_size.size(); index++)
    {
        anchors[index] /= strides[index];
        num_anchors = floor(anchors[index].size() / 2);
        auto both_grids = make_grid(anchors[index], strides[index], outputs_size[index], outputs_size[index], num_anchors);
        xt::xarray<float> grid = std::get<0>(both_grids);
        xt::xarray<float> anchor_grid = std::get<1>(both_grids);
        grids.emplace_back(grid);
        anchor_grids.emplace_back(anchor_grid);
    }
    params->grids = grids;
    params->anchor_grids = anchor_grids;
    params->num_anchors = num_anchors;
    return params;
}

void free_resources(void *params_void_ptr)
{
    Yolov5segParams *params = reinterpret_cast<Yolov5segParams *>(params_void_ptr);
    delete params;
}

/**
 * @brief call the post process and add the detections to the roi
 *
 * @param roi the region of interest
 */
void yolov5seg(HailoROIPtr roi, void *params_void_ptr)
{
    Yolov5segParams *params = reinterpret_cast<Yolov5segParams *>(params_void_ptr);
    std::map<std::string, HailoTensorPtr> tensors = roi->get_tensors_by_name();
    std::vector<HailoDetection> detections = yolov5seg_post(tensors, params->anchors, params->strides, params->iou_threshold, params->score_threshold, params->grids, params->anchor_grids, params->num_anchors);
    hailo_common::add_detections(roi, detections);
}

/**
 * @brief default filter function
 *
 * @param roi the region of interest
 */
void filter(HailoROIPtr roi, void *params_void_ptr)
{
    yolov5seg(roi, params_void_ptr);
}