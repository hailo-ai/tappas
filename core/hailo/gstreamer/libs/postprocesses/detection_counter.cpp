/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include <gst/video/video.h>
#include <iostream>
#include <map>
#include <queue>
#include <opencv2/opencv.hpp>
#include <typeinfo>
#include <cmath>
#include <sstream>

#include "detection_counter.hpp"
#include "hailo_detection.hpp"

#define TEXT_FACTOR (10)
#define YUY2_FORMAT_FACTOR (0.5)
#define TEXT_FONT_SCALE (0.7)
#define NUM_FONT_SCALE (0.8)

#define BBOX_THICKNESS (1)
#define LEFT_LINE_X (YUY2_FORMAT_FACTOR * (0.5 * 1280 - AVERAGE_HALF_BOX_WIDTH))
#define RIGHT_LINE_X (YUY2_FORMAT_FACTOR * (0.5 * 1280 + AVERAGE_HALF_BOX_WIDTH))
#define AVERAGE_HALF_BOX_WIDTH (75)
#define FONT (cv::FONT_HERSHEY_SIMPLEX)
#define TEXT_THICKNESS (2)
#define NUM_SEGMENTS (12)

int accumulative_counter[4] = {0};
int y_stage[4][12] = {0};
cv::Point bbox_max, bbox_min;
int current_frame_counter[4] = {0};
int size_average = 0;
float accumulative_size_average = 0;
float accumulative_square_size_average = 0;
float current_size_std = 0;
int y_index;
cv::Mat image_planes;
gint cv2_format = CV_8UC4;
cv::Scalar color_light_blue = cv::Scalar(141, 192, 141, 27);
cv::Scalar color_dark_blue = cv::Scalar(41, 162, 41, 98);
cv::Scalar color_white = cv::Scalar(255, 128, 255, 128);
cv::Scalar color_orange = cv::Scalar(171, 59, 171, 187);
cv::Scalar color_red = cv::Scalar(76, 84, 76, 255);
cv::Scalar color_yellow = cv::Scalar(255, 0, 255, 148);

static std::map<guint8, cv::Scalar> peppers_colors = {
    {0, color_white}, {1, color_orange}, {2, color_red}, {3, color_yellow}};

void draw_vertical_line(HailoFramePtr hailo_frame, cv::Mat image_planes)
{
    cv::Point left_line_top = cv::Point(LEFT_LINE_X, 0);
    cv::Point left_line_buttom = cv::Point(LEFT_LINE_X, 720);
    cv::line(image_planes, left_line_top, left_line_buttom, color_white, 2, cv::LINE_4);
}

void debug_draw(HailoFramePtr hailo_frame, cv::Mat image_planes, int class_id)
{
    for (int i = 0; i < NUM_SEGMENTS; i++)
    {
        cv::Point left = cv::Point(0, i * (hailo_frame->height) / NUM_SEGMENTS);
        cv::Point right = cv::Point(YUY2_FORMAT_FACTOR * hailo_frame->width, i * (hailo_frame->height) / NUM_SEGMENTS);
        cv::line(image_planes, right, left, cv::Scalar(255, 128, 255, 128), 1, cv::LINE_4);
        auto text_position = cv::Point(YUY2_FORMAT_FACTOR *
                                           (0.5 * 1300),
                                       (i + 1) * (hailo_frame->height) / NUM_SEGMENTS);
        std::string text = std::to_string(y_stage[3][i]);
        cv::putText(image_planes, text, text_position, FONT, TEXT_FONT_SCALE, color_white, TEXT_THICKNESS);
    }

    cv::Point right_line_top = cv::Point(RIGHT_LINE_X, 0);
    cv::Point right_line_buttom = cv::Point(RIGHT_LINE_X, 720);
    cv::line(image_planes, right_line_top, right_line_buttom, color_white, 2, cv::LINE_4);
}

