/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include "lpr_croppers.hpp"
#include <iostream>

#define VEHICLE_LABEL "car"
#define LICENSE_PLATE_LABEL "license_plate"
#define OCR_LABEL "ocr"

/**
 * @brief Returns the calculate the variance of edges.
 *
 * @param image  -  cv::Mat
 *        The original image.
 *
 * @param roi  -  HailoBBox
 *        The ROI to read from the image
 *
 * @param crop_ratio  -  float
 *        The percent of the image to crop in from the edges (default 10%).
 *
 * @return float
 *         The variance of edges in the image.
 */
float quality_estimation(std::shared_ptr<HailoMat> hailo_mat, const HailoBBox &roi, const float crop_ratio = 0.1)
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

    // If the cropepd image is too small then quality is zero
    if (cropped_width <= CROP_WIDTH_LIMIT || cropped_height <= CROP_HEIGHT_LIMIT)
        return -1.0;

    // If it is not too small then we can make the crop
    HailoROIPtr crop_roi = std::make_shared<HailoROI>(HailoBBox(cropped_xmin, cropped_ymin, cropped_width_n, cropped_height_n));
    cv::Mat cropped_image = hailo_mat->crop(crop_roi);

    // Convert image to BGR
    cv::Mat bgr_image;
    switch (hailo_mat->get_type())
    {
    case HAILO_MAT_YUY2:
    {
        cv::Mat yuy2_image = cv::Mat(cropped_image.rows, cropped_image.cols * 2, CV_8UC2, (char *)cropped_image.data, cropped_image.step);
        cv::cvtColor(yuy2_image, bgr_image, cv::COLOR_YUV2BGR_YUY2);
        break;
    }
    case HAILO_MAT_NV12:
    {
        cv::cvtColor(cropped_image, bgr_image, cv::COLOR_YUV2BGR_NV12);
        break;
    }
    default:
        bgr_image = cropped_image;
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

/**
 * @brief Returns a vector of HailoROIPtr to crop and resize.
 *        Specific to LPR pipelines, this function assumes that
 *        license plate ROIs are nested inside vehicle detection ROIs.
 *
 * @param image  -  cv::Mat
 *        The original image.
 *
 * @param roi  -  HailoROIPtr
 *        The main ROI of this picture.
 *
 * @return std::vector<HailoROIPtr>
 *         vector of ROI's to crop and resize.
 */
std::vector<HailoROIPtr> license_plate_quality_estimation(std::shared_ptr<HailoMat> image, HailoROIPtr roi)
{
    std::vector<HailoROIPtr> crop_rois;
    float variance;
    // Get all detections.
    std::vector<HailoDetectionPtr> vehicle_ptrs = hailo_common::get_hailo_detections(roi);
    for (HailoDetectionPtr &vehicle : vehicle_ptrs)
    {
        if (VEHICLE_LABEL != vehicle->get_label())
            continue;
        // For each detection, check the inner detections
        std::vector<HailoDetectionPtr> license_plate_ptrs = hailo_common::get_hailo_detections(vehicle);
        for (HailoDetectionPtr &license_plate : license_plate_ptrs)
        {
            if (LICENSE_PLATE_LABEL != license_plate->get_label())
                continue;
            HailoBBox license_plate_box = hailo_common::create_flattened_bbox(license_plate->get_bbox(), license_plate->get_scaling_bbox());

            // Get the variance of the image, only add ROIs that are above threshold.
            variance = quality_estimation(image, license_plate_box, CROP_RATIO);

            if (variance >= QUALITY_THRESHOLD)
            {
                crop_rois.emplace_back(license_plate);
            }
            else
            {
                vehicle->remove_object(license_plate); // If it is not a good license plate, then remove it!
            }
        }
    }
    return crop_rois;
}

/**
 * @brief Returns a vector of HailoROIPtr to crop and resize.
 *        Specific to LPR pipelines, this function searches if
 *        a detected vehicle has an OCR classification. If not,
 *        then it is submitted for cropping.
 *        This function also throws out car detections that are not yet
 *        fully in the image.
 *
 * @param image  -  cv::Mat
 *        The original image.
 *
 * @param roi  -  HailoROIPtr
 *        The main ROI of this picture.
 *
 * @return std::vector<HailoROIPtr>
 *         vector of ROI's to crop and resize.
 */
std::vector<HailoROIPtr> vehicles_without_ocr(std::shared_ptr<HailoMat> image, HailoROIPtr roi)
{
    std::vector<HailoROIPtr> crop_rois;
    bool has_ocr = false;
    // Get all detections.
    std::vector<HailoDetectionPtr> detections_ptrs = hailo_common::get_hailo_detections(roi);
    for (HailoDetectionPtr &detection : detections_ptrs)
    {
        HailoBBox vehicle_bbox = detection->get_bbox();
        // If the bbox is not yet in the image, then throw it out
        if ((vehicle_bbox.xmin() < 0.0) ||
            (vehicle_bbox.xmax() > 1.0) ||
            (vehicle_bbox.ymin() < 0.0) ||
            (vehicle_bbox.ymax() > 1.0))
            continue;

        int vehicle_width = vehicle_bbox.width() * image->width();
        int vehicle_height = vehicle_bbox.height() * image->height();
        if ((vehicle_width * vehicle_height) < 40000)
            continue;

        // if the bbox is above the top half of the image then throw it out
        if (vehicle_bbox.ymax() < 0.75)
            continue;

        has_ocr = false;
        // For each detection, check the classifications
        std::vector<HailoClassificationPtr> vehicle_classifications = hailo_common::get_hailo_classifications(detection);
        for (HailoClassificationPtr &classification : vehicle_classifications)
        {
            if (OCR_LABEL == classification->get_classification_type())
            {
                has_ocr = true;
                break;
            }
        }
        if (!has_ocr)
            crop_rois.emplace_back(detection);
    }
    return crop_rois;
}
