
/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)

**/

#include <vector>

#include "mspn.hpp"
#include "common/tensors.hpp"

// Open source includes
#include <opencv2/opencv.hpp>

#include "xtensor/xarray.hpp"
#include "xtensor/xoperation.hpp"
#include "xtensor/xpad.hpp"
#include "xtensor/xshape.hpp"
#include "xtensor/xsort.hpp"
#include "xtensor/xview.hpp"

// MSPN NETWORK SPECIFIC PARAMETERS
#define SCORE_THRESHOLD 0.2
#define KERNEL_SIZE 5
#define EPS 1e-12

const std::vector<std::pair<int, int>> centerpose_joint_pairs =
    {
        {0, 1}, {1, 3}, {0, 2}, {2, 4}, {5, 6}, {5, 7}, {7, 9}, {6, 8}, {8, 10}, {5, 11}, {6, 12}, {11, 12}, {11, 13}, {12, 14}, {13, 15}, {14, 16}};

/**
 * @brief performs gaussian blur on heatmaps
 *
 * @param heatmaps the tensor containing the heatmaps
 * @param num_joints the number of joints of the skeleton
 * @param width the width of the image
 * @param height the height of the image
 */
void gaussian_blur(xt::xarray<float> &heatmaps, int num_joints, int width, int height)
{
    if (KERNEL_SIZE % 2 == 0)
    {
        throw std::runtime_error("Kernel size must be odd");
    }

    int border = floor((KERNEL_SIZE - 1) / 2);

    int j;
    for (j = 0; j < num_joints; j++)
    {
        xt::xarray<float> sliced_view = xt::view(heatmaps, j, xt::all(), xt::all());
        xt::xarray<float> origin_max = xt::amax(sliced_view);
        cv::Mat dr = cv::Mat::zeros(height + 2 * border, width + 2 * border, CV_32F);
        cv::Mat sliced_view_mat(sliced_view.shape()[0], sliced_view.shape()[1], CV_32F, sliced_view.data(), 0);
        cv::copyMakeBorder(sliced_view_mat, dr, border, border, border, border, cv::BORDER_CONSTANT, 0);
        cv::Mat gaussian_image;
        cv::GaussianBlur(dr, gaussian_image, cv::Size(KERNEL_SIZE, KERNEL_SIZE), 0);
        cv::Mat sliced_view_mat2(sliced_view.shape()[0], sliced_view.shape()[1], CV_32F, sliced_view.data(), 0);
        cv::Mat gaussian_image_roi = gaussian_image(cv::Rect(border, border, width, height));
        gaussian_image_roi.copyTo(sliced_view_mat2);
        float new_max = std::max(xt::amax(sliced_view)(0), (float)EPS);
        xt::xarray<float> sliced_view_mul = sliced_view * (origin_max / new_max);
        sliced_view = sliced_view_mul;
    }
}

/**
 * @brief Get the max predictions
 *
 * @param heatmaps the tensor containing the heatmaps
 * @param num_joints the number of joints of the skeleton
 * @param width the width of the image
 * @param height the height of the image
 * @return xt::xarray<float>
 */
xt::xarray<float> get_max_predictions(xt::xarray<float> &heatmaps, int num_joints, int width, int height)
{
    xt::xarray<float> heatmaps_reshape = xt::reshape_view(heatmaps, {num_joints, 3072});
    xt::xarray<int> max_indices = xt::argmax(heatmaps_reshape, 1);
    xt::xarray<int> max_indices_reshape = xt::reshape_view(max_indices, {num_joints, 1});
    xt::xarray<float> max_vals = xt::amax(heatmaps_reshape, 1);
    max_vals = xt::where(max_vals > 1.0, 1.0, max_vals); // tappas doesn't allow confidence to be greater than 1
    xt::xarray<float> max_vals_reshape = xt::reshape_view(max_vals, {num_joints, 1});
    xt::xarray<int> preds_ = xt::tile(max_indices_reshape, {1, 2});
    xt::xarray<int> preds_first = xt::view(preds_, xt::all(), 0) % width;
    xt::view(preds_, xt::all(), 0) = preds_first;
    xt::xarray<int> preds_second_ = xt::view(preds_, xt::all(), 1);
    xt::xarray<int> preds_second = floor(preds_second_ / width);
    xt::view(preds_, xt::all(), 1) = preds_second;
    xt::xarray<float> preds = xt::where(xt::tile(max_vals_reshape, {1, 2}) > 0.0, preds_, -1);
    xt::xarray<float> preds_with_confidence = xt::hstack(xt::xtuple(preds, max_vals_reshape / 255 + 0.5));

    return preds_with_confidence;
}

/**
 * @brief calculates the joints and resizes them to the network input size
 *
 * @param heatmaps the tensor containing the heatmaps
 * @param preds the tensor containing the predictions
 * @param num_joints the number of joints of the skeleton
 * @param width the width of the image
 * @param height the height of the image
 */
void calculate_joints_and_resize(xt::xarray<float> &heatmaps, xt::xarray<float> &preds, int num_joints, int width, int height)
{
    xt::xarray<float> heatmap;
    int px, py;
    float diff;
    for (int k = 0; k < num_joints; k++)
    {
        heatmap = xt::view(heatmaps, k, xt::all(), xt::all());
        px = (int)preds(k, 0);
        py = (int)preds(k, 1);
        if (px < width - 1 && px > 1 && py < height - 1 && py > 1)
        {
            diff = heatmap(py, px + 1) - heatmap(py, px - 1);
            preds(k, 0) += (diff > 0) ? 0.75 : 0.25;

            diff = heatmap(py + 1, px) - heatmap(py - 1, px);
            preds(k, 0) += (diff > 0) ? 0.75 : 0.25;
        }
        preds(k, 0) = preds(k, 0) / 48;
        preds(k, 1) = preds(k, 1) / 64;
    }
}

/**
 * @brief mspn post process
 *
 * @param roi region of interest
 * @param score_threshold threshold for score filterig
 * @return std::vector<HailoDetection> the detected objects
 */
void mspn_postprocess(HailoROIPtr roi,
                      const float score_threshold)
{
    std::vector<HailoDetection> objects; // The detection meta we will eventually return
    HailoTensorPtr tensor = roi->get_tensors()[0];
    xt::xarray<float> tensor_xarray = common::get_xtensor_float(tensor);
    xt::xarray<float> heatmaps = xt::transpose(tensor_xarray, {2, 0, 1});
    int num_joints = heatmaps.shape()[0];
    int height = heatmaps.shape()[1];
    int width = heatmaps.shape()[2];

    gaussian_blur(heatmaps, num_joints, width, height);

    xt::xarray<float> preds = get_max_predictions(heatmaps, num_joints, width, height);

    calculate_joints_and_resize(heatmaps, preds, num_joints, width, height);

    std::vector<HailoPoint> points;
    points.reserve(num_joints);
    for (int i = 0; i < num_joints; i++)
    {
        points.emplace_back(HailoPoint(preds(i, 0), preds(i, 1), preds(i, 2)));
    }
    roi->add_object(std::make_shared<HailoLandmarks>("centerpose", points, score_threshold, centerpose_joint_pairs));
}

/**
 * @brief Perform post process and add the detected objects to the roi object
 *
 * @param roi region of interest
 */
void mspn(HailoROIPtr roi)
{
    if (roi->has_tensors())
    {
        mspn_postprocess(roi, SCORE_THRESHOLD);
    }
}

void filter(HailoROIPtr roi)
{
    mspn(roi);
}
