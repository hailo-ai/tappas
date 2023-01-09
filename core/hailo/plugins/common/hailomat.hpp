/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
/**
 * @file overlay/common.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-01-20
 *
 * @copyright Copyright (c) 2022
 *
 */

#pragma once

#include <opencv2/opencv.hpp>
#include "hailo_common.hpp"
#include "hailo_objects.hpp"
// Transformations were taken from https://stackoverflow.com/questions/17892346/how-to-convert-rgb-yuv-rgb-both-ways.
#define RGB2Y(R, G, B) CLIP((0.257 * (R) + 0.504 * (G) + 0.098 * (B)) + 16)
#define RGB2U(R, G, B) CLIP((-0.148 * (R)-0.291 * (G) + 0.439 * (B)) + 128)
#define RGB2V(R, G, B) CLIP((0.439 * (R)-0.368 * (G)-0.071 * (B)) + 128)

class HailoMat
{
protected:
    uint m_height;
    uint m_width;
    uint m_stride;
    int m_line_thickness;
    int m_font_thickness;
    cv::Rect get_bounding_rect(HailoBBox bbox, uint channel_width, uint channel_height)
    {
        cv::Rect rect;
        uint width = channel_width;
        uint height = channel_height;
        rect.x = CLAMP(bbox.xmin() * width, 0, width);
        rect.y = CLAMP(bbox.ymin() * height, 0, height);
        rect.width = CLAMP(bbox.width() * width, 0, width - rect.x);
        rect.height = CLAMP(bbox.height() * height, 0, height - rect.y);
        return rect;
    }

public:
    HailoMat(uint height, uint width, uint stride, int line_thickness = 1, int font_thickness = 1) : m_height(height),
                                                                                                     m_width(width),
                                                                                                     m_stride(stride),
                                                                                                     m_line_thickness(line_thickness),
                                                                                                     m_font_thickness(font_thickness){};
    HailoMat() : m_height(0), m_width(0), m_stride(0), m_line_thickness(0), m_font_thickness(0){};
    virtual ~HailoMat() = default;
    uint width() { return m_width; };
    uint height() { return m_height; };
    virtual cv::Mat &get_mat() = 0;
    virtual void draw_rectangle(cv::Rect rect, const cv::Scalar color) = 0;
    virtual void draw_text(std::string text, cv::Point position, double font_scale, const cv::Scalar color) = 0;
    virtual void draw_line(cv::Point point1, cv::Point point2, const cv::Scalar color, int thickness, int line_type) = 0;
    virtual void draw_ellipse(cv::Point center, cv::Size axes, double angle, double start_angle, double end_angle, const cv::Scalar color, int thickness) = 0;
    /*
     * @brief Crop ROIs from the mat, note the present implementation is valid
     *        for interlaced formats. Planar formats such as NV12 should override.
     * 
     * @param crop_roi 
     *        The roi to crop from this mat.
     * @return cv::Mat 
     *         The cropped mat.
     */
    virtual cv::Mat crop(HailoROIPtr crop_roi)
    {
        auto bbox = hailo_common::create_flattened_bbox(crop_roi->get_bbox(), crop_roi->get_scaling_bbox());
        cv::Rect rect = get_bounding_rect(bbox, m_width, m_height);
        cv::Mat cropped_cv_mat = get_mat()(rect);
        return cropped_cv_mat;
    }
};

