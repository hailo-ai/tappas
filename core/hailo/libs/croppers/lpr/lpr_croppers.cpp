/**
 * Copyright (c) 2021-2026 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include "lpr_croppers.hpp"
#include "opencv_utils.hpp"
#include <iostream>

#define VEHICLE_LABEL "car"
#define LICENSE_PLATE_LABEL "license_plate"
#define OCR_LABEL "ocr"

float quality_estimation(std::shared_ptr<HailoMat> hailo_mat, const HailoBBox &roi, const float crop_ratio)
{
    return OpenCVUtils::quality_estimation(hailo_mat, roi, crop_ratio, CROP_WIDTH_LIMIT, CROP_HEIGHT_LIMIT);
}

/**
 * @brief Returns a vector of HailoROIPtr to crop and resize.
 *        Specific to LPR pipelines, this function assumes that
 *        license plate ROIs are nested inside vehicle detection ROIs.
 *
 * @param image  - std::shared_ptr<HailoMat>
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
 * @param image  - std::shared_ptr<HailoMat>
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
