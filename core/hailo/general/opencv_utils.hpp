#pragma once

#include <vector>
#include <memory>
#include "hailomat.hpp"
#include "hailo_objects.hpp"


namespace OpenCVUtils {

void normalize(std::vector<float> &feat);

/**
 * @brief Performs gaussian blur on heatmaps
 *
 * @param heatmaps_data Pointer to the heatmaps data (num_joints x height x width layout)
 * @param num_joints the number of joints of the skeleton
 * @param width the width of the image
 * @param height the height of the image
 * @param kernel_size the size of the gaussian kernel (must be odd)
 * @param eps epsilon value for numerical stability
 */
void gaussian_blur(float* heatmaps_data, int num_joints, int width, int height, int kernel_size, float eps);

/**
 * @brief Returns the variance of edges for quality estimation.
 *
 * @param hailo_mat The HailoMat image
 * @param roi The ROI to analyze
 * @param crop_ratio The percent of the image to crop from edges (default 10%)
 * @param crop_width_limit Minimum width limit for cropped image
 * @param crop_height_limit Minimum height limit for cropped image
 * @return float The variance of edges in the image, or -1.0 if too small
 */
float quality_estimation(std::shared_ptr<HailoMat> hailo_mat, const HailoBBox &roi, float crop_ratio, int crop_width_limit, int crop_height_limit);

/**
 * @brief Returns the quality estimation of a person's crop for re-id.
 *
 * @param hailo_mat The HailoMat image
 * @param roi The bounding box of the person
 * @param network_width Width to resize to for quality calculation
 * @param network_height Height to resize to for quality calculation
 * @return float The quality estimation of the person
 */
float quality_estimation_reid(std::shared_ptr<HailoMat> hailo_mat, const HailoBBox &roi, int network_width, int network_height);

}