class HailoRGBMat : public HailoMat
{
protected:
    cv::Mat m_mat;
    std::string m_name;

public:
    HailoRGBMat(uint8_t *buffer, uint height, uint width, uint stride, int line_thickness = 1, int font_thickness = 1, std::string name = "HailoRGBMat") : HailoMat(height, width, stride, line_thickness, font_thickness)
    {
        m_name = name;
        m_mat = cv::Mat(m_height, m_width, CV_8UC3, buffer, m_stride);
    };
    HailoRGBMat(cv::Mat mat, std::string name, int line_thickness = 1, int font_thickness = 1)
    {
        m_mat = mat;
        m_name = name;
        m_height = mat.rows;
        m_width = mat.cols;
        m_stride = mat.step;
        m_line_thickness = line_thickness;
        m_font_thickness = font_thickness;
    }
    virtual cv::Mat &get_mat()
    {
        return m_mat;
    }
    virtual std::string get_name() const
    {
        return m_name;
    }
    virtual void draw_rectangle(cv::Rect rect, const cv::Scalar color)
    {
        cv::rectangle(m_mat, rect, color, m_line_thickness);
    }
    virtual void draw_text(std::string text, cv::Point position, double font_scale, const cv::Scalar color)
    {
        cv::putText(m_mat, text, position, cv::FONT_HERSHEY_SIMPLEX, font_scale, color, m_font_thickness);
    }
    virtual void draw_line(cv::Point point1, cv::Point point2, const cv::Scalar color, int thickness, int line_type)
    {
        cv::line(m_mat, point1, point2, color, thickness, line_type);
    }
    virtual void draw_ellipse(cv::Point center, cv::Size axes, double angle, double start_angle, double end_angle, const cv::Scalar color, int thickness)
    {
        cv::ellipse(m_mat, center, axes, angle, start_angle, end_angle, color, thickness);
    }
    virtual ~HailoRGBMat()
    {
        m_mat.release();
    }
};

class HailoRGBAMat : public HailoMat
{
protected:
    cv::Mat m_mat;
    cv::Scalar get_rgba_color(cv::Scalar rgb_color, int alpha=1.0)
    {
        // setting default alpha as 1.0 as shown in an example: https://www.w3schools.com/css/css_colors_rgb.asp
        rgb_color[3] = alpha;
        return rgb_color;
    }

public:
    HailoRGBAMat(uint8_t *buffer, uint height, uint width, uint stride, int line_thickness = 1, int font_thickness = 1) : HailoMat(height, width, stride, line_thickness, font_thickness)
    {
        m_mat = cv::Mat(m_height, m_width, CV_8UC4, buffer, m_stride);
    };
    virtual cv::Mat &get_mat()
    {
        return m_mat;
    }
    virtual void draw_rectangle(cv::Rect rect, const cv::Scalar color)
    {
        cv::rectangle(m_mat, rect, get_rgba_color(color), m_line_thickness);
    }
    virtual void draw_text(std::string text, cv::Point position, double font_scale, const cv::Scalar color)
    {
        cv::putText(m_mat, text, position, cv::FONT_HERSHEY_SIMPLEX, font_scale, get_rgba_color(color), m_font_thickness);
    }
    virtual void draw_line(cv::Point point1, cv::Point point2, const cv::Scalar color, int thickness, int line_type)
    {
        cv::line(m_mat, point1, point2, get_rgba_color(color), thickness, line_type);
    }
    virtual void draw_ellipse(cv::Point center, cv::Size axes, double angle, double start_angle, double end_angle, const cv::Scalar color, int thickness)
    {
        cv::ellipse(m_mat, center, axes, angle, start_angle, end_angle, get_rgba_color(color), thickness);
    }
    virtual ~HailoRGBAMat()
    {
        m_mat.release();
    }
};

class HailoYUY2Mat : public HailoMat
{
protected:
    cv::Mat m_mat;
    cv::Scalar get_yuy2_color(cv::Scalar rgb_color)
    {
        uint r = rgb_color[0];
        uint g = rgb_color[1];
        uint b = rgb_color[2];
        uint y = RGB2Y(r, g, b);
        uint u = RGB2U(r, g, b);
        uint v = RGB2V(r, g, b);
        return cv::Scalar(y, u, y, v);
    }

public:
    HailoYUY2Mat(uint8_t *buffer, uint height, uint width, uint stride, int line_thickness = 1, int font_thickness = 1) : HailoMat(height, width, stride, line_thickness, font_thickness)
    {
        m_width = m_width / 2;
        m_mat = cv::Mat(m_height, m_width, CV_8UC4, buffer, m_stride);
    };
    virtual cv::Mat &get_mat()
    {
        return m_mat;
    }
    virtual void draw_rectangle(cv::Rect rect, const cv::Scalar color)
    {
        cv::rectangle(m_mat, rect, get_yuy2_color(color), m_line_thickness);
    }
    virtual void draw_text(std::string text, cv::Point position, double font_scale, const cv::Scalar color){};
    virtual void draw_line(cv::Point point1, cv::Point point2, const cv::Scalar color, int thickness, int line_type){};
    virtual void draw_ellipse(cv::Point center, cv::Size axes, double angle, double start_angle, double end_angle, const cv::Scalar color, int thickness){};
    virtual ~HailoYUY2Mat()
    {
        m_mat.release();
    }
};

