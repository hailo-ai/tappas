/**
 * Copyright (c) 2021-2026 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include "hailomat_internal.hpp"

//=============================================================================
// Conversion helpers (internal use only)
//=============================================================================

static inline cv::Point to_cv_point(const hailo_point_t& p) {
    return cv::Point(p.x, p.y);
}

static inline cv::Rect to_cv_rect(const hailo_rect_t& r) {
    return cv::Rect(r.x, r.y, r.width, r.height);
}

static inline cv::Size to_cv_size(const hailo_size_t& s) {
    return cv::Size(s.width, s.height);
}

static inline cv::Scalar to_cv_scalar(const hailo_scalar_t& s) {
    return cv::Scalar(s.val[0], s.val[1], s.val[2], s.val[3]);
}

static inline hailo_rect_t from_cv_rect(const cv::Rect& r) {
    return hailo_rect_t(r.x, r.y, r.width, r.height);
}

//=============================================================================
// Utility functions
//=============================================================================

hailo_line_orientation_t line_orientation(hailo_point_t point1, hailo_point_t point2)
{
    if (point1.x == point2.x)
        return HAILO_LINE_VERTICAL;
    else if (point1.y == point2.y)
        return HAILO_LINE_HORIZONTAL;
    else if (point1.x < point2.x && point1.y < point2.y)
        return HAILO_LINE_DIAGONAL;
    else if (point1.x < point2.x && point1.y > point2.y)
        return HAILO_LINE_ANTI_DIAGONAL;
    else
        return HAILO_LINE_NONE;
}

int floor_to_even_number(int x)
{
    /*
    The expression x &~1 in C++ performs a bitwise AND operation between the number x and the number ~1(bitwise negation of 1).
    In binary representation, the number 1 is represented as 0000 0001, and its negation, ~1, is equal to 1111 1110.
    The bitwise AND operation between x and ~1 zeros out the least significant bit of x,
    effectively rounding it down to the nearest even number.
    This is because any odd number in binary representation will have its least significant bit set to 1,
    and ANDing it with ~1 will zero out that bit.
    */
    return x & ~1;
}

//=============================================================================
// HailoMat implementation
//=============================================================================

HailoMat::HailoMat(std::uint32_t height, std::uint32_t width, std::uint32_t stride, int line_thickness, int font_thickness)
    : m_height(height),
      m_width(width),
      m_native_height(height),
      m_native_width(width),
      m_stride(stride),
      m_line_thickness(line_thickness),
      m_font_thickness(font_thickness),
      m_impl(std::make_unique<HailoMatImpl>())
{
}

HailoMat::HailoMat()
    : m_height(0), m_width(0), m_native_height(0), m_native_width(0),
      m_stride(0), m_line_thickness(0), m_font_thickness(0),
      m_impl(std::make_unique<HailoMatImpl>())
{
}

HailoMat::~HailoMat() = default;

HailoMat::HailoMat(HailoMat&&) noexcept = default;
HailoMat& HailoMat::operator=(HailoMat&&) noexcept = default;

std::uint32_t HailoMat::width() { return m_width; }
std::uint32_t HailoMat::height() { return m_height; }
std::uint32_t HailoMat::native_width() { return m_native_width; }
std::uint32_t HailoMat::native_height() { return m_native_height; }

HailoMatImpl* HailoMat::get_impl() { return m_impl.get(); }
const HailoMatImpl* HailoMat::get_impl() const { return m_impl.get(); }

hailo_rect_t HailoMat::get_bounding_rect(HailoBBox bbox, std::uint32_t channel_width, std::uint32_t channel_height)
{
    hailo_rect_t rect;
    std::uint32_t width = channel_width;
    std::uint32_t height = channel_height;
    rect.x = CLAMP(bbox.xmin() * width, 0, width);
    rect.y = CLAMP(bbox.ymin() * height, 0, height);
    rect.width = CLAMP(bbox.width() * width, 0, width - rect.x);
    rect.height = CLAMP(bbox.height() * height, 0, height - rect.y);
    return rect;
}

std::unique_ptr<HailoMatImpl> HailoMat::crop(HailoROIPtr crop_roi)
{
    hailo_rect_t rect = get_crop_rect(crop_roi);
    cv::Rect cv_rect = to_cv_rect(rect);
    cv::Mat cropped_cv_mat = m_impl->get(0)(cv_rect);
    
    auto result = std::make_unique<HailoMatImpl>();
    result->push(std::move(cropped_cv_mat));
    return result;
}

