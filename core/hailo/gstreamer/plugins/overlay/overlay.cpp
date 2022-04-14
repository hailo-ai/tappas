/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/**
 * @file overlay.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2022-01-20
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <opencv2/opencv.hpp>
#include <iostream>
#include "overlay.hpp"

#define SPACE " "
#define TEXT_CLS_FONT_SCALE (0.0025f)
#define TEXT_DEFAULT_HEIGHT (0.125f)
#define TEXT_FONT_FACTOR (0.12f)
#define MINIMAL_BOX_WIDTH_FOR_TEXT (10)
#define LANDMARKS_COLOR (cv::Scalar(255, 0, 0))
#define DEFAULT_DETECTION_COLOR (cv::Scalar(255, 255, 255))
#define DEFAULT_TILE_COLOR (cv::Scalar(0, 0, 255))

#define RGB2Y(R, G, B) CLIP((0.257 * (R) + 0.504 * (G) + 0.098 * (B)) + 16)
#define RGB2U(R, G, B) CLIP((-0.148 * (R) - 0.291 * (G) + 0.439 * (B)) + 128)
#define RGB2V(R, G, B) CLIP((0.439 * (R) - 0.368 * (G) - 0.071 * (B)) + 128)

static const std::vector<cv::Scalar> tile_layer_color_table = {
    cv::Scalar(0, 0, 255), cv::Scalar(200, 100, 120), cv::Scalar(255, 0, 0), cv::Scalar(120, 0, 0), cv::Scalar(0, 0, 120)
};

static const std::vector<cv::Scalar> color_table = {
    cv::Scalar(255, 0, 0), cv::Scalar(0, 255, 0), cv::Scalar(0, 0, 255), cv::Scalar(255, 255, 0), cv::Scalar(0, 255, 255),
    cv::Scalar(255, 0, 255), cv::Scalar(255, 170, 0), cv::Scalar(255, 0, 170), cv::Scalar(0, 255, 170), cv::Scalar(170, 255, 0),
    cv::Scalar(170, 0, 255), cv::Scalar(0, 170, 255), cv::Scalar(255, 85, 0), cv::Scalar(85, 255, 0), cv::Scalar(0, 255, 85),
    cv::Scalar(0, 85, 255), cv::Scalar(85, 0, 255), cv::Scalar(255, 0, 85), cv::Scalar(255, 255, 255)};

cv::Scalar RGB_TO_YUY2(cv::Scalar rgb)
{
    uint r = rgb[0];
    uint g = rgb[1];
    uint b = rgb[2];
    uint y = RGB2Y(r, g, b);
    uint u = RGB2U(r, g, b);
    uint v = RGB2V(r, g, b);
    return cv::Scalar(y, u, y, v);
}

cv::Scalar get_color(cv::Mat &mat, cv::Scalar color)
{
    if (mat.type() == CV_8UC4)
    {
        return RGB_TO_YUY2(color);
    }
    return color;
}

cv::Scalar indexToColor(size_t index)
{
    return color_table[index % color_table.size()];
}

float text_height = TEXT_DEFAULT_HEIGHT;

std::string confidence_to_string(float confidence)
{
    int confidence_percentage = (confidence * 100);

    return std::to_string(confidence_percentage) + "%";
}

static overlay_status_t draw_classification(cv::Mat &image_planes, HailoClassificationPtr result, HailoROIPtr roi, int font_thickness = 1)
{
    auto bbox = hailo_common::create_flattened_bbox(roi->get_bbox(), roi->get_scaling_bbox());
    int roi_xmin = bbox.xmin() * image_planes.cols;
    int roi_ymin = bbox.ymin() * image_planes.rows;
    int roi_width = image_planes.cols * bbox.width();
    int roi_height = image_planes.rows * bbox.height();
    auto text_position = cv::Point(roi_xmin, roi_ymin + (text_height * roi_height) + log(roi_height));
    
    std::string text = result->get_label() + SPACE + confidence_to_string(result->get_confidence());
    cv::putText(image_planes, text, text_position, cv::FONT_HERSHEY_SIMPLEX, TEXT_CLS_FONT_SCALE * roi_width,
                get_color(image_planes, DEFAULT_COLOR), font_thickness);
    text_height += TEXT_DEFAULT_HEIGHT;
    return OVERLAY_STATUS_OK;
}

static overlay_status_t draw_landmarks(cv::Mat &image_planes, HailoLandmarksPtr landmarks, HailoROIPtr roi)
{
    HailoBBox bbox = roi->get_bbox();
    for (auto &point : landmarks->get_points())
    {
        if (point.confidence() >= landmarks->get_threshold())
        {
            uint x = ((point.x() * bbox.width()) + bbox.xmin()) * image_planes.cols;
            uint y = ((point.y() * bbox.height()) + bbox.ymin()) * image_planes.rows;
            // Draw the keypoint (multiply x,y values by the sizes of the frame)
            auto center = cv::Point(x, y);
            cv::ellipse(image_planes, center, {1, 1}, 0, 0, 360,
                        get_color(image_planes, DEFAULT_COLOR));
        }
    }
    return OVERLAY_STATUS_OK;
}

static overlay_status_t draw_detection(cv::Mat &image_planes, NewHailoDetectionPtr detection, HailoROIPtr roi, int font_thickness = 1, int line_thickness = 1)
{
    HailoBBox roi_bbox = hailo_common::create_flattened_bbox(roi->get_bbox(), roi->get_scaling_bbox());
    auto detection_bbox = detection->get_bbox();

    auto bbox_min = cv::Point(((detection_bbox.xmin() * roi_bbox.width()) + roi_bbox.xmin()) * image_planes.cols,
                              ((detection_bbox.ymin() * roi_bbox.height()) + roi_bbox.ymin()) * image_planes.rows);
    auto bbox_max = cv::Point(((detection_bbox.xmax() * roi_bbox.width()) + roi_bbox.xmin()) * image_planes.cols,
                              ((detection_bbox.ymax() * roi_bbox.height()) + roi_bbox.ymin()) * image_planes.rows);

    // Draw the detection box
    auto color_rgb = (detection->get_class_id() == NULL_CLASS_ID) ? DEFAULT_DETECTION_COLOR : indexToColor(detection->get_class_id());
    auto color = get_color(image_planes, color_rgb);
    cv::rectangle(image_planes, bbox_min, bbox_max, color, line_thickness);
    auto bbox_width = bbox_max.x - bbox_min.x;
    // Adding label and confidence to the box only if the box is big enough.
    if (bbox_width > MINIMAL_BOX_WIDTH_FOR_TEXT)
    {
        // Calculating the font size according to the box width.
        float font_scale = TEXT_FONT_FACTOR * log(bbox_width);
        std::string text;
        std::string confidence = confidence_to_string(detection->get_confidence());
        std::string label = detection->get_label(); 
        if (label != "")
        {
            text = label + SPACE + confidence + SPACE;
        }
        else
        {
            text = confidence + SPACE;
        }
        // Get the confidence from the meta as well, round the number to significant figures
        auto text_position = cv::Point(bbox_min.x - log(bbox_width), bbox_min.y - log(bbox_width));
        // Draw the class and confidence text
        cv::putText(image_planes, text, text_position, cv::FONT_HERSHEY_SIMPLEX, font_scale, color, font_thickness);
    }
    draw_all(image_planes, detection, font_thickness, line_thickness);

    return OVERLAY_STATUS_OK;
}

static overlay_status_t draw_tile(cv::Mat &image_planes, HailoTileROIPtr tile, int font_thickness = 1, int line_thickness = 1)
{
    auto bbox = tile->get_bbox();
    auto bbox_min = cv::Point(bbox.xmin() * image_planes.cols, bbox.ymin() * image_planes.rows);
    auto bbox_max = cv::Point(bbox.xmax() * image_planes.cols, bbox.ymax() * image_planes.rows);

    cv::Scalar color;
    uint tile_layer = tile->get_layer();
    if (tile_layer < tile_layer_color_table.size())
        color = get_color(image_planes, tile_layer_color_table[tile_layer % tile_layer_color_table.size()]);
    else
        color = get_color(image_planes, DEFAULT_TILE_COLOR);

    // Draw the tile box
    cv::rectangle(image_planes, bbox_min, bbox_max, color);
    draw_all(image_planes, tile, font_thickness, line_thickness);

    return OVERLAY_STATUS_OK;
}

static overlay_status_t draw_id(cv::Mat &image_planes, HailoUniqueIDPtr &hailo_id, HailoROIPtr roi, int font_thickness = 1)
{
    NewHailoDetectionPtr detection = std::dynamic_pointer_cast<NewHailoDetection>(roi);
    std::string id_text = std::to_string(hailo_id->get_id());

    auto bbox = detection->get_bbox();
    auto bbox_min = cv::Point(bbox.xmin() * image_planes.cols, bbox.ymin() * image_planes.rows);
    auto bbox_max = cv::Point(bbox.xmax() * image_planes.cols, bbox.ymax() * image_planes.rows);
    auto bbox_width = bbox_max.x - bbox_min.x;
    auto color = (detection->get_class_id() == NULL_CLASS_ID) ? DEFAULT_DETECTION_COLOR : indexToColor(detection->get_class_id());

    if (bbox_width > MINIMAL_BOX_WIDTH_FOR_TEXT)
    {
        // Calculating the font size according to the box width.
        float font_scale = TEXT_FONT_FACTOR * log(bbox_width);
        auto text_position = cv::Point(bbox_min.x + log(bbox_width), bbox_max.y - log(bbox_width));
        // Draw the class and confidence text
        cv::putText(image_planes, id_text, text_position, cv::FONT_HERSHEY_SIMPLEX, font_scale, color, font_thickness);
    }
    return OVERLAY_STATUS_OK;
}

overlay_status_t draw_all(cv::Mat &mat, HailoROIPtr roi, int font_thickness, int line_thickness)
{
    overlay_status_t ret = OVERLAY_STATUS_UNINITIALIZED;
    text_height = TEXT_DEFAULT_HEIGHT;
    for (auto obj : roi->get_objects())
    {

        switch (obj->get_type())
        {
        case HAILO_DETECTION:
        {
            NewHailoDetectionPtr detection = std::dynamic_pointer_cast<NewHailoDetection>(obj);
            draw_detection(mat, detection, roi, font_thickness, line_thickness);
            break;
        }
        case HAILO_CLASSIFICATION:
        {
            HailoClassificationPtr classification = std::dynamic_pointer_cast<HailoClassification>(obj);
            draw_classification(mat, classification, roi, font_thickness);
            break;
        }
        case HAILO_LANDMARKS:
        {
            HailoLandmarksPtr landmarks = std::dynamic_pointer_cast<HailoLandmarks>(obj);
            draw_landmarks(mat, landmarks, roi);
            break;
        }
        case HAILO_TILE:
        {
            HailoTileROIPtr tile = std::dynamic_pointer_cast<HailoTileROI>(obj);
            draw_tile(mat, tile, font_thickness, line_thickness);
            break;
        }
        case HAILO_UNIQUE_ID:
        {
            HailoUniqueIDPtr id = std::dynamic_pointer_cast<HailoUniqueID>(obj);
            draw_id(mat, id, roi, font_thickness);
            break;
        }
        default:
            // continue
            break;
        }
    }
    ret = OVERLAY_STATUS_OK;
    return ret;
}