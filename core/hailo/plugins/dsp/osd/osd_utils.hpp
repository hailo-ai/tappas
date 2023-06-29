
#pragma once
#include <ctime> 
#include <chrono>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <iostream>
#include <map>
#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/filesystem.hpp>
#include <string>
#include <vector>
#include "dsp/gsthailodsp.h"

namespace osd
{
    struct staticText {
        std::string label;
        float font_size;
        int line_thickness;
        std::array<int, 3> color_rgb;
        float x;
        float y;
        GstVideoFrame video_frame;
        dsp_image_properties_t dsp_image_properties;
        dsp_overlay_properties_t dsp_overlay_properties;
    };

    struct staticImage {
        std::string image_path;
        float width;
        float height;
        float x;
        float y;
        GstVideoFrame video_frame;
        dsp_image_properties_t dsp_image_properties;
        dsp_overlay_properties_t dsp_overlay_properties;
    };

    struct dateTime {
        float font_size;
        int line_thickness;
        std::array<int, 3> color_rgb;
        float x;
        float y;
        std::array<GstVideoFrame, 13> video_frames; // chars 0-9 - ' ' and :
        std::array<dsp_image_properties_t, 13> dsp_image_properties; // chars 0-9 - ' ' and :
    };

    inline char DATE_TIME_CHARS[13] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', ' ', ':'};

    /**
    * Load an image file to an RGBA cv::Mat
    *
    * @param[in] image_path the path of the image to load
    * @return a loaded cv::Mat
    */
    inline cv::Mat load_image_from_file(std::string image_path)
    {
        // check if the file exists
        if (!cv::utils::fs::exists(image_path))
        {
            std::cerr << "Error: file " << image_path << " does not exist" << std::endl;
            throw std::runtime_error("Image path in json config is not valid");
        }
        // read the image from file, keeping alpha channel
        cv::Mat image = cv::imread(image_path, cv::IMREAD_UNCHANGED);
        // check if the image was read successfully
        if (image.empty())
        {
            std::cerr << "Error: failed to read image file " << image_path << std::endl;
            throw std::runtime_error("Image path is valid but failed to load");
        }
        // convert image to 4-channel RGBA format if necessary
        if (image.channels() != 4)
        {
            GST_INFO("READ IMAGE THAT WAS NOT 4 channels");
            cv::cvtColor(image, image, cv::COLOR_BGR2BGRA);
        }
        return image;
    }

    /**
    * Create a BGRA cv::Mat of a string label
    *
    * @param[in] label the text to draw
    * @param[in] font_size the font scaling factor
    * @param[in] line_thickness the line thickness
    * @return a drawn cv::Mat
    */
    inline cv::Mat load_image_from_text(const std::string& label, float font_size, int line_thickness, std::array<int, 3> color_rgb)
    {
        // calculate the size of the text
        int baseline=0;
        cv::Size text_size = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, font_size, line_thickness, &baseline);

        // ensure even dimensions, round up not to clip text
        text_size.width += (text_size.width % 2);
        text_size.height += (text_size.height % 2);

        // create a transparent BGRA image
        cv::Mat bgra_text(text_size.height + baseline, text_size.width, CV_8UC4, cv::Scalar(0, 0, 0, 0));
        auto text_position = cv::Point(0, text_size.height);
        cv::Scalar text_color(color_rgb[2], color_rgb[1], color_rgb[0], 255); // The input is expected RGB, but we draw as BGRA

        // draw the text
        cv::putText(bgra_text, label, text_position, cv::FONT_HERSHEY_SIMPLEX, font_size, text_color, line_thickness);