hailo_rect_t HailoMat::get_crop_rect(HailoROIPtr crop_roi)
{
    auto bbox = hailo_common::create_flattened_bbox(crop_roi->get_bbox(), crop_roi->get_scaling_bbox());
    hailo_rect_t rect = get_bounding_rect(bbox, m_width, m_height);
    return rect;
}

//=============================================================================
// HailoRGBMat implementation
//=============================================================================

HailoRGBMat::HailoRGBMat(uint8_t *buffer, std::uint32_t height, std::uint32_t width, std::uint32_t stride, int line_thickness, int font_thickness, std::string name) 
    : HailoMat(height, width, stride, line_thickness, font_thickness), m_name(name)
{
    cv::Mat mat = cv::Mat(m_height, m_width, CV_8UC3, buffer, m_stride);
    m_impl->push(mat);
}

hailo_mat_t HailoRGBMat::get_type()
{
    return HAILO_MAT_RGB;
}

std::string HailoRGBMat::get_name() const
{
    return m_name;
}

void HailoRGBMat::draw_rectangle(hailo_rect_t rect, const hailo_scalar_t color)
{
    cv::rectangle(m_impl->get(0), to_cv_rect(rect), to_cv_scalar(color), m_line_thickness);
}

void HailoRGBMat::draw_text(std::string text, hailo_point_t position, double font_scale, const hailo_scalar_t color)
{
    cv::putText(m_impl->get(0), text, to_cv_point(position), cv::FONT_HERSHEY_SIMPLEX, font_scale, to_cv_scalar(color), m_font_thickness);
}

void HailoRGBMat::draw_line(hailo_point_t point1, hailo_point_t point2, const hailo_scalar_t color, int thickness, int line_type)
{
    cv::line(m_impl->get(0), to_cv_point(point1), to_cv_point(point2), to_cv_scalar(color), thickness, line_type);
}

void HailoRGBMat::draw_ellipse(hailo_point_t center, hailo_size_t axes, double angle, double start_angle, double end_angle, const hailo_scalar_t color, int thickness)
{
    cv::ellipse(m_impl->get(0), to_cv_point(center), to_cv_size(axes), angle, start_angle, end_angle, to_cv_scalar(color), thickness);
}

void HailoRGBMat::blur(hailo_rect_t rect, hailo_size_t ksize)
{
    cv::Mat target_roi = m_impl->get(0)(to_cv_rect(rect));
    cv::blur(target_roi, target_roi, to_cv_size(ksize));
}

HailoRGBMat::~HailoRGBMat() = default;

//=============================================================================
// HailoRGBAMat implementation
//=============================================================================

static cv::Scalar get_rgba_color(const hailo_scalar_t& rgb_color, int alpha = 1)
{
    cv::Scalar color = to_cv_scalar(rgb_color);
    color[3] = alpha;
    return color;
}

HailoRGBAMat::HailoRGBAMat(uint8_t *buffer, std::uint32_t height, std::uint32_t width, std::uint32_t stride, int line_thickness, int font_thickness) 
    : HailoMat(height, width, stride, line_thickness, font_thickness)
{
    cv::Mat mat = cv::Mat(m_height, m_width, CV_8UC4, buffer, m_stride);
    m_impl->push(mat);
}

hailo_mat_t HailoRGBAMat::get_type()
{
    return HAILO_MAT_RGBA;
}

void HailoRGBAMat::draw_rectangle(hailo_rect_t rect, const hailo_scalar_t color)
{
    cv::rectangle(m_impl->get(0), to_cv_rect(rect), get_rgba_color(color), m_line_thickness);
}

void HailoRGBAMat::draw_text(std::string text, hailo_point_t position, double font_scale, const hailo_scalar_t color)
{
    cv::putText(m_impl->get(0), text, to_cv_point(position), cv::FONT_HERSHEY_SIMPLEX, font_scale, get_rgba_color(color), m_font_thickness);
}

void HailoRGBAMat::draw_line(hailo_point_t point1, hailo_point_t point2, const hailo_scalar_t color, int thickness, int line_type)
{
    cv::line(m_impl->get(0), to_cv_point(point1), to_cv_point(point2), get_rgba_color(color), thickness, line_type);
}

void HailoRGBAMat::draw_ellipse(hailo_point_t center, hailo_size_t axes, double angle, double start_angle, double end_angle, const hailo_scalar_t color, int thickness)
{
    cv::ellipse(m_impl->get(0), to_cv_point(center), to_cv_size(axes), angle, start_angle, end_angle, get_rgba_color(color), thickness);
}

