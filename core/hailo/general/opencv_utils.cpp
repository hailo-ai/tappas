#include <opencv2/opencv.hpp>
#include <xtensor/xarray.hpp>
#include <xtensor/xview.hpp>
#include <xtensor/xadapt.hpp>
#include "opencv_utils.hpp"
#include "hailomat_internal.hpp"
#include <vector>
#include <cmath>
#include <stdexcept>

namespace OpenCVUtils {

void normalize(std::vector<float> &feat)
{
    // call cv::norm
    cv::Mat feat_mat(feat);
    float feat_value = cv::norm(feat_mat);
    for (uint i = 0; i < feat.size(); ++i)
    {
        feat[i] /= feat_value;
    }
}

void gaussian_blur(float* heatmaps_data, int num_joints, int width, int height, int kernel_size, float eps)
{
    if (kernel_size % 2 == 0)
    {
        throw std::runtime_error("Kernel size must be odd");
    }

    int border = floor((kernel_size - 1) / 2);
    
    // Adapt the raw pointer to xtensor view
    std::vector<std::size_t> shape = {static_cast<std::size_t>(num_joints), static_cast<std::size_t>(height), static_cast<std::size_t>(width)};
    auto heatmaps = xt::adapt(heatmaps_data, num_joints * height * width, xt::no_ownership(), shape);

    for (int j = 0; j < num_joints; j++)
    {
        auto sliced_view = xt::view(heatmaps, j, xt::all(), xt::all());
        float origin_max = xt::amax(sliced_view)(0);
        cv::Mat dr = cv::Mat::zeros(height + 2 * border, width + 2 * border, CV_32F);
        cv::Mat sliced_view_mat(height, width, CV_32F, &sliced_view(0, 0), 0);
        cv::copyMakeBorder(sliced_view_mat, dr, border, border, border, border, cv::BORDER_CONSTANT, 0);
        cv::Mat gaussian_image;
        cv::GaussianBlur(dr, gaussian_image, cv::Size(kernel_size, kernel_size), 0);
        cv::Mat sliced_view_mat2(height, width, CV_32F, &sliced_view(0, 0), 0);
        cv::Mat gaussian_image_roi = gaussian_image(cv::Rect(border, border, width, height));
        gaussian_image_roi.copyTo(sliced_view_mat2);
        float new_max = std::max(xt::amax(sliced_view)(0), eps);
        sliced_view *= (origin_max / new_max);
    }
}

float quality_estimation(std::shared_ptr<HailoMat> hailo_mat, const HailoBBox &roi, float crop_ratio, int crop_width_limit, int crop_height_limit)
{
    // Crop the center of the roi from the image, avoid cropping out of bounds
    float roi_width = roi.width();
    float roi_height = roi.height();
    float roi_xmin = roi.xmin();
    float roi_ymin = roi.ymin();
    float roi_xmax = roi.xmax();
    float roi_ymax = roi.ymax();
    float x_offset = roi_width * crop_ratio;
    float y_offset = roi_height * crop_ratio;
    float cropped_xmin = CLAMP(roi_xmin + x_offset, 0, 1);
    float cropped_ymin = CLAMP(roi_ymin + y_offset, 0, 1);
    float cropped_xmax = CLAMP(roi_xmax - x_offset, cropped_xmin, 1);
    float cropped_ymax = CLAMP(roi_ymax - y_offset, cropped_ymin, 1);
    float cropped_width_n = cropped_xmax - cropped_xmin;
    float cropped_height_n = cropped_ymax - cropped_ymin;
    int cropped_width = int(cropped_width_n * hailo_mat->native_width());
    int cropped_height = int(cropped_height_n * hailo_mat->native_height());

    // If the cropped image is too small then quality is zero
    if (cropped_width <= crop_width_limit || cropped_height <= crop_height_limit)
        return -1.0;

    // If it is not too small then we can make the crop
    HailoROIPtr crop_roi = std::make_shared<HailoROI>(HailoBBox(cropped_xmin, cropped_ymin, cropped_width_n, cropped_height_n));
    std::vector<cv::Mat> cropped_image_vec = crop_to_cv_matrices(*hailo_mat, crop_roi);

    // Convert image to BGR
    cv::Mat bgr_image;
    switch (hailo_mat->get_type())
    {
    case HAILO_MAT_YUY2:
    {
        cv::Mat cropped_image = cropped_image_vec[0];
        cv::Mat yuy2_image = cv::Mat(cropped_image.rows, cropped_image.cols * 2, CV_8UC2, (char *)cropped_image.data, cropped_image.step);
        cv::cvtColor(yuy2_image, bgr_image, cv::COLOR_YUV2BGR_YUY2);
        break;
    }
    case HAILO_MAT_NV12:
    {
        cv::Mat full_mat = cv::Mat(cropped_image_vec[0].rows + cropped_image_vec[1].rows, cropped_image_vec[0].cols, CV_8UC1);
        memcpy(full_mat.data, cropped_image_vec[0].data, cropped_image_vec[0].rows * cropped_image_vec[0].cols);
        memcpy(full_mat.data + cropped_image_vec[0].rows * cropped_image_vec[0].cols, cropped_image_vec[1].data, cropped_image_vec[1].rows * cropped_image_vec[1].cols);
        cv::cvtColor(full_mat, bgr_image, cv::COLOR_YUV2BGR_NV12);
        break;
    }
    default:
        bgr_image = cropped_image_vec[0];
        break;
    }

    // Resize the frame
    cv::Mat resized_image;
    cv::resize(bgr_image, resized_image, cv::Size(200, 40), 0, 0, cv::INTER_AREA);

    // Gaussian Blur
    cv::Mat gaussian_image;
    cv::GaussianBlur(resized_image, gaussian_image, cv::Size(3, 3), 0);

    // Convert to grayscale
    cv::Mat gray_image;
    cv::Mat gray_image_normalized;
    cv::cvtColor(gaussian_image, gray_image, cv::COLOR_BGR2GRAY);
    cv::normalize(gray_image, gray_image_normalized, 255, 0, cv::NORM_INF);

    // Compute the Laplacian of the gray image
    cv::Mat laplacian_image;
    cv::Laplacian(gray_image_normalized, laplacian_image, CV_64F);

    // Calculate the variance of edges
    cv::Scalar mean, stddev;
    cv::meanStdDev(laplacian_image, mean, stddev, cv::Mat());
    float variance = stddev.val[0] * stddev.val[0];
    return variance;
}

float quality_estimation_reid(std::shared_ptr<HailoMat> hailo_mat, const HailoBBox &roi, int network_width, int network_height)
{
    const cv::Mat& image = get_cv_matrices(*hailo_mat)[0];
    
    // Crop the center of the roi from the image, avoid cropping out of bounds
    int cropped_xmin = CLAMP((image.cols * roi.xmin()), 0, image.cols);
    int cropped_ymin = CLAMP((image.rows * roi.ymin()), 0, image.rows);
    int cropped_xmax = CLAMP((image.cols * roi.xmax()), cropped_xmin, image.cols);
    int cropped_ymax = CLAMP((image.rows * roi.ymax()), cropped_ymin, image.rows);
    int cropped_width = cropped_xmax - cropped_xmin;
    int cropped_height = cropped_ymax - cropped_ymin;

    // If it is not too small then we can make the crop
    cv::Rect center_crop(cropped_xmin, cropped_ymin, cropped_width, cropped_height);
    cv::Mat cropped_image = image(center_crop);

    // Resize the frame
    cv::Mat resized_image;
    cv::resize(cropped_image, resized_image, cv::Size(network_width, network_height), 0, 0, cv::INTER_LINEAR);

    // Convert to grayscale
    cv::Mat gray_image;
    cv::cvtColor(resized_image, gray_image, cv::COLOR_RGB2GRAY);

    // Compute the Laplacian of the gray image
    cv::Mat laplacian_image;
    cv::Laplacian(gray_image, laplacian_image, CV_64F);

    // Calculate the quality of person
    cv::Scalar mean, stddev;
    cv::meanStdDev(laplacian_image, mean, stddev, cv::Mat());
    float quality = stddev.val[0] * stddev.val[0];

    return quality;
}

}