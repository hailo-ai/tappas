/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include <gst/video/video.h>
#include <iostream>
#include <map>
#include <opencv2/opencv.hpp>
#include <typeinfo>
#include <math.h>
#include "common/color_maps/yolact20.hpp"
#include "detection_draw.hpp"
#include "hailo_detection.hpp"
#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"
#include "xtensor/xio.hpp"
#include "xtensor/xview.hpp"

// Detection
#define DETECTION_TEXT_MARGIN (2)
#define DEFAULT_SIGNIFICANT_FIGURES (3)
#define DEFAULT_FORMAT_FACTOR (1)
#define YUY2_FORMAT_FACTOR (0.5)
#define TEXT_FONT_FACTOR (0.12)
#define MINIMAL_BOX_WIDTH_FOR_TEXT (10)

// Classification
#define TEXT_CLS_FONT_SCALE (0.45)
#define TEXT_CLS_COLOR (cv::Scalar(200, 200, 200))
#define TEXT_CLS_HEIGHT_MARGIN (40)
#define TYPE_CLASSIFICATION "classification_result"
#define TYPE "type"
#define LABEL "label"
#define CONFIDENCE "confidence"

// Keypoints
#define KEYPOINT_SCALE_FACTOR (150)

// General
#define TEXT_THICKNESS (1)
#define TEXT_CLS_THICKNESS (2)
#define SPACE " "

std::string round_confidence(float confidence, uint significant_figures = DEFAULT_SIGNIFICANT_FIGURES)
{
    size_t count = std::to_string(confidence).find(".") + significant_figures;
    std::string rounded_confidence = std::to_string(confidence).substr(0, count);
    return rounded_confidence;
}

void draw_segmentation(cv::Mat &image_planes, GstStructure *segmentation_data, int image_width, int image_height,
                       int box_width, int box_height, DetectionObject &bbox, gint cv2_format)
{
    gsize mask_size;
    int mask_height = 0;
    int mask_width = 0;
    // First get the mask size, if false is returned then
    bool mask_available = gst_structure_get(segmentation_data, "mask_size", G_TYPE_INT, &mask_size, NULL);
    if (!mask_available)
    {
        // If there is no mask available, then return
        return;
    }
    // Gather the mask raw data from the GstStructure
    const GValue *mask_gvalue = gst_structure_get_value(segmentation_data, "mask"); // Get the g value from the gststructure
    GVariant *mask_variant = g_value_get_variant(mask_gvalue);                      // From the g value extract the variant
    // From the variant extract the uint8 array
    uint8_t *mask_raw = (guint8 *)g_variant_get_fixed_array(mask_variant, &mask_size, 1);

    // Get the mask width and height, this is independent of image size and not the same for each detection
    gst_structure_get(segmentation_data, "mask_height", G_TYPE_INT, &mask_height, NULL);
    gst_structure_get(segmentation_data, "mask_width", G_TYPE_INT, &mask_width, NULL);

    // Make the cv mask and resize it to the bounding box size
    cv::Mat resized_mask;
    cv::Mat mask_plane(mask_height, mask_width, CV_32F, (float *)mask_raw);
    cv::resize(mask_plane, resized_mask, {box_width, box_height}); // Bilinear resize

    // Apply the mask to the image plane
    std::vector<uint8_t> mask_color = common::yolact20_colors[bbox.class_id]; // Get the color for this class
    // Relevant mask values will only appear within their bounding box, so iterate over the bounds.
    int box_xmin = bbox.xmin * image_width;
    int box_ymin = bbox.ymin * image_height;
    for (int row = box_ymin + 1; row < (bbox.ymax * image_height) - 1; row++)
    {
        for (int col = box_xmin + 1; col < (bbox.xmax * image_width) - 1; col++)
        {
            // Check that the pixel is scored above threshold
            if (resized_mask.at<float>(row - box_ymin, col - box_xmin) > 0.5)
            {
                // Apply alpha blending based on class colors
                cv::Vec3b &image_pixel = image_planes.at<cv::Vec3b>(row, col);
                image_pixel[0] = image_pixel[0] * 0.7 + mask_color[0] * 0.3;
                image_pixel[1] = image_pixel[1] * 0.7 + mask_color[1] * 0.3;
                image_pixel[2] = image_pixel[2] * 0.7 + mask_color[2] * 0.3;
                image_planes.at<cv::Vec3b>(row, col) = image_pixel;
            }
        }
    }
    // Release the cv matrices
    mask_plane.release();
    resized_mask.release();
}