void HailoRGBAMat::blur(hailo_rect_t rect, hailo_size_t ksize)
{
    cv::Mat target_roi = m_impl->get(0)(to_cv_rect(rect));
    cv::blur(target_roi, target_roi, to_cv_size(ksize));
}

HailoRGBAMat::~HailoRGBAMat() = default;

//=============================================================================
// HailoYUY2Mat implementation
//=============================================================================

static cv::Scalar get_yuy2_color(const hailo_scalar_t& rgb_color)
{
    std::uint32_t r = rgb_color[0];
    std::uint32_t g = rgb_color[1];
    std::uint32_t b = rgb_color[2];
    std::uint32_t y = RGB2Y(r, g, b);
    std::uint32_t u = RGB2U(r, g, b);
    std::uint32_t v = RGB2V(r, g, b);
    return cv::Scalar(y, u, y, v);
}

HailoYUY2Mat::HailoYUY2Mat(uint8_t *buffer, std::uint32_t height, std::uint32_t width, std::uint32_t stride, int line_thickness, int font_thickness) 
    : HailoMat(height, width, stride, line_thickness, font_thickness)
{
    m_width = m_width / 2;
    cv::Mat mat = cv::Mat(m_height, m_width, CV_8UC4, buffer, m_stride);
    m_impl->push(mat);
}

hailo_mat_t HailoYUY2Mat::get_type()
{
    return HAILO_MAT_YUY2;
}

void HailoYUY2Mat::draw_rectangle(hailo_rect_t rect, const hailo_scalar_t color)
{
    cv::Rect fixed_rect = cv::Rect(rect.x / 2, rect.y, rect.width / 2, rect.height);
    cv::rectangle(m_impl->get(0), fixed_rect, get_yuy2_color(color), m_line_thickness);
}

void HailoYUY2Mat::draw_text(std::string text, hailo_point_t position, double font_scale, const hailo_scalar_t color)
{
    // Empty implementation - YUY2 text drawing not supported
}

void HailoYUY2Mat::draw_line(hailo_point_t point1, hailo_point_t point2, const hailo_scalar_t color, int thickness, int line_type)
{
    // Empty implementation - YUY2 line drawing not supported
}

void HailoYUY2Mat::draw_ellipse(hailo_point_t center, hailo_size_t axes, double angle, double start_angle, double end_angle, const hailo_scalar_t color, int thickness)
{
    // Empty implementation - YUY2 ellipse drawing not supported
}

void HailoYUY2Mat::blur(hailo_rect_t rect, hailo_size_t ksize)
{
    // Empty implementation - YUY2 blur not supported
}

HailoYUY2Mat::~HailoYUY2Mat() = default;

//=============================================================================
// HailoNV12Mat implementation
//=============================================================================

static cv::Scalar get_nv12_color(const hailo_scalar_t& rgb_color)
{
    std::uint32_t r = rgb_color[0];
    std::uint32_t g = rgb_color[1];
    std::uint32_t b = rgb_color[2];
    std::uint32_t y = RGB2Y(r, g, b);
    std::uint32_t u = RGB2U(r, g, b);
    std::uint32_t v = RGB2V(r, g, b);
    return cv::Scalar(y, u, v);
}

HailoNV12Mat::HailoNV12Mat(uint8_t *buffer, std::uint32_t height, std::uint32_t width, std::uint32_t y_plane_stride, std::uint32_t uv_plane_stride, int line_thickness, int font_thickness, void *plane0, void *plane1) 
    : HailoMat(height, width, y_plane_stride, line_thickness, font_thickness)
{
    m_height = (m_height * 3 / 2);
    m_y_stride = y_plane_stride;
    m_uv_stride = uv_plane_stride;

    if (plane0 == nullptr)
        plane0 = (char *)buffer;
    if (plane1 == nullptr)
    {
        plane1 = (char *)buffer + ((m_native_height) * m_uv_stride);
    }
    cv::Mat y_plane_mat = cv::Mat(m_native_height, m_width, CV_8UC1, plane0, y_plane_stride);
    cv::Mat uv_plane_mat = cv::Mat(m_native_height / 2, m_native_width / 2, CV_8UC2, plane1, uv_plane_stride);
    m_impl->push(y_plane_mat);
    m_impl->push(uv_plane_mat);
}

hailo_mat_t HailoNV12Mat::get_type()
{
    return HAILO_MAT_NV12;
}