        return bgra_text;
    }

    /**
    * Resize a cv::Mat, intended for RGB,BGR,RGBA,BGRA
    * use only
    *
    * @param[in] mat the cv::Mat to resize
    * @param[in] width the target width
    * @param[in] height the target height
    * @return a resized cv::Mat
    */
    inline cv::Mat resize_mat(cv::Mat original, int width, int height)
    {
        // ensure even dimensions, round up not to clip image
        width += (width % 2);
        height += (height % 2);

        // resize the mat image to target size
        cv::Mat resized_image;
        cv::resize(original, resized_image, cv::Size(width, height), 0, 0, cv::INTER_AREA);
        return resized_image;
    }

    /**
    * Converts an BGRA cv::Mat to GstVideoFrame
    *
    * @param[in] mat the cv::Mat to convert
    * @return a GstVideoFrame of the given cv::Mat
    */
    inline GstVideoFrame gst_video_frame_from_mat_bgra(cv::Mat mat)
    {
        // Create caps at BGRA format and required size
        GstCaps *caps = gst_caps_new_simple("video/x-raw", "format", G_TYPE_STRING, "BGRA",
                                            "width", G_TYPE_INT, mat.cols,
                                            "height", G_TYPE_INT, mat.rows,
                                            NULL);
        // Create GstVideoInfo meta from those caps
        GstVideoInfo *image_info = gst_video_info_new();
        gst_video_info_from_caps(image_info, caps);
        // Create a GstBuffer from the cv::mat, allowing for contiguous memory
        GstBuffer *buffer = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY,
                                                        mat.data,
                                                        mat.total() * mat.elemSize(),
                                                        0, mat.total() * mat.elemSize(),
                                                        NULL, NULL );
        // Create and map a GstVideoFrame from the GstVideoInfo and GstBuffer
        GstVideoFrame frame;
        gst_video_frame_map(&frame, image_info, buffer, GST_MAP_READ);
        gst_video_info_free(image_info);
        gst_caps_unref(caps);
        return frame;
    }

    /**
    * Converts an GstVideoFrame to a given format
    * the new frame will have contiguous memory
    *
    * @param[in] src_frame the GstVideoFrame to convert
    * @param[in] dest_format the format to convert to
    * @return a GstVideoFrame* to the new GstVideoFrame
    */
    inline void convert_2_dsp_video_frame(GstVideoFrame* src_frame, GstVideoFrame* dest_frame, GstVideoFormat dest_format) {
        // Prepare the video info and set the new format
        GstVideoInfo *dest_info = gst_video_info_copy(&src_frame->info);
        gst_video_info_set_format(dest_info, dest_format, src_frame->info.width, src_frame->info.height);
        // Prepare the GstVideoConverter that will facilitate the conversion
        GstVideoConverter* converter = gst_video_converter_new(&src_frame->info, dest_info, NULL);

        void *buffer_ptr = NULL;
        dsp_status buffer_status = create_hailo_dsp_buffer(dest_info->size, &buffer_ptr);
        if (buffer_status != DSP_SUCCESS)
        {
            std::cerr << "Error: create_hailo_dsp_buffer - failed to create buffer" << std::endl;
            throw std::runtime_error("Failed to create Hailo dsp buffer");
        }
        GstBuffer *buffer = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_PHYSICALLY_CONTIGUOUS,
                                                        buffer_ptr,
                                                        dest_info->size, 0, dest_info->size, buffer_ptr, GDestroyNotify(release_hailo_dsp_buffer));

        // Prepare the destination buffer and frame
        gst_video_frame_map(dest_frame, dest_info, buffer, GST_MAP_WRITE);
        // Make the conversion
        gst_video_converter_frame(converter, src_frame, dest_frame);
        gst_video_converter_free(converter);
    }

    /**
    * Converts a number of indeterminant length to indices
    * and pushes them into char_indices
    *
    * @param[in] number the int to convert
    * @param[in] char_indices where to push the converted indices
    * @return an A420 cv::Mat
    */
    inline void numbers_2_indices(int number, std::vector<int> &char_indices)
    {
        std::string number_s = std::to_string(number);
        // Traverse the string
        for (auto &ch : number_s) {
            // Get the char as an int
            int x = ch - '0';
            char_indices.push_back(x);
        }
    }

    /**
    * Curates a selection of indices representing chars to draw
    *
    * @return std::vector<int> indices of the chars needed to draw
    */
    inline std::vector<int> select_chars_for_timestamp()
    {
        auto system_time = std::chrono::system_clock::now();
        std::time_t ttime = std::chrono::system_clock::to_time_t(system_time);
        std::tm *time_now = std::gmtime(&ttime);

        std::vector<int> char_indices;
        numbers_2_indices(time_now->tm_mday, char_indices);         // day
        char_indices.push_back(10);                                 // -
        numbers_2_indices(time_now->tm_mon, char_indices);          // month
        char_indices.push_back(10);                                 // -
        numbers_2_indices(1900 + time_now->tm_year, char_indices);  // year
        char_indices.push_back(11);                                 // ' '
        numbers_2_indices(time_now->tm_hour, char_indices);         // hour
        char_indices.push_back(12);                                 // :
        numbers_2_indices(time_now->tm_min, char_indices);          // min
        char_indices.push_back(12);                                 // :
        numbers_2_indices(time_now->tm_sec, char_indices);          // sec

        return char_indices;
    }

    /**
    * Overloads << for printing the staticText struct info
    *
    * @return ostream
    */
    inline std::ostream& operator<<(std::ostream& os, const staticText& sText) {
    return os << "--- staticText ---" << std::endl
              << "label: " << sText.label << std::endl
              << "font_size: " << sText.font_size << std::endl
              << "line_thickness: " << sText.line_thickness << std::endl
              << "rgb: [" << sText.color_rgb[0] << ", " << sText.color_rgb[1] << ", " << sText.color_rgb[2] << "]" << std::endl
              << "x: " << sText.x << std::endl
              << "y: " << sText.y << std::endl;
    }

    /**
    * Overloads << for printing the staticImage struct info
    *
    * @return ostream
    */
    inline std::ostream& operator<<(std::ostream& os, const staticImage& sImage) {
    return os << "--- staticImage ---" << std::endl
              << "image_path: " << sImage.image_path << std::endl
              << "x: " << sImage.x << std::endl
              << "y: " << sImage.y << std::endl
              << "width: " << sImage.width << std::endl
              << "height: " << sImage.height << std::endl;
    }

    /**
    * Overloads << for printing the dateTime struct info
    *
    * @return ostream
    */
    inline std::ostream& operator<<(std::ostream& os, const dateTime& dTime) {
    return os << "--- dateTime ---" << std::endl
              << "font_size: " << dTime.font_size << std::endl
              << "line_thickness: " << dTime.line_thickness << std::endl
              << "rgb: [" << dTime.color_rgb[0] << ", " << dTime.color_rgb[1] << ", " << dTime.color_rgb[2] << "]" << std::endl
              << "x: " << dTime.x << std::endl
              << "y: " << dTime.y << std::endl;
    }

    inline const char *json_schema = R""""({
        "$schema": "http://json-schema.org/draft-04/schema#",
        "type": "object",
        "properties": {
            "staticImage": {
                "type": "array",
                "items":{
                    "type": "object",
                    "properties": {
                        "image_path": {
                            "type": "string"
                        },
                        "width": {
                            "type": "number"
                        },
                        "height": {
                            "type": "number"
                        },
                        "x": {
                            "type": "number"
                        },
                        "y": {
                            "type": "number"
                        }
                    },
                    "required": ["image_path"]
                }
			},
            "dateTime": {
                "type": "array",
                "items":{
                    "type": "object",
                    "properties": {
                        "font_size": {
                            "type": "number"
                        },
                        "line_thickness": {
                            "type": "integer"
                        },
                        "rgb": {
                            "type": "array",
                            "items": [
                                {
                                "type": "integer"
                                },
                                {
                                "type": "integer"
                                },
                                {
                                "type": "integer"
                                }
                            ]
                        },
                        "x": {
                            "type": "number"
                        },
                        "y": {
                            "type": "number"
                        }
                    }
                }
			},
            "staticText": {
                "type": "array",
                "default": [],
                "items":{
                    "type": "object",
                    "properties": {
                        "label": {
                            "type": "string"
                        },
                        "font_size": { 
                            "type": "number"
                        },
                        "line_thickness": {
                            "type": "integer"
                        },
                        "rgb": {
                            "type": "array",
                            "items": [
                                {
                                "type": "integer"
                                },
                                {
                                "type": "integer"
                                },
                                {
                                "type": "integer"
                                }
                            ]
                        },
                        "x": {
                            "type": "number"
                        },
                        "y": {
                            "type": "number"
                        }
                    },
                    "required": ["label"]
                }
            }
		}
    })"""";
}