void draw_landmarks(cv::Mat &image_planes, GstStructure *landmarks, DetectionObject &bbox, cv::Scalar &color1, cv::Scalar &color2, int thickness)
{
    cv::Point center;
    gsize keypoints_size;
    cv::Size axes = {3, 3};
    // First get the size of the landmarks, if this field is not set then the landmarks structure must be empty
    bool landmarks_available = gst_structure_has_field_typed(landmarks, "keypoints_size", G_TYPE_INT);
    if (!landmarks_available)
    {
        // If there are no landmarks available, then return
        return;
    }
    (void)gst_structure_get(landmarks, "keypoints_size", G_TYPE_INT, &keypoints_size, NULL);
    bool has_inner_keypoints = gst_structure_has_field(landmarks, "inner_keypoints");
    const GValue *keypoints_gvalue = gst_structure_get_value(landmarks, "keypoints"); // Get the g value from the gststructure
    GVariant *keypoints_variant = g_value_get_variant(keypoints_gvalue);              // From the g value extract the variant
    // From the variant extract the uint8 array
    float *keypoints_raw = (float *)g_variant_get_fixed_array(keypoints_variant, &keypoints_size, 1);
    // Adapt the variant array into an xtensor array
    size_t size = keypoints_size / 4; // There 4 bytes per element, so divide by 4
    size_t num_keypoints = size / 2;  // each keypoint has x and y, so divide by 2
    std::vector<std::size_t> shape = {num_keypoints, 2};
    xt::xarray<float> xjoints = xt::adapt(keypoints_raw, size, xt::no_ownership(), shape);

    if (has_inner_keypoints)
    {
        xt::view(xjoints, xt::all(), 0) = xt::view(xjoints, xt::all(), 0) * bbox.width + bbox.xmin;
        xt::view(xjoints, xt::all(), 1) = xt::view(xjoints, xt::all(), 1) * bbox.height + bbox.ymin;
        thickness = 1;
        axes = {1, 1};
    }

    // For each keypoint
    for (uint index = 0; index < num_keypoints; ++index)
    {
        // If the keypoint is negative, then skip it
        if (xjoints(index, 0) <= 0.0)
        {
            continue;
        }
        // Draw the keypoint (multiply x,y values by the sizes of the frame)
        center = cv::Point((xjoints(index, 0) * image_planes.cols), (xjoints(index, 1) * image_planes.rows));
        cv::ellipse(image_planes, center, axes, 0, 0, 360, color1, thickness, cv::LINE_4);
    }

    // Lastly, draw the connections between landmarks if there are any.
    connect_landmarks(image_planes, landmarks, xjoints, color2, thickness);
}

void connect_landmarks(cv::Mat &image_planes,
                       GstStructure *landmarks,
                       xt::xarray<float> &keypoints,
                       cv::Scalar &color, int &thickness)
{
    cv::Point joint1, joint2;
    gsize joint_pairing_size;
    // First get the size of the pairings, if this field is not set then there are no pairings
    bool pairings_available = gst_structure_has_field_typed(landmarks, "joint_pairs_size", G_TYPE_INT);
    if (!pairings_available)
    {
        // If there are no pairings, then skip drawing connections
        return;
    }
    (void)gst_structure_get(landmarks, "joint_pairs_size", G_TYPE_INT, &joint_pairing_size, NULL);
    const GValue *pairs_gvalue = gst_structure_get_value(landmarks, "joint_pairs"); // Get the g value from the gststructure
    GVariant *pairs_variant = g_value_get_variant(pairs_gvalue);                    // From the g value extract the variant
    // From the variant extract the uint8 array
    uint8_t *pairs_raw = (guint8 *)g_variant_get_fixed_array(pairs_variant, &joint_pairing_size, 1);

    // Adapt the variant array into an xtensor array
    size_t size = joint_pairing_size / 4; // There 4 bytes per element, so divide by 4
    size_t num_pairs = size / 2;          // each pair is two keypoints, so divide by 2
    std::vector<std::size_t> shape = {num_pairs, 2};
    xt::xarray<int> xpairs = xt::adapt((int *)pairs_raw, size, xt::no_ownership(), shape);

    // For each joint pair
    for (uint index = 0; index < num_pairs; index++)
    {
        // If both joints in the pair are present, draw a line connecting them
        if (keypoints(xpairs(index, 0), 0) > 0.0 && keypoints(xpairs(index, 1), 0) > 0.0)
        {
            joint1 = cv::Point(keypoints(xpairs(index, 0), 0) * image_planes.cols, keypoints(xpairs(index, 0), 1) * image_planes.rows);
            joint2 = cv::Point(keypoints(xpairs(index, 1), 0) * image_planes.cols, keypoints(xpairs(index, 1), 1) * image_planes.rows);
            cv::line(image_planes, joint1, joint2, color, thickness, cv::LINE_4);
        }
    }
}

