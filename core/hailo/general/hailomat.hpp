/**
 * Copyright (c) 2021-2026 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
/**
 * @file overlay/common.hpp
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2022-01-20
 *
 * @copyright Copyright (c) 2022-2026
 *
 */

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include "hailo_common.hpp"
#include "hailo_objects.hpp"

// Transformations were taken from https://stackoverflow.com/questions/17892346/how-to-convert-rgb-yuv-rgb-both-ways.
#define RGB2Y(R, G, B) CLIP((0.257 * (R) + 0.504 * (G) + 0.098 * (B)) + 16)
#define RGB2U(R, G, B) CLIP((-0.148 * (R)-0.291 * (G) + 0.439 * (B)) + 128)
#define RGB2V(R, G, B) CLIP((0.439 * (R)-0.368 * (G)-0.071 * (B)) + 128)

//=============================================================================
// OpenCV-independent geometry types
//=============================================================================

/**
 * @brief Simple 2D point structure (replaces cv::Point)
 */
struct hailo_point_t {
    int x;
    int y;

    hailo_point_t(int x_ = 0, int y_ = 0) : x(x_), y(y_) {}
};

/**
 * @brief Simple rectangle structure (replaces cv::Rect)
 */
struct hailo_rect_t {
    int x;
    int y;
    int width;
    int height;

    hailo_rect_t(int x_ = 0, int y_ = 0, int w_ = 0, int h_ = 0)
        : x(x_), y(y_), width(w_), height(h_) {}
    
    hailo_rect_t(hailo_point_t pt1, hailo_point_t pt2)
        : x(pt1.x), y(pt1.y), width(pt2.x - pt1.x), height(pt2.y - pt1.y) {}
};

/**
 * @brief Simple size structure (replaces cv::Size)
 */
struct hailo_size_t {
    int width;
    int height;

    hailo_size_t(int w_ = 0, int h_ = 0) : width(w_), height(h_) {}
};

/**
 * @brief Simple 4-channel scalar for colors (replaces cv::Scalar)
 * Channels are BGR order (like OpenCV): val[0]=B, val[1]=G, val[2]=R, val[3]=A
 */
struct hailo_scalar_t {
    double val[4];

    hailo_scalar_t(double v0 = 0, double v1 = 0, double v2 = 0, double v3 = 0)
        : val{v0, v1, v2, v3} {}

    double& operator[](size_t i) { return val[i]; }
    const double& operator[](size_t i) const { return val[i]; }
};

//=============================================================================
// Enums
//=============================================================================

typedef enum
{
    HAILO_MAT_NONE = -1,
    HAILO_MAT_RGB,
    HAILO_MAT_RGBA,
    HAILO_MAT_YUY2,
    HAILO_MAT_NV12
} hailo_mat_t;

typedef enum
{
    HAILO_LINE_NONE = -1,
    HAILO_LINE_VERTICAL,
    HAILO_LINE_HORIZONTAL,
    HAILO_LINE_DIAGONAL,
    HAILO_LINE_ANTI_DIAGONAL,
} hailo_line_orientation_t;

// Legacy enum names for backwards compatibility
typedef hailo_line_orientation_t LineOrientation;
constexpr LineOrientation NONE = HAILO_LINE_NONE;
constexpr LineOrientation VERTICAL = HAILO_LINE_VERTICAL;
constexpr LineOrientation HORIZONTAL = HAILO_LINE_HORIZONTAL;
constexpr LineOrientation DIAGONAL = HAILO_LINE_DIAGONAL;
constexpr LineOrientation ANTI_DIAGONAL = HAILO_LINE_ANTI_DIAGONAL;

//=============================================================================
// Forward declarations
//=============================================================================

// Forward declaration for PIMPL pattern - hides cv::Mat from header
struct HailoMatImpl;

//=============================================================================
// Utility functions
//=============================================================================

hailo_line_orientation_t line_orientation(hailo_point_t point1, hailo_point_t point2);
int floor_to_even_number(int x);

//=============================================================================
// HailoMat base class
//=============================================================================

class HailoMat
{
protected:
    std::uint32_t m_height;
    std::uint32_t m_width;
    std::uint32_t m_native_height;
    std::uint32_t m_native_width;
    std::uint32_t m_stride;
    int m_line_thickness;
    int m_font_thickness;
    
    // PIMPL: Implementation details hidden from header
    std::unique_ptr<HailoMatImpl> m_impl;
    
    hailo_rect_t get_bounding_rect(HailoBBox bbox, std::uint32_t channel_width, std::uint32_t channel_height);

public:
    HailoMat(std::uint32_t height, std::uint32_t width, std::uint32_t stride, int line_thickness = 1, int font_thickness = 1);
    HailoMat();
    virtual ~HailoMat();
    
    // Move semantics for PIMPL
    HailoMat(HailoMat&&) noexcept;
    HailoMat& operator=(HailoMat&&) noexcept;
    
    // Disable copy (due to unique_ptr)
    HailoMat(const HailoMat&) = delete;
    HailoMat& operator=(const HailoMat&) = delete;
    
    std::uint32_t width();
    std::uint32_t height();
    std::uint32_t native_width();
    std::uint32_t native_height();
    
    /**
     * @brief Get pointer to implementation (for internal use with OpenCV)
     * @return Pointer to HailoMatImpl, which can be used in .cpp files that include OpenCV
     */
    HailoMatImpl* get_impl();
    const HailoMatImpl* get_impl() const;
    
