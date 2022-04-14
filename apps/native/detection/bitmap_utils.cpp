/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/**
 * @file bitmap_utils.cpp
 * @brief bitmap utils functions
 **/

#include "bitmap_utils.hpp"

const uint32_t RGB_CHANNELS_COUNT = 3;
const uint32_t RGB_BITS_PER_PIXEL = RGB_CHANNELS_COUNT * 8;


const std::string get_file_name(const std::string &file_path)
{
    auto dir_char = file_path.find('/');
    if (std::string::npos == dir_char) {
        return file_path;
    } else {
        auto extension = file_path.find('.');
        if (std::string::npos == extension) {
            return file_path.substr((dir_char + 1));
        }
        size_t len = extension - dir_char - 1;
        return file_path.substr((dir_char + 1), len);
    }
}

hailo_status BMPImage::read_image(const std::string &file_path, std::unique_ptr<BMPImage> &input_image)
{
    bmp_file_header_t file_header;
    std::ifstream image(file_path, std::ios::in | std::ios_base::binary);
    if (image.fail()) {
        std::cerr << "Failed opening file '" << file_path << "' with errno = " << errno << std::endl;
        return HAILO_OPEN_FILE_FAILURE;
    }

    // Read to file header
    image.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    if (image.fail()) {
        std::cerr << "Failed reading file header of '" << file_path << "' with errno = " << errno << std::endl;
        return HAILO_FILE_OPERATION_FAILURE;
    }
    if (BMP_FILE_TYPE != file_header.type) {
        std::cerr << "The file '" << file_path << "' is in the wrong format. Please insert an image of type BMP" << std::endl;
        return HAILO_FILE_OPERATION_FAILURE;
    }

    // Read to bmp info header
    bmp_info_header_t info_header;
    image.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));
    if (image.fail()) {
        std::cerr << "Failed reading bmp header of '" << file_path << "' with errno = " << errno << std::endl;
        return HAILO_FILE_OPERATION_FAILURE;
    }

    // Validate input image match the yolov5 net
    if ((YOLOV5M_IMAGE_WIDTH != info_header.width) || (YOLOV5M_IMAGE_HEIGHT != info_header.height)) {
        std::cerr << "Input image '" << file_path << "' has the wrong size! Size should be" << YOLOV5M_IMAGE_WIDTH <<
            "x" << YOLOV5M_IMAGE_HEIGHT << ", received: " << info_header.width << "x" << info_header.height << std::endl;
        return HAILO_INVALID_ARGUMENT;
    }
    if (RGB_BITS_PER_PIXEL != info_header.bits_per_pixel_count) {
        std::cerr << "Input image '" << file_path << "' bits per pixel must be " << RGB_BITS_PER_PIXEL << ", but received: " 
            << info_header.bits_per_pixel_count << std::endl;
        return HAILO_INVALID_ARGUMENT;
    }

    // Move fd to the pixel data location
    image.seekg(file_header.data_offset, image.beg);
    if (image.fail()) {
        std::cerr << "seekg() failed in file '" << file_path << "' with errno = " << errno << std::endl;
        return HAILO_FILE_OPERATION_FAILURE;
    }

    // Read image data
    uint32_t data_size = info_header.width * info_header.height * (info_header.bits_per_pixel_count / 8);
    std::vector<uint8_t> data(data_size);
    image.read(reinterpret_cast<char*>(data.data()), data.size());
    if (image.fail()) {
        std::cerr << "Failed reading data of image '" << file_path << "' with errno = " << errno << std::endl;
        return HAILO_FILE_OPERATION_FAILURE;
    }

    file_header.size += static_cast<uint32_t>(data.size());
    input_image = std::make_unique<BMPImage>(BMPImage(std::move(file_header), std::move(info_header), std::move(data), get_file_name(file_path)));
    return HAILO_SUCCESS;
}