void draw_bbox(cv::Mat image_planes, cv::Point bbox_min, cv::Point bbox_max, int class_id, std::string label)
{
    auto color = peppers_colors[class_id];
    cv::rectangle(image_planes, bbox_min, bbox_max, color, BBOX_THICKNESS, cv::LINE_4);
    std::string text_object = label;
    auto text_position_class = cv::Point(bbox_min.x - TEXT_FACTOR, bbox_min.y - TEXT_FACTOR);
    cv::putText(image_planes, text_object, text_position_class, FONT, TEXT_FONT_SCALE,
                color, BBOX_THICKNESS);
}

void draw_counters(cv::Mat image_planes, int offset, int counters_array[4])
{
    std::string orange_count = std::to_string(counters_array[1]);
    cv::putText(image_planes, "Orange", cv::Point(YUY2_FORMAT_FACTOR * 1300, offset + 50), FONT, TEXT_FONT_SCALE,
                color_dark_blue, TEXT_THICKNESS);
    cv::putText(image_planes, orange_count, cv::Point(YUY2_FORMAT_FACTOR * 1480, offset + 50), FONT, NUM_FONT_SCALE,
                color_light_blue, TEXT_THICKNESS);
    std::string red_count = std::to_string(counters_array[2]);
    cv::putText(image_planes, "Red", cv::Point(YUY2_FORMAT_FACTOR * 1300, offset + 120), FONT, TEXT_FONT_SCALE,
                color_dark_blue, TEXT_THICKNESS);
    cv::putText(image_planes, red_count, cv::Point(YUY2_FORMAT_FACTOR * 1480, offset + 120), FONT, NUM_FONT_SCALE,
                color_light_blue, TEXT_THICKNESS);
    std::string yellow_count = std::to_string(counters_array[3]);
    cv::putText(image_planes, "Yellow", cv::Point(YUY2_FORMAT_FACTOR * 1300, offset + 190), FONT, TEXT_FONT_SCALE,
                color_dark_blue, TEXT_THICKNESS);
    cv::putText(image_planes, yellow_count, cv::Point(YUY2_FORMAT_FACTOR * 1480, offset + 190), FONT, NUM_FONT_SCALE,
                color_light_blue, TEXT_THICKNESS);
}
void draw_stats_block(cv::Mat image_planes)
{
    int y_offset1 = 70;
    int x_offset = 1290;
    cv::putText(image_planes, "Accumulated", cv::Point(YUY2_FORMAT_FACTOR * (x_offset + 30), y_offset1),
                FONT, TEXT_FONT_SCALE, color_dark_blue, TEXT_THICKNESS);
    cv::putText(image_planes, "Counter", cv::Point(YUY2_FORMAT_FACTOR * (x_offset + 30), y_offset1 + 30),
                FONT, TEXT_FONT_SCALE, color_dark_blue, TEXT_THICKNESS);
    int y_offset2 = 400;
    cv::putText(image_planes, "Objects in", cv::Point(YUY2_FORMAT_FACTOR * (x_offset + 30), y_offset2), FONT, TEXT_FONT_SCALE,
                color_dark_blue, TEXT_THICKNESS);
    cv::putText(image_planes, "Current Frame", cv::Point(YUY2_FORMAT_FACTOR * (x_offset + 30), y_offset2 + 30), FONT, TEXT_FONT_SCALE,
                color_dark_blue, TEXT_THICKNESS);
    draw_counters(image_planes, y_offset1 + 30, accumulative_counter);
    draw_counters(image_planes, y_offset2 + 30, current_frame_counter);

    cv::putText(image_planes, "Model: YOLOv5", cv::Point(YUY2_FORMAT_FACTOR * 30, 775), FONT, 0.8, color_dark_blue,
                TEXT_THICKNESS);
    cv::putText(image_planes, "Resolution: 640x640", cv::Point(YUY2_FORMAT_FACTOR * 500, 775), FONT, 0.8,
                color_dark_blue, TEXT_THICKNESS);
    cv::putText(image_planes, "Power: 2.25 W", cv::Point(YUY2_FORMAT_FACTOR * 30, 825), FONT, 0.8, color_dark_blue,
                TEXT_THICKNESS);
    cv::putText(image_planes, "Latency: 6.28 ms", cv::Point(YUY2_FORMAT_FACTOR * 500, 825), FONT, 0.8, color_dark_blue,
                TEXT_THICKNESS);
}

