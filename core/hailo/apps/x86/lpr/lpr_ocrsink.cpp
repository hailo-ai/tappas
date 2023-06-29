/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/

#include <gst/video/video-format.h>
#include <iostream>
#include <map>
#include <typeinfo>
#include <math.h>

// Hailo includes
#include "hailo_objects.hpp"
#include "hailo_common.hpp"
#include "lpr_ocrsink.hpp"
#include "hailo_cv_singleton.hpp"
#include "hailo_tracker.hpp"
#include "image.hpp"

// Open source includes
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

// General
#define MAP_LIMIT (5)              // Number of license plates to store at any time
#define OCR_SCORE_THRESHOLD (0.90) // OCR score threshold
int singleton_map_key = 0;
std::vector<int> seen_ocr_track_ids;
const gchar *OCR_LABEL_TYPE = "ocr";
std::string tracker_name = "hailo_tracker";

void catalog_nv12_mat(std::string text, cv::Mat &mat)
{
    // Resize the mat to a presentable size, add padding
    int target_h = 114;
    int target_w = 300;
    cv::Mat resized_nv12 = cv::Mat(target_h, target_w, CV_8UC1);
    resize_nv12(mat, resized_nv12);

    // Split planes
    int y_h = target_h * 2 / 3;
    int y_w = target_w;
    int uv_h = target_h / 3;
    int uv_w = target_w / 2;
    cv::Mat y_mat = cv::Mat(y_h, y_w, CV_8UC1, (char *)resized_nv12.data, resized_nv12.step);
    cv::Mat uv_mat = cv::Mat(uv_h, uv_w, CV_8UC2, (char *)resized_nv12.data + (y_h * y_w), resized_nv12.step);

    // To make padding, prepare a padded mat and split channels from that mat
    int padded_h = target_h + 45;
    cv::Mat padded_nv12 = cv::Mat(padded_h, target_w, CV_8UC1, cv::Scalar(0));
    int padded_y_h = padded_h * 2 / 3;
    int padded_y_w = target_w;
    int padded_uv_h = padded_h / 3;
    int padded_uv_w = target_w / 2;
    cv::Mat padded_y_mat = cv::Mat(padded_y_h, padded_y_w, CV_8UC1, (char *)padded_nv12.data, padded_nv12.step);
    cv::Mat padded_uv_mat = cv::Mat(padded_uv_h, padded_uv_w, CV_8UC2, (char *)padded_nv12.data + (padded_y_h * padded_y_w), padded_nv12.step);

    // Fill the padded image with white padding
    cv::copyMakeBorder(y_mat, padded_y_mat, 30, 0, 0, 0, cv::BORDER_CONSTANT, cv::Scalar(235));
    cv::copyMakeBorder(uv_mat, padded_uv_mat, 15, 0, 0, 0, cv::BORDER_CONSTANT, cv::Scalar(128, 128));

    // Draw text on the two channels
    cv::Point y_position = cv::Point(4, 24);
    cv::Point uv_position = cv::Point(2, 12);
    cv::putText(padded_y_mat, text, y_position, cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(65), 2);
    cv::putText(padded_uv_mat, text, uv_position, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(110, 255), 1);

    CVMatSingleton::GetInstance().set_mat_at_key(singleton_map_key % MAP_LIMIT, padded_nv12);
    CVMatSingleton::GetInstance().set_mat_type(HAILO_MAT_NV12);
}

void catalog_yuy2_mat(std::string text, cv::Mat &mat)
{
    // Resize the mat to a presentable size, add padding
    cv::Mat padded_yuy2;
    cv::Mat resized_yuy2 = cv::Mat(75, 150, CV_8UC4);
    resize_yuy2(mat, resized_yuy2);

    // Add padding top, bottom, left, right, borderType
    cv::copyMakeBorder(resized_yuy2, padded_yuy2, 30, 0, 0, 0, cv::BORDER_CONSTANT, cv::Scalar(235, 128, 235, 128));

    // write the OCR text on that padding (view as 2 channel su the yuy2 draws correctly)
    cv::Mat image_2_channel = cv::Mat(padded_yuy2.rows, padded_yuy2.cols * 2, CV_8UC2, (char *)padded_yuy2.data, padded_yuy2.step);
    auto text_position = cv::Point(5, 25);
    cv::putText(image_2_channel, text, text_position, cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(81, 90, 81, 239), 2);

    // Set the new license plate in our CV Map singleton
    CVMatSingleton::GetInstance().set_mat_at_key(singleton_map_key % MAP_LIMIT, padded_yuy2);
    CVMatSingleton::GetInstance().set_mat_type(HAILO_MAT_YUY2);
}