class HailoNV12Mat : public HailoMat
{
/**
    NV12 Layout in memory (planar YUV 4:2:0):
 
    +-----+-----+-----+-----+-----+-----+
    | Y0  | Y1  | Y2  | Y3  | Y4  | Y5  |
    +-----+-----+-----+-----+-----+-----+
    | Y6  | ... | ... | ... | ... | ... |
    +-----+-----+-----+-----+-----+-----+
    | Y12 | ... | ... | ... | ... | ... |
    +-----+-----+-----+-----+-----+-----+
    | Y18 | ... | ... | ... | ... | ... |
    +-----+-----+-----+-----+-----+-----+
    | Y24 | ... | ... | ... | ... | ... |
    +-----+-----+-----+-----+-----+-----+
    | Y30 | Y31 | Y32 | Y33 | Y34 | Y35 |
    +-----+-----+-----+-----+-----+-----+
    | U0  | V0  | U1  | V1  | U2  | V2  |
    +-----+-----+-----+-----+-----+-----+
    | U3  | V3  | ... | ... | ... | ... |
    +-----+-----+-----+-----+-----+-----+
    | U6  | V6  | U7  | V7  | U8  | V8  |
    +-----+-----+-----+-----+-----+-----+
*/
protected:
    cv::Mat m_mat;
    cv::Scalar get_nv12_color(cv::Scalar rgb_color)
    {
        uint r = rgb_color[0];
        uint g = rgb_color[1];
        uint b = rgb_color[2];
        uint y = RGB2Y(r, g, b);
        uint u = RGB2U(r, g, b);
        uint v = RGB2V(r, g, b);
        return cv::Scalar(y, u, v);
    }

public:
    HailoNV12Mat(uint8_t *buffer, uint height, uint width, uint stride, int line_thickness = 1, int font_thickness = 1) : HailoMat(height, width, stride, line_thickness, font_thickness)
    {
        m_height = m_height * 3 / 2;
        m_mat = cv::Mat(m_height, m_width, CV_8UC1, buffer, m_stride);
    };
    virtual cv::Mat &get_mat()
    {
        return m_mat;
    }
    virtual void draw_rectangle(cv::Rect rect, const cv::Scalar color)
    {
        cv::Mat y_mat = cv::Mat(m_height * 2 / 3, m_width, CV_8UC1, (char *)m_mat.data, m_stride);
        cv::Mat uv_mat = cv::Mat(m_height / 3, m_width / 2, CV_8UC2, (char *)m_mat.data + ((m_height*2/3)*m_width), m_stride);
        cv::Scalar yuv_color = get_nv12_color(color);
        cv::Rect y_rect = cv::Rect(rect.x, rect.y  * 2 / 3, rect.width, rect.height * 2 / 3);
        cv::Rect uv_rect = cv::Rect(rect.x / 2, rect.y / 3, rect.width / 2, rect.height / 3);
        cv::rectangle(y_mat, y_rect, cv::Scalar(yuv_color[0]), m_line_thickness);
        cv::rectangle(uv_mat, uv_rect, cv::Scalar(yuv_color[1], yuv_color[2]), m_line_thickness);
    }
    virtual void draw_text(std::string text, cv::Point position, double font_scale, const cv::Scalar color)
    {
        cv::Mat y_mat = cv::Mat(m_height * 2 / 3, m_width, CV_8UC1, (char *)m_mat.data, m_stride);
        cv::Mat uv_mat = cv::Mat(m_height / 3, m_width / 2, CV_8UC2, (char *)m_mat.data + ((m_height*2/3)*m_width), m_stride);
        cv::Scalar yuv_color = get_nv12_color(color);
        cv::Point y_position = cv::Point(position.x, position.y  * 2 / 3);
        cv::Point uv_position = cv::Point(position.x / 2, position.y / 3);
        cv::putText(y_mat, text, y_position, cv::FONT_HERSHEY_SIMPLEX, font_scale, cv::Scalar(yuv_color[0]), m_font_thickness);
        cv::putText(uv_mat, text, uv_position, cv::FONT_HERSHEY_SIMPLEX, font_scale, cv::Scalar(yuv_color[1], yuv_color[2]), m_font_thickness);

    };
    virtual void draw_line(cv::Point point1, cv::Point point2, const cv::Scalar color, int thickness, int line_type)
    {
        cv::Mat y_mat = cv::Mat(m_height * 2 / 3, m_width, CV_8UC1, (char *)m_mat.data, m_stride);
        cv::Mat uv_mat = cv::Mat(m_height / 3, m_width / 2, CV_8UC2, (char *)m_mat.data + ((m_height*2/3)*m_width), m_stride);
        cv::Scalar yuv_color = get_nv12_color(color);

        cv::Point y_position1 = cv::Point(point1.x, point1.y  * 2 / 3);
        cv::Point uv_position1 = cv::Point(point1.x / 2, point1.y / 3);
        cv::Point y_position2 = cv::Point(point2.x, point2.y  * 2 / 3);
        cv::Point uv_position2 = cv::Point(point2.x / 2, point2.y / 3);

        cv::line(y_mat, y_position1, y_position2, cv::Scalar(yuv_color[0]), thickness, line_type);
        cv::line(uv_mat, uv_position1, uv_position2, cv::Scalar(yuv_color[1], yuv_color[2]), thickness, line_type);
    }
    virtual void draw_ellipse(cv::Point center, cv::Size axes, double angle, double start_angle, double end_angle, const cv::Scalar color, int thickness)
    {
        // Wrap the mat with Y and UV channel windows
        cv::Mat y_mat = cv::Mat(m_height * 2 / 3, m_width, CV_8UC1, (char *)m_mat.data, m_stride);
        cv::Mat uv_mat = cv::Mat(m_height / 3, m_width / 2, CV_8UC2, (char *)m_mat.data + ((m_height*2/3)*m_width), m_stride);
        cv::Scalar yuv_color = get_nv12_color(color);

        cv::Point y_position = cv::Point(center.x, center.y  * 2 / 3);
        cv::Point uv_position = cv::Point(center.x / 2, center.y / 3);

        cv::ellipse(y_mat, y_position, axes, angle, start_angle, end_angle, cv::Scalar(yuv_color[0]), thickness);
        cv::ellipse(uv_mat, uv_position, axes / 2, angle, start_angle, end_angle, cv::Scalar(yuv_color[1], yuv_color[2]), thickness);
    }
    virtual cv::Mat crop(HailoROIPtr crop_roi)
    {
        // Wrap the mat with Y and UV channel windows
        cv::Mat y_mat = cv::Mat(m_height * 2 / 3, m_width, CV_8UC1, (char *)m_mat.data, m_stride);
        cv::Mat uv_mat = cv::Mat(m_height / 3, m_width / 2, CV_8UC2, (char *)m_mat.data + ((m_height*2/3)*m_width), m_stride);
        auto bbox = hailo_common::create_flattened_bbox(crop_roi->get_bbox(), crop_roi->get_scaling_bbox());

        // The Y channel is packed before the U & V on it's own
        cv::Rect y_rect = get_bounding_rect(bbox, m_width, m_height * 2/3);

        // The U and V channels are interlaced together after the Y channel,
        // so they need to be cropped separately
        cv::Rect uv_rect = get_bounding_rect(bbox, m_width / 2, m_height / 3);

        // Prepare the cropped mat, and channel windows to fill with Y, U , & V
        cv::Mat cropped_mat = cv::Mat(y_rect.height + uv_rect.height, y_rect.width, CV_8UC1);
        cv::Mat cropped_y_mat = cv::Mat(y_rect.height, y_rect.width, CV_8UC1, (char *)cropped_mat.data, y_rect.width);
        cv::Mat cropped_uv_mat = cv::Mat(uv_rect.height, uv_rect.width, CV_8UC2, (char *)cropped_mat.data + (y_rect.height*y_rect.width), y_rect.width);

        // Fill the cropped mat with the cropped channels
        y_mat(y_rect).copyTo(cropped_y_mat);
        uv_mat(uv_rect).copyTo(cropped_uv_mat);
        return cropped_mat;
    }
    virtual ~HailoNV12Mat()
    {
        m_mat.release();
    }
};