float calculate_box_size(cv::Point bbox_min, cv::Point bbox_max)
{
    return (1 / YUY2_FORMAT_FACTOR) * (bbox_max.x - bbox_min.x) * (bbox_max.y - bbox_min.y);
}

void update_size_average(cv::Point bbox_min, cv::Point bbox_max)
{
    float size = calculate_box_size(bbox_min, bbox_max);
    int total_count = accumulative_counter[1] + accumulative_counter[2] + accumulative_counter[3];
    accumulative_size_average = (accumulative_size_average * (total_count - 1) + size) / (total_count);
}

void update_square_size_average(cv::Point bbox_min, cv::Point bbox_max)
{
    float size = calculate_box_size(bbox_min, bbox_max);
    int total_count = accumulative_counter[1] + accumulative_counter[2] + accumulative_counter[3];
    accumulative_square_size_average = (accumulative_square_size_average *
                                            (total_count - 1) +
                                        size * size) /
                                       (total_count);
}

void update_size_std(cv::Point bbox_min, cv::Point bbox_max)
{
    update_size_average(bbox_min, bbox_max);
    update_square_size_average(bbox_min, bbox_max);
    current_size_std = sqrt(accumulative_square_size_average - accumulative_size_average * accumulative_size_average);
}

void filter(HailoFramePtr hailo_frame)
{
    auto image_planes = cv::Mat(hailo_frame->height, hailo_frame->width / 2, cv2_format,
                                hailo_frame->plane_data, hailo_frame->stride);

    draw_vertical_line(hailo_frame, image_planes);
    auto detections = hailo_frame->get_detections();

    for (auto &detection : detections)
    {
        auto bbox = detection->get();

        bbox_min = cv::Point(YUY2_FORMAT_FACTOR * bbox.xmin * 1280, bbox.ymin * 720);
        bbox_max = cv::Point(YUY2_FORMAT_FACTOR * bbox.xmax * 1280, bbox.ymax * 720);

        y_index = (0.5 * (bbox_max.y + bbox_min.y)) / (hailo_frame->height / NUM_SEGMENTS);

        // Box is on the right line.
        if ((bbox_max.x > RIGHT_LINE_X) && (bbox_min.x < RIGHT_LINE_X))
        {
            y_stage[bbox.class_id][y_index] = 1;
        }

        // Box is between the lines.
        if ((bbox_max.x < RIGHT_LINE_X) && (bbox_min.x > LEFT_LINE_X))
        {
            if (y_stage[bbox.class_id][y_index] == 1)
                y_stage[bbox.class_id][y_index] = 2;
        }

        // Box is on the left line.
        if ((bbox_max.x > LEFT_LINE_X) && (bbox_min.x < LEFT_LINE_X))
        {
            if (y_stage[bbox.class_id][y_index] == 2)
            {
                if (accumulative_counter[bbox.class_id] == 999999)
                    accumulative_counter[bbox.class_id] = 0;
                accumulative_counter[bbox.class_id]++;
                y_stage[bbox.class_id][y_index] = 0;
                update_size_std(bbox_min, bbox_max);
            }
        }

        draw_bbox(image_planes, bbox_min, bbox_max, bbox.class_id, bbox.label);
        current_frame_counter[bbox.class_id]++;
    }

    draw_stats_block(image_planes);
    current_frame_counter[1] = 0;
    current_frame_counter[2] = 0;
    current_frame_counter[3] = 0;
}