void catalog_rgb_mat(std::string text, cv::Mat &mat)
{
    // Resize the mat to a presentable size, add padding
    cv::Mat resized_image;
    cv::Mat padded_image;
    cv::resize(mat, resized_image, cv::Size(300, 75), 0, 0, cv::INTER_AREA);

    // Add padding top, bottom, left, right, borderType
    cv::copyMakeBorder(resized_image, padded_image, 30, 0, 0, 0, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

    // write the OCR text on that padding
    auto text_position = cv::Point(10, 25);
    cv::putText(padded_image, text, text_position, cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 0, 0), 2);

    // Set the new license plate in our CV Map singleton
    CVMatSingleton::GetInstance().set_mat_at_key(singleton_map_key % MAP_LIMIT, padded_image);
    CVMatSingleton::GetInstance().set_mat_type(HAILO_MAT_RGB);
}

void catalog_license_plate(std::string label, float confidence, HailoBBox license_plate_box, std::shared_ptr<HailoMat> hmat, HailoROIPtr crop_roi)
{
    cv::Mat &mat = hmat->get_mat();
    // Prepare the cropped license plate and text
    std::string text = label + " " + std::to_string((int)(confidence * 100)) + "%";
    cv::Rect rect;
    rect.x = CLAMP(license_plate_box.xmin() * mat.cols, 0, mat.cols);
    rect.y = CLAMP(license_plate_box.ymin() * mat.rows, 0, mat.rows);
    rect.width = CLAMP(license_plate_box.width() * mat.cols, 0, mat.cols - rect.x);
    rect.height = CLAMP(license_plate_box.height() * mat.rows, 0, mat.rows - rect.y);
    if (rect.width == 0 || rect.height == 0)
        return;
    cv::Mat cropped_image = hmat->crop(crop_roi);

    switch (hmat->get_type())
    {
    case HAILO_MAT_YUY2:
    {
        catalog_yuy2_mat(text, cropped_image);
        break;
    }
    case HAILO_MAT_RGB:
    {
        catalog_rgb_mat(text, cropped_image);
        break;
    }
    case HAILO_MAT_NV12:
    {
        catalog_nv12_mat(text, cropped_image);
        break;
    }
    default:
        break;
    }

    singleton_map_key++;
}

void ocr_sink(HailoROIPtr roi, std::shared_ptr<HailoMat> hmat)
{
    if (nullptr == roi)
        return;

    std::vector<HailoDetectionPtr> vehicle_detections;   // The vehicle detections in the ROI
    std::vector<HailoUniqueIDPtr> unique_ids;            // The unique ids of those vehicle detections
    std::vector<HailoDetectionPtr> lp_detections;        // The license plate detections in those vehicle detections
    std::vector<HailoClassificationPtr> classifications; // The classifications of those license plate detections
    float confidence;                                    // The confidence of those classifications
    std::string license_plate_ocr_label;                 // The labels of those classifications
    std::string jde_tracker_name = tracker_name + "_" + roi->get_stream_id();

    // For each roi, check the detections
    vehicle_detections = hailo_common::get_hailo_detections(roi);
    for (HailoDetectionPtr &vehicle_detection : vehicle_detections)
    {
        // Get the unique id of the detection
        unique_ids = hailo_common::get_hailo_unique_id(vehicle_detection);
        if (unique_ids.empty())
            continue;

        // For each vehicle, get the license plate detection
        lp_detections = hailo_common::get_hailo_detections(vehicle_detection);
        for (HailoDetectionPtr &lp_detection : lp_detections)
        {
            HailoBBox license_plate_box = hailo_common::create_flattened_bbox(lp_detection->get_bbox(), lp_detection->get_scaling_bbox());
            // For each license plate detection, check the classifications
            classifications = hailo_common::get_hailo_classifications(lp_detection);
            if (classifications.size() != 1)
            {
                vehicle_detection->remove_object(lp_detection);  // If no ocr was found then remove this license plate
                continue;
            }
            HailoClassificationPtr classification = classifications[0];
            if (OCR_LABEL_TYPE == classification->get_classification_type())
            {
                confidence = classification->get_confidence();
                license_plate_ocr_label = classification->get_label();
                if (std::find(seen_ocr_track_ids.begin(), seen_ocr_track_ids.end(), unique_ids[0]->get_id()) != seen_ocr_track_ids.end())
                    continue;  // this track id was already updated
                else
                    seen_ocr_track_ids.emplace_back(unique_ids[0]->get_id());

                // Update the tracker with the found ocr
                HailoTracker::GetInstance().add_object_to_track(jde_tracker_name,
                                                                unique_ids[0]->get_id(),
                                                                classification);

                catalog_license_plate(license_plate_ocr_label, confidence, license_plate_box, hmat, lp_detection);
            }
        }
    }

    return;
}

void filter(HailoROIPtr roi, GstVideoFrame *frame)
{
    std::shared_ptr<HailoMat> hmat = get_mat_by_format(*(&frame->buffer), &frame->info, 1, 1);
    ocr_sink(roi, hmat);
}