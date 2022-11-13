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

#include <gst/video/video.h>
#include <opencv2/opencv.hpp>
#include "hailo_objects.hpp"
// Transformations were taken from https://stackoverflow.com/questions/17892346/how-to-convert-rgb-yuv-rgb-both-ways.
#define RGB2Y(R, G, B) CLIP((0.257 * (R) + 0.504 * (G) + 0.098 * (B)) + 16)
#define RGB2U(R, G, B) CLIP((-0.148 * (R)-0.291 * (G) + 0.439 * (B)) + 128)
#define RGB2V(R, G, B) CLIP((0.439 * (R)-0.368 * (G)-0.071 * (B)) + 128)

__BEGIN_DECLS

/**
 * @brief Get the size of buffer with specific caps
 *
 * @param caps The caps to extract size from.
 * @return size_t The size of a buffer with those caps.
 */
size_t get_size(GstCaps *caps);

/**
 * @brief Get the mat object
 *
 * @param info The GstVideoInfo to extract mat from.
 * @param map The GstMapInfo to extract mat from.
 * @return cv::Mat The mat object.
 */
cv::Mat get_mat(GstVideoInfo *info, GstMapInfo *map);

/**
 * @brief Resizes a YUY2 image (4 channel cv::Mat)
 *
 * @param cropped_image - cv::Mat &
 *        The cropped image to resize
 *
 * @param resized_image - cv::Mat &
 *        The resized image container to fill
 *        (dims for resizing are assumed from here)
 *
 * @param interpolation - int
 *        The interpolation type to resize by.
 *        Must be a supported opencv type
 *        (bilinear, nearest neighbors, etc...)
 */
void resize_yuy2(cv::Mat &cropped_image, cv::Mat &resized_image, int interpolation = cv::INTER_LINEAR);
__END_DECLS

class HailoMat
{
protected:
    uint m_height;
    uint m_width;
    uint m_stride;
    int m_line_thickness;
    int m_font_thickness;

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
};

class HailoRGBMat : public HailoMat
{
protected:
    cv::Mat m_mat;

public:
    HailoRGBMat(uint8_t *buffer, uint height, uint width, uint stride, int line_thickness = 1, int font_thickness = 1) : HailoMat(height, width, stride, line_thickness, font_thickness)
    {
        m_mat = cv::Mat(m_height, m_width, CV_8UC3, buffer, m_stride);
    };
    virtual cv::Mat &get_mat()
    {
        return m_mat;
    }
    virtual void draw_rectangle(cv::Rect rect, const cv::Scalar color)
    {
        cv::rectangle(m_mat, rect, color, m_line_thickness);
    }
    virtual void draw_text(std::string text, cv::Point position, double font_scale, const cv::Scalar color)
    {
        cv::putText(m_mat, text, position, cv::FONT_HERSHEY_SIMPLEX, font_scale, color, m_font_thickness);
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
    cv::Scalar get_rgba_color(cv::Scalar rgb_color, int alpha=255)
    {
        return rgb_color + cv::Scalar(alpha);
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
    virtual ~HailoYUY2Mat() 
    {
        m_mat.release();
    }
};
std::shared_ptr<HailoMat> get_mat_by_format(GstVideoInfo *info, GstMapInfo *map, int line_thickness=1, int font_thickness=1);