void filter(HailoFramePtr hailo_frame)
{
    gint cv2_format;
    cv::Scalar color_blue, color_pink, color_green;
    cv::Point bbox_max, bbox_min;
    float x_format_factor;
    uint matrix_width;
    auto thickness = 2;

    switch (hailo_frame->format)
    {
    case GST_VIDEO_FORMAT_YUY2:
        cv2_format = CV_8UC4;
        x_format_factor = YUY2_FORMAT_FACTOR;
        color_blue = cv::Scalar(117, 61, 117, 44);
        color_green = cv::Scalar(202, 85, 202, 74);
        color_pink = cv::Scalar(61, 117, 117, 44);
        matrix_width = hailo_frame->width / 2;
        thickness = 1;
        break;
    case GST_VIDEO_FORMAT_RGB:
    default:
        cv2_format = CV_8UC3;
        x_format_factor = DEFAULT_FORMAT_FACTOR;
        color_blue = cv::Scalar(128, 255, 255);
        color_green = cv::Scalar(128, 255, 128);
        color_pink = cv::Scalar(255, 128, 255);
        matrix_width = hailo_frame->width;
        break;
    }

    auto image_planes = cv::Mat(hailo_frame->height, matrix_width, cv2_format,
                                hailo_frame->plane_data, hailo_frame->stride);
    auto detections = hailo_frame->get_detections();
    for (auto &detection : detections)
    {
        // Get the bbox
        auto bbox = detection->get();
        // Extract the xy min and max from the detection meta
        bbox_min = cv::Point(x_format_factor * bbox.xmin * hailo_frame->width, bbox.ymin * hailo_frame->height);
        bbox_max = cv::Point(x_format_factor * bbox.xmax * hailo_frame->width, bbox.ymax * hailo_frame->height);
        // Draw the detection box
        cv::rectangle(image_planes, bbox_min, bbox_max, color_blue, thickness, cv::LINE_4);
        auto bbox_width = bbox_max.x - bbox_min.x;
        auto bbox_height = bbox_max.y - bbox_min.y;

        // Adding label and confidence to the box only if the box is big enough.
        if (bbox_width > MINIMAL_BOX_WIDTH_FOR_TEXT)
        {
            // Calculating the font size according to the box width.
            float font_scale = TEXT_FONT_FACTOR * log(bbox_width);
            std::string text;
            std::string confidence = round_confidence(bbox.confidence);
            if (bbox.label != "")
            {
                text = std::string(bbox.label) + SPACE + confidence;
            }
            else
            {
                text = confidence;
            }
            // Get the confidence from the meta as well, round the number to significant figures
            auto text_position = cv::Point(bbox_min.x, bbox_min.y - DETECTION_TEXT_MARGIN * log(bbox_width));
            // Draw the class and confidence text
            cv::putText(image_planes, text, text_position, cv::FONT_HERSHEY_SIMPLEX, font_scale, color_blue, TEXT_THICKNESS);
        }

        // Extract landmarks, if any, and draw them
        auto landmarks = detection->get_landmarks();
        draw_landmarks(image_planes, landmarks, bbox, color_pink, color_green, thickness);

        // Extract segmentation data, if any, and draw it
        auto segmentation_data = detection->get_segmentation_data();
        draw_segmentation(image_planes, segmentation_data, hailo_frame->width, hailo_frame->height, bbox_width, bbox_height, bbox, cv2_format);
    }

    image_planes.release();
}