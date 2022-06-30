/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/**
 * @file bitmap_utils.hpp
 * @brief bitmap utils functions
 **/

#ifndef _HAILO_BITMAP_UTILS_HPP_
#define _HAILO_BITMAP_UTILS_HPP_

#include "hailo/hailort.h"

#include <fstream>
#include <iostream>
#include <functional>
#include <memory>
#include<string>  

const std::string INPUT_DIR_PATH = "input_images/";
const std::string OUTPUT_DIR_PATH = "output_images/";

struct Color {
    Color(uint8_t in_r, uint8_t in_g, uint8_t in_b) : r(in_r), g(in_g), b(in_b) {}

    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class BMPImage final
{
public:
    BMPImage(BMPImage &&other) noexcept = default;
    BMPImage &operator=(BMPImage &&other) noexcept = default;

    #pragma pack(push, 1)
    struct bmp_file_header_t {
        uint16_t type;
        uint32_t size;
        uint32_t reserved;
        uint32_t data_offset;
    };

    struct bmp_info_header_t {
        uint32_t size;
        int width;
        int height;
        uint16_t color_planes;
        uint16_t bits_per_pixel_count;
        uint32_t compression;
        uint32_t image_size;
        int horizontal_resolution;
        int vertical_resolution;
        uint32_t color_count;
        uint32_t important_color_count;
    };
    #pragma pack(pop)

    static hailo_status get_images(std::vector<std::unique_ptr<BMPImage>> &input_images, const size_t inputs_count, int image_width, int image_height);
    static hailo_status read_image(const std::string &file_path, std::unique_ptr<BMPImage> &input_image, int image_width, int image_height);

    hailo_status dump_image();
    void draw_border(uint32_t left, uint32_t right, uint32_t bottom, uint32_t top, Color color);
    const std::vector<uint8_t> &get_data() const;
    const std::string name() const;

private:
    BMPImage(bmp_file_header_t &&file_header, bmp_info_header_t &&info_header, std::vector<uint8_t> &&data, const std::string &file_name);
    void draw_horizontal_line(uint32_t left, uint32_t right, uint32_t bottom, uint32_t top, Color color);
    void draw_vertical_line(uint32_t left, uint32_t right, uint32_t bottom, uint32_t top, Color color);
    void fill_pixel(uint32_t x, uint32_t y, Color color);

    static const uint16_t BMP_FILE_TYPE = 0x4D42;
    bmp_file_header_t m_file_header;
    bmp_info_header_t m_info_header;
    std::vector<uint8_t> m_data;
    std::string m_file_name;
};

#endif /* _HAILO_BITMAP_UTILS_HPP_ */