void HailoNV12Mat::draw_rectangle(hailo_rect_t rect, const hailo_scalar_t color)
{
    cv::Scalar yuv_color = get_nv12_color(color);
    std::uint32_t thickness = m_line_thickness > 1 ? m_line_thickness / 2 : 1;
    // always floor the rect coordinates to even numbers to avoid drawing on the wrong pixel
    int y_plane_rect_x = floor_to_even_number(rect.x);
    int y_plane_rect_y = floor_to_even_number(rect.y);
    int y_plane_rect_width = floor_to_even_number(rect.width);
    int y_plane_rect_height = floor_to_even_number(rect.height);

    cv::Rect y1_rect = cv::Rect(y_plane_rect_x, y_plane_rect_y, y_plane_rect_width, y_plane_rect_height);
    cv::Rect y2_rect = cv::Rect(y_plane_rect_x + 1, y_plane_rect_y + 1, y_plane_rect_width - 2, y_plane_rect_height - 2);
    cv::rectangle(m_impl->get(0), y1_rect, cv::Scalar(yuv_color[0]), thickness);
    cv::rectangle(m_impl->get(0), y2_rect, cv::Scalar(yuv_color[0]), thickness);

    cv::Rect uv_rect = cv::Rect(y_plane_rect_x / 2, y_plane_rect_y / 2, y_plane_rect_width / 2, y_plane_rect_height / 2);
    cv::rectangle(m_impl->get(1), uv_rect, cv::Scalar(yuv_color[1], yuv_color[2]), thickness);
}

void HailoNV12Mat::draw_text(std::string text, hailo_point_t position, double font_scale, const hailo_scalar_t color)
{
    cv::Scalar yuv_color = get_nv12_color(color);
    cv::Point y_position = cv::Point(position.x, position.y);
    cv::Point uv_position = cv::Point(position.x / 2, position.y / 2);
    cv::putText(m_impl->get(0), text, y_position, cv::FONT_HERSHEY_SIMPLEX, font_scale, cv::Scalar(yuv_color[0]), m_font_thickness);
    cv::putText(m_impl->get(1), text, uv_position, cv::FONT_HERSHEY_SIMPLEX, font_scale / 2, cv::Scalar(yuv_color[1], yuv_color[2]), m_font_thickness / 2);
}

void HailoNV12Mat::draw_line(hailo_point_t point1, hailo_point_t point2, const hailo_scalar_t color, int thickness, int line_type)
{
    cv::Scalar yuv_color = get_nv12_color(color);

    int y_plane_x1_value = floor_to_even_number(point1.x);
    int y_plane_y1_value = floor_to_even_number(point1.y);
    int y_plane_x2_value = floor_to_even_number(point2.x);
    int y_plane_y2_value = floor_to_even_number(point2.y);

    cv::line(m_impl->get(1), cv::Point(y_plane_x1_value / 2, y_plane_y1_value / 2), cv::Point(y_plane_x2_value / 2, y_plane_y2_value / 2), cv::Scalar(yuv_color[1], yuv_color[2]), thickness, line_type);

    switch (line_orientation(point1, point2))
    {
    case HAILO_LINE_HORIZONTAL:
        cv::line(m_impl->get(0), cv::Point(y_plane_x1_value, y_plane_y1_value), cv::Point(y_plane_x2_value, y_plane_y2_value), cv::Scalar(yuv_color[0]), thickness, line_type);
        cv::line(m_impl->get(0), cv::Point(y_plane_x1_value, y_plane_y1_value + 1), cv::Point(y_plane_x2_value, y_plane_y2_value + 1), cv::Scalar(yuv_color[0]), thickness, line_type);
        break;
    case HAILO_LINE_VERTICAL:
        cv::line(m_impl->get(0), cv::Point(y_plane_x1_value, y_plane_y1_value), cv::Point(y_plane_x2_value, y_plane_y2_value), cv::Scalar(yuv_color[0]), thickness, line_type);
        cv::line(m_impl->get(0), cv::Point(y_plane_x1_value + 1, y_plane_y1_value), cv::Point(y_plane_x2_value + 1, y_plane_y2_value), cv::Scalar(yuv_color[0]), thickness, line_type);
        break;
    case HAILO_LINE_DIAGONAL:
        cv::line(m_impl->get(0), cv::Point(y_plane_x1_value, y_plane_y1_value), cv::Point(y_plane_x2_value, y_plane_y2_value), cv::Scalar(yuv_color[0]), thickness, line_type);
        cv::line(m_impl->get(0), cv::Point(y_plane_x1_value + 1, y_plane_y1_value + 1), cv::Point(y_plane_x2_value + 1, y_plane_y2_value + 1), cv::Scalar(yuv_color[0]), thickness, line_type);
        break;
    case HAILO_LINE_ANTI_DIAGONAL:
        cv::line(m_impl->get(0), cv::Point(y_plane_x1_value, y_plane_y1_value), cv::Point(y_plane_x2_value, y_plane_y2_value), cv::Scalar(yuv_color[0]), thickness, line_type);
        cv::line(m_impl->get(0), cv::Point(y_plane_x1_value + 1, y_plane_y1_value), cv::Point(y_plane_x2_value + 1, y_plane_y2_value), cv::Scalar(yuv_color[0]), thickness, line_type);
        break;
    default:
        break;
    }
}