    virtual void draw_rectangle(hailo_rect_t rect, const hailo_scalar_t color) = 0;
    virtual void draw_text(std::string text, hailo_point_t position, double font_scale, const hailo_scalar_t color) = 0;
    virtual void draw_line(hailo_point_t point1, hailo_point_t point2, const hailo_scalar_t color, int thickness, int line_type) = 0;
    virtual void draw_ellipse(hailo_point_t center, hailo_size_t axes, double angle, double start_angle, double end_angle, const hailo_scalar_t color, int thickness) = 0;
    virtual void blur(hailo_rect_t rect, hailo_size_t ksize) = 0;
    
    /*
     * @brief Crop ROIs from the mat, note the present implementation is valid
     *        for interlaced formats. Planar formats such as NV12 should override.
     *
     * @param crop_roi
     *        The roi to crop from this mat.
     * @return HailoMatImpl*
     *         Pointer to a new HailoMatImpl containing the cropped matrices.
     *         Caller is responsible for managing the returned pointer.
     */
    virtual std::unique_ptr<HailoMatImpl> crop(HailoROIPtr crop_roi);

    virtual hailo_rect_t get_crop_rect(HailoROIPtr crop_roi);

    /**
     * @brief Get the type of mat
     *
     * @return hailo_mat_t - The type of the mat.
     */
    virtual hailo_mat_t get_type() = 0;
};

//=============================================================================
// HailoRGBMat
//=============================================================================

class HailoRGBMat : public HailoMat
{
protected:
    std::string m_name;

public:
    HailoRGBMat(uint8_t *buffer, std::uint32_t height, std::uint32_t width, std::uint32_t stride, int line_thickness = 1, int font_thickness = 1, std::string name = "HailoRGBMat");
    virtual hailo_mat_t get_type();
    virtual std::string get_name() const;
    virtual void draw_rectangle(hailo_rect_t rect, const hailo_scalar_t color);
    virtual void draw_text(std::string text, hailo_point_t position, double font_scale, const hailo_scalar_t color);
    virtual void draw_line(hailo_point_t point1, hailo_point_t point2, const hailo_scalar_t color, int thickness, int line_type);
    virtual void draw_ellipse(hailo_point_t center, hailo_size_t axes, double angle, double start_angle, double end_angle, const hailo_scalar_t color, int thickness);
    virtual void blur(hailo_rect_t rect, hailo_size_t ksize);
    virtual ~HailoRGBMat();
};

//=============================================================================
// HailoRGBAMat
//=============================================================================

class HailoRGBAMat : public HailoMat
{
public:
    HailoRGBAMat(uint8_t *buffer, std::uint32_t height, std::uint32_t width, std::uint32_t stride, int line_thickness = 1, int font_thickness = 1);
    virtual hailo_mat_t get_type();
    virtual void draw_rectangle(hailo_rect_t rect, const hailo_scalar_t color);
    virtual void draw_text(std::string text, hailo_point_t position, double font_scale, const hailo_scalar_t color);
    virtual void draw_line(hailo_point_t point1, hailo_point_t point2, const hailo_scalar_t color, int thickness, int line_type);
    virtual void draw_ellipse(hailo_point_t center, hailo_size_t axes, double angle, double start_angle, double end_angle, const hailo_scalar_t color, int thickness);
    virtual void blur(hailo_rect_t rect, hailo_size_t ksize);
    virtual ~HailoRGBAMat();
};

//=============================================================================
// HailoYUY2Mat
//=============================================================================

class HailoYUY2Mat : public HailoMat
{
public:
    HailoYUY2Mat(uint8_t *buffer, std::uint32_t height, std::uint32_t width, std::uint32_t stride, int line_thickness = 1, int font_thickness = 1);
    virtual hailo_mat_t get_type();
    virtual void draw_rectangle(hailo_rect_t rect, const hailo_scalar_t color);
    virtual void draw_text(std::string text, hailo_point_t position, double font_scale, const hailo_scalar_t color);
    virtual void draw_line(hailo_point_t point1, hailo_point_t point2, const hailo_scalar_t color, int thickness, int line_type);
    virtual void draw_ellipse(hailo_point_t center, hailo_size_t axes, double angle, double start_angle, double end_angle, const hailo_scalar_t color, int thickness);
    virtual void blur(hailo_rect_t rect, hailo_size_t ksize);
    virtual ~HailoYUY2Mat();
};

//=============================================================================
// HailoNV12Mat
//=============================================================================

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
    std::uint32_t m_y_stride;
    std::uint32_t m_uv_stride;

public:
    HailoNV12Mat(uint8_t *buffer, std::uint32_t height, std::uint32_t width, std::uint32_t y_plane_stride, std::uint32_t uv_plane_stride, int line_thickness = 1, int font_thickness = 1, void *plane0 = nullptr, void *plane1 = nullptr);
    virtual hailo_mat_t get_type();
    virtual void draw_rectangle(hailo_rect_t rect, const hailo_scalar_t color);
    virtual void draw_text(std::string text, hailo_point_t position, double font_scale, const hailo_scalar_t color);
    virtual void draw_line(hailo_point_t point1, hailo_point_t point2, const hailo_scalar_t color, int thickness, int line_type);
    virtual void draw_ellipse(hailo_point_t center, hailo_size_t axes, double angle, double start_angle, double end_angle, const hailo_scalar_t color, int thickness);
    virtual void blur(hailo_rect_t rect, hailo_size_t ksize);
    virtual hailo_rect_t get_crop_rect(HailoROIPtr crop_roi);
    virtual std::unique_ptr<HailoMatImpl> crop(HailoROIPtr crop_roi);
    virtual ~HailoNV12Mat();
};
