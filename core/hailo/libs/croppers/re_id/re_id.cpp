/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include <vector>
#include <iostream>
#include "re_id.hpp"

#define PERSON_LABEL "person"
#define MIN_RATIO (1.7f)
#define MAX_RATIO (4.5f)
#define MIN_HEIGHT (0.3f)
#define MAX_HEIGHT (0.8f)
#define MIN_X (0.05f)
#define MAX_X (0.95f)
#define TRACK_DELAY (5)
#define MIN_QUALITY (400)
#define RE_ID_NETWORK_SIZE (cv::Size(128, 256))
std::map<int, int> track_counter;

/**
 * @brief Returns the quaility estimation of the person's crop.
 *
 * @param image  -  cv::Mat
 *        The original image.
 *
 * @param roi  -  HailoBBox
 *        The Bounding box of the person to calculate quality estimation on.
 *
 * @return float
 *         The quality estimation of the person.
 */
float quality_estimation(const cv::Mat &image, const HailoBBox &roi)
{
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
    cv::resize(cropped_image, resized_image, RE_ID_NETWORK_SIZE, 0, 0, cv::INTER_LINEAR);

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

    // Release resources
    resized_image.release();
    cropped_image.release();
    gray_image.release();
    laplacian_image.release();

    return quality;
}

HailoUniqueIDPtr get_tracking_id(HailoDetectionPtr detection)
{
    for (auto obj : detection->get_objects_typed(HAILO_UNIQUE_ID))
    {
        HailoUniqueIDPtr id = std::dynamic_pointer_cast<HailoUniqueID>(obj);
        if (id->get_mode() == TRACKING_ID)
        {
            return id;
        }
    }
    return nullptr;
}

/**
 * @brief Returns a vector of HailoROIPtr to crop and resize.
 *
 * @param image The original picture (cv::Mat).
 * @param roi The main ROI of this picture.
 * @return std::vector<HailoROIPtr> vector of ROI's to crop and resize.
 */
std::vector<HailoROIPtr> create_crops(std::shared_ptr<HailoMat> image, HailoROIPtr roi)
{
    std::vector<HailoROIPtr> crop_rois;
    // Get all detections.
    std::vector<HailoDetectionPtr> detections_ptrs = hailo_common::get_hailo_detections(roi);
    for (HailoDetectionPtr &detection : detections_ptrs)
    {
        // Modify only detections with "person" label.
        if (std::string(PERSON_LABEL) == detection->get_label())
        {
            // Remove previous matrices
            roi->remove_objects_typed(HAILO_MATRIX);

            int tracking_id = get_tracking_id(detection)->get_id();

            auto counter = track_counter.find(tracking_id);
            if (counter == track_counter.end())
            {
                track_counter[tracking_id] = 0;
            }
            else if (counter->second < TRACK_DELAY)
            {
                track_counter[tracking_id] += 1;
            }
            else
            {
                auto bbox = detection->get_bbox();
                float quality = quality_estimation(image->get_mat(), bbox);
                float ratio = (bbox.height() * image->height()) / (bbox.width() * image->width());
                if (ratio > MIN_RATIO && ratio < MAX_RATIO && 
                    bbox.height() > MIN_HEIGHT && bbox.height() < MAX_HEIGHT &&
                    bbox.xmin() > MIN_X && bbox.xmax() < MAX_X && quality > MIN_QUALITY)
                {
                    crop_rois.emplace_back(detection);
                }
            }
        }
    }
    return crop_rois;
}