void HailoNV12Mat::draw_ellipse(hailo_point_t center, hailo_size_t axes, double angle, double start_angle, double end_angle, const hailo_scalar_t color, int thickness)
{
    thickness = 2;
    cv::Scalar yuv_color = get_nv12_color(color);

    cv::Point y_position = cv::Point(floor_to_even_number(center.x), floor_to_even_number(center.y));
    cv::Point uv_position = cv::Point(floor_to_even_number(center.x) / 2, floor_to_even_number(center.y) / 2);

    cv::ellipse(m_impl->get(0), y_position, {floor_to_even_number(axes.width), floor_to_even_number(axes.height)}, angle, start_angle, end_angle, cv::Scalar(yuv_color[0]), thickness / 2);
    cv::ellipse(m_impl->get(0), y_position, {floor_to_even_number(axes.width) + 1, floor_to_even_number(axes.height) + 1}, angle, start_angle, end_angle, cv::Scalar(yuv_color[0]), thickness / 2);
    cv::ellipse(m_impl->get(1), uv_position, cv::Size(axes.width, axes.height) / 2, angle, start_angle, end_angle, cv::Scalar(yuv_color[1], yuv_color[2]), thickness);
}

void HailoNV12Mat::blur(hailo_rect_t rect, hailo_size_t ksize)
{
    cv::Rect y_rect = cv::Rect(rect.x, rect.y, rect.width, rect.height);
    cv::Mat target_roi_y = m_impl->get(0)(y_rect);
    cv::blur(target_roi_y, target_roi_y, to_cv_size(ksize));
}

hailo_rect_t HailoNV12Mat::get_crop_rect(HailoROIPtr crop_roi)
{
    auto bbox = hailo_common::create_flattened_bbox(crop_roi->get_bbox(), crop_roi->get_scaling_bbox());

    // The Y channel is packed before the U & V on it's own
    hailo_rect_t y_rect = get_bounding_rect(bbox, m_native_width, m_native_height);
    // y_rect values should be even (round if necessary)
    y_rect.width = floor_to_even_number(y_rect.width);
    y_rect.height = floor_to_even_number(y_rect.height);
    y_rect.x = floor_to_even_number(y_rect.x);
    y_rect.y = floor_to_even_number(y_rect.y);

    return y_rect;
}

std::unique_ptr<HailoMatImpl> HailoNV12Mat::crop(HailoROIPtr crop_roi)
{
    hailo_rect_t y_rect = get_crop_rect(crop_roi);

    // The U and V channels are interlaced together after the Y channel,
    // so they need to be cropped separately
    cv::Rect uv_rect;
    // uv_rect values should be exactly half the size of y_rect
    uv_rect.width = y_rect.width / 2;
    uv_rect.height = y_rect.height / 2;
    uv_rect.x = y_rect.x / 2;
    uv_rect.y = y_rect.y / 2;

    cv::Mat cropped_y_mat = cv::Mat(y_rect.height, y_rect.width, CV_8UC1, m_y_stride);
    cv::Mat cropped_uv_mat = cv::Mat(uv_rect.height, uv_rect.width, CV_8UC2, m_uv_stride);

    // Fill the cropped mat with the cropped channels
    m_impl->get(0)(to_cv_rect(y_rect)).copyTo(cropped_y_mat);
    m_impl->get(1)(uv_rect).copyTo(cropped_uv_mat);

    auto result = std::make_unique<HailoMatImpl>();
    result->push(std::move(cropped_y_mat));
    result->push(std::move(cropped_uv_mat));

    return result;
}

HailoNV12Mat::~HailoNV12Mat() = default;