BMPImage::BMPImage(bmp_file_header_t &&file_header, bmp_info_header_t &&info_header, std::vector<uint8_t> &&data,
    const std::string &file_name) :
    m_file_header(file_header),
    m_info_header(info_header),
    m_data(data),
    m_file_name(file_name)
{}

hailo_status BMPImage::dump_image()
{
    const std::string file_path = OUTPUT_DIR_PATH + name() + ".bmp";
    std::ofstream image(file_path, std::ios::out | std::ios_base::binary);
    if (image.fail()) {
        std::cerr << "Failed opening output file '" << file_path << "'" << std::endl;
        return HAILO_OPEN_FILE_FAILURE;
    }

    image.write(reinterpret_cast<const char*>(&m_file_header), sizeof(m_file_header));
    if (image.fail()) {
        std::cerr << "Failed writing the file header of output image '" << file_path << "'" << std::endl;
        return HAILO_FILE_OPERATION_FAILURE;
    }

    image.write(reinterpret_cast<const char*>(&m_info_header), sizeof(m_info_header));
    if (image.fail()) {
        std::cerr << "Failed writing the info header of output image '" << file_path << "'" << std::endl;
        return HAILO_FILE_OPERATION_FAILURE;
    }

    image.write(reinterpret_cast<const char*>(m_data.data()), m_data.size());
    if (image.fail()) {
        std::cerr << "Failed writing the data of output image '" << file_path << "'" << std::endl;
        return HAILO_FILE_OPERATION_FAILURE;
    }

    return HAILO_SUCCESS;
}

const std::vector<uint8_t> &BMPImage::get_data() const
{
    return m_data;
}

const std::string BMPImage::name() const
{
    return m_file_name;
}

void BMPImage::draw_border(uint32_t left, uint32_t right, uint32_t bottom, uint32_t top, Color color)
{
    draw_horizontal_line(left, right, bottom, top, color);
    draw_vertical_line(left, right, bottom, top, color);
}

void BMPImage::draw_horizontal_line(uint32_t left, uint32_t right, uint32_t bottom, uint32_t top, Color color)
{
    for (uint32_t x = left; x <= right; x++) {
        fill_pixel(x, bottom, color);
        fill_pixel(x, top, color);

        // Make detection lines wider
        if ((bottom + 1 < static_cast<uint32_t>(m_info_header.height)) && (top - 1 > 0)) {
            fill_pixel(x, bottom + 1, color);
            fill_pixel(x, top - 1, color);
        }
    }
}

void BMPImage::draw_vertical_line(uint32_t left, uint32_t right, uint32_t bottom, uint32_t top, Color color)
{
    for (uint32_t y = bottom; y <= top; y++) {
        fill_pixel(left, y, color);
        fill_pixel(right, y, color);

        // Make detection lines wider
        if ((left + 1 < static_cast<uint32_t>(m_info_header.width)) && (right - 1 > 0)) {
            fill_pixel(left + 1, y, color);
            fill_pixel(right - 1, y, color);
        }
    }
}

void BMPImage::fill_pixel(uint32_t x, uint32_t y, Color color)
{
    m_data[RGB_CHANNELS_COUNT * (y * m_info_header.width + x)] = color.b;
    m_data[RGB_CHANNELS_COUNT * (y * m_info_header.width + x) + 1] = color.g;
    m_data[RGB_CHANNELS_COUNT * (y * m_info_header.width + x) + 2] = color.r;
}

hailo_status BMPImage::get_images(std::vector<std::unique_ptr<BMPImage>> &input_images, const size_t inputs_count)
{
    for (uint32_t i = 0; i < inputs_count; i++) {
        std::string file_path = INPUT_DIR_PATH + "image" + std::to_string(i) + ".bmp";
        std::unique_ptr<BMPImage> image;
        auto status = BMPImage::read_image(file_path, image);
        if (HAILO_SUCCESS != status) {
            std::cerr << "Failed reading file: '" << file_path << "', status = " << status << std::endl;
            return status;
        }

        input_images.emplace_back(image.release());
    }
    
    return HAILO_SUCCESS;
}
