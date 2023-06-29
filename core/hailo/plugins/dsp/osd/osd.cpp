/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/

// General includes
#include <algorithm>
#include <iostream>
#include <tuple>

// Hailo includes
#include "osd.hpp"
#include "hailo_common.hpp"
#include "json_config.hpp"

// rapidjson includes
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/schema.h"

#if __GNUC__ > 8
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

/**
* Load json configuration to OsdParams. Use default values if necessary.
*
* @param[in] config_path path to json config file
* @return OsdParams*
*/
OsdParams *load_json_config(const std::string config_path)
{
    std::vector<osd::staticText> static_texts;
    std::vector<osd::staticImage> static_images;
    std::vector<osd::dateTime> date_times;
    
    if (!fs::exists(config_path))
    {
        std::cerr << "Config file doesn't exist, using default parameters" << std::endl;
        // default static texts
        static_texts.push_back({"Hailo", 1.0, 1, {255, 255, 255}, 0.7, 0.7});
        static_texts.push_back({"Stream 0", 1.0, 1, {255, 255, 255}, 0.1, 0.1});

        // default date/time
        date_times.push_back({1.0, 1, {255, 255, 255}, 0.6, 0.1});
    } else {
        char config_buffer[4096];
        std::FILE *fp = fopen(config_path.c_str(), "r");
        if (fp == nullptr)
        {
            throw std::runtime_error("JSON config file is not valid");
        }
        rapidjson::FileReadStream stream(fp, config_buffer, sizeof(config_buffer));
        bool valid_json = common::validate_json_with_schema(stream, osd::json_schema);
        if (valid_json)
        {
            rapidjson::Document doc_config_json;
            doc_config_json.ParseStream(stream);

            if (doc_config_json.HasMember("staticText") && doc_config_json["staticText"].IsArray())
            {
                auto json_static_texts = doc_config_json["staticText"].GetArray();
                for (uint i = 0; i < json_static_texts.Size(); i++)
                {
                    const rapidjson::Value& static_text_entry = json_static_texts[i];
                    std::array<int, 3> color_rgb = {255, 255, 255};
                    if (static_text_entry.HasMember("rgb"))
                    {
                        auto rgb_values = static_text_entry["rgb"].GetArray();
                        color_rgb = {rgb_values[0].GetInt(), rgb_values[1].GetInt(), rgb_values[2].GetInt()};
                    }
                    static_texts.push_back({static_text_entry["label"].GetString(),
                                            static_text_entry["font_size"].GetFloat(),
                                            static_text_entry["line_thickness"].GetInt(),
                                            color_rgb,
                                            static_text_entry["x"].GetFloat(),
                                            static_text_entry["y"].GetFloat()});
                }
            }
            if (doc_config_json.HasMember("staticImage") && doc_config_json["staticImage"].IsArray())
            {
                auto json_static_images = doc_config_json["staticImage"].GetArray();
                for (uint i = 0; i < json_static_images.Size(); i++)
                {
                    const rapidjson::Value& static_image_entry = json_static_images[i];
                    static_images.push_back({static_image_entry["image_path"].GetString(),
                                             static_image_entry["width"].GetFloat(),
                                             static_image_entry["height"].GetFloat(),
                                             static_image_entry["x"].GetFloat(),
                                             static_image_entry["y"].GetFloat()});
                }
            }
            if (doc_config_json.HasMember("dateTime") && doc_config_json["dateTime"].IsArray())
            {
                auto json_date_times = doc_config_json["dateTime"].GetArray();
                for (uint i = 0; i < json_date_times.Size(); i++)
                {
                    const rapidjson::Value& static_date_time_entry = json_date_times[i];
                    std::array<int, 3> color_rgb = {255, 255, 255};
                    if (static_date_time_entry.HasMember("rgb"))
                    {
                        auto rgb_values = static_date_time_entry["rgb"].GetArray();
                        color_rgb = {rgb_values[0].GetInt(), rgb_values[1].GetInt(), rgb_values[2].GetInt()};
                    }
                    date_times.push_back({static_date_time_entry["font_size"].GetFloat(),
                                          static_date_time_entry["line_thickness"].GetInt(),
                                          color_rgb,
                                          static_date_time_entry["x"].GetFloat(),
                                          static_date_time_entry["y"].GetFloat()});
                }
            }
        }
        else
        {
            throw std::runtime_error("JSON config file is not valid");
        }
    }
    OsdParams *params = new OsdParams(static_texts, static_images, date_times);
    return params;
}

/**
* Deep free on OsdParams, freeing and unmaping all overlay data
*
* @param[in] params_ptr params to free
* @return void
*/
void free_param_resources(OsdParams *params_ptr)
{
    if (nullptr == params_ptr)
        return;

    // iterate over params, free all resources
    for (osd::staticText static_text : params_ptr->static_texts)
    {
        gst_video_frame_unmap(&(static_text.video_frame));
        free_overlay_property_planes(&(static_text.dsp_overlay_properties));
    }
    for (osd::staticImage static_image : params_ptr->static_images)
    {
        gst_video_frame_unmap(&(static_image.video_frame));
        free_overlay_property_planes(&(static_image.dsp_overlay_properties));
    }
    for (osd::dateTime date_time : params_ptr->date_times)
    {
        for (uint index = 0; index < sizeof(osd::DATE_TIME_CHARS); ++index)
        {
            gst_video_frame_unmap(&(date_time.video_frames[index]));
            free_image_property_planes(&(date_time.dsp_image_properties[index]));
        }
    }
    delete params_ptr;
}

/**
* Calculate x and y offsets for overlays
* DSP blending does not support out-of-bounds overlays
* so throw errors if any
*
* @param[in] x_norm normalized x for top-left corner
* @param[in] y_norm normalized y for top-left corner
* @param[in] overlay_width width of overlay in pixels
* @param[in] overlay_height height of overlay in pixels
* @param[in] image_width width of image in pixels
* @param[in] image_height height of image in pixels
* @param[in] overlay_type overlay type for error logs
* @return std::tuple<int, int>
*/
std::tuple<int, int> calc_xy_offsets(float x_norm, float y_norm,
                                     int overlay_width, int overlay_height,
                                     int image_width, int image_height, const std::string &overlay_type)
{
    int x_offset = x_norm * image_width;
    int y_offset = y_norm * image_height;
    if (x_offset + overlay_width > image_width)
        throw std::runtime_error(overlay_type + " overlay too wide to fit in frame! Adjust width or x offset in json.");
    if (y_offset + overlay_height > image_height)
        throw std::runtime_error(overlay_type + " overlay too tall to fit in frame! Adjust height or y offset in json.");
    return {x_offset, y_offset};
}

/**
* Load all overlay images in A420 format. Reads static images from
* file and generates static texts & date/time chars.
*
* @param[in] params params to load
* @param[in] full_image_width width of image in pixels
* @param[in] full_image_height height of image in pixels
* @return osd_status_t
*/
osd_status_t initialize_overlay_images(OsdParams *params, int full_image_width, int full_image_height)
{
    osd_status_t ret = OSD_STATUS_UNINITIALIZED;

    // Initialize static images
    for (uint i = 0; i < params->static_images.size(); i++)
    {
        cv::Mat loaded_image = osd::load_image_from_file(params->static_images[i].image_path);
        cv::Mat resized_image = osd::resize_mat(loaded_image, 
                                                params->static_images[i].width * full_image_width,
                                                params->static_images[i].height * full_image_height);
        GstVideoFrame gst_bgra_image = osd::gst_video_frame_from_mat_bgra(resized_image);
        GstVideoFrame dest_frame;
        params->static_images[i].video_frame = dest_frame;
        osd::convert_2_dsp_video_frame(&gst_bgra_image, &(params->static_images[i].video_frame), GST_VIDEO_FORMAT_A420);
        params->static_images[i].dsp_image_properties = create_image_properties_from_video_frame(&(params->static_images[i].video_frame));
        auto [x_offset, y_offset] = calc_xy_offsets(params->static_images[i].x, params->static_images[i].y,
                                                    params->static_images[i].dsp_image_properties.width, params->static_images[i].dsp_image_properties.height,
                                                    full_image_width, full_image_height, "Static Image");
        params->static_images[i].dsp_overlay_properties = (dsp_overlay_properties_t){params->static_images[i].dsp_image_properties,
                                                                                     (size_t)x_offset, (size_t)y_offset};
        gst_buffer_unref(gst_bgra_image.buffer);
        gst_video_frame_unmap(&gst_bgra_image);
    }

    // Initialize static texts
    for (uint i = 0; i < params->static_texts.size(); i++)
    {
        cv::Mat bgra_text = osd::load_image_from_text(params->static_texts[i].label,
                                                      params->static_texts[i].font_size,
                                                      params->static_texts[i].line_thickness,
                                                      params->static_texts[i].color_rgb);
        GstVideoFrame gst_bgra_image = osd::gst_video_frame_from_mat_bgra(bgra_text);
        GstVideoFrame dest_frame;
        params->static_texts[i].video_frame = dest_frame;
        osd::convert_2_dsp_video_frame(&gst_bgra_image, &(params->static_texts[i].video_frame), GST_VIDEO_FORMAT_A420);
        params->static_texts[i].dsp_image_properties = create_image_properties_from_video_frame(&(params->static_texts[i].video_frame));
        auto [x_offset, y_offset] = calc_xy_offsets(params->static_texts[i].x, params->static_texts[i].y,
                                                    params->static_texts[i].dsp_image_properties.width, params->static_texts[i].dsp_image_properties.height,
                                                    full_image_width, full_image_height, "Static Text");
        params->static_texts[i].dsp_overlay_properties = (dsp_overlay_properties_t){params->static_texts[i].dsp_image_properties,
                                                                                    (size_t)x_offset, (size_t)y_offset};
        gst_buffer_unref(gst_bgra_image.buffer);
        gst_video_frame_unmap(&gst_bgra_image);
    }

    // Initialize static texts
    for (uint i = 0; i < params->date_times.size(); i++)
    {
        for (uint j = 0; j < sizeof(osd::DATE_TIME_CHARS); j++)
        {
            cv::Mat bgra_char = osd::load_image_from_text(std::string(&osd::DATE_TIME_CHARS[j], 1),
                                                          params->date_times[i].font_size,
                                                          params->date_times[i].line_thickness,
                                                          params->date_times[i].color_rgb);
            GstVideoFrame gst_bgra_char = osd::gst_video_frame_from_mat_bgra(bgra_char);
            GstVideoFrame dest_frame;
            params->date_times[i].video_frames[j] = dest_frame;
            osd::convert_2_dsp_video_frame(&gst_bgra_char, &(params->date_times[i].video_frames[j]), GST_VIDEO_FORMAT_A420);
            params->date_times[i].dsp_image_properties[j] = create_image_properties_from_video_frame(&(params->date_times[i].video_frames[j]));
            gst_buffer_unref(gst_bgra_char.buffer);
            gst_video_frame_unmap(&gst_bgra_char);
        }
        auto [x_offset, y_offset] = calc_xy_offsets(params->date_times[i].x, params->date_times[i].y,
                                                    params->date_times[i].dsp_image_properties[0].width * 19, params->date_times[i].dsp_image_properties[0].height,
                                                    full_image_width, full_image_height, "Date/Time");
    }

    ret = OSD_STATUS_OK;
    return ret;
}

/**
* Call DSP blending on all loaded overlays to the incoming GstVideoFrame
*
* @param[in] video_frame the incoming image to blend onto
* @param[in] params params with loaded overlay images to blend with
* @return osd_status_t
*/
osd_status_t blend_all(GstVideoFrame *video_frame, OsdParams *params)
{
    osd_status_t ret = OSD_STATUS_UNINITIALIZED;

    // build image_properties from the input image and overlay
    dsp_image_properties_t input_image_properties = create_image_properties_from_video_frame(video_frame);

    // We prepare to blend all overlays at once
    int num_overlays = params->static_texts.size() + params->static_images.size() + (params->date_times.size() * 19);
    dsp_overlay_properties_t overlays[num_overlays];

    int staged_overlays = 0;
    // iterate over params
    for (osd::staticText static_text : params->static_texts)
    {
        overlays[staged_overlays] = static_text.dsp_overlay_properties;
        staged_overlays++;
    }
    for (osd::staticImage static_image : params->static_images)
    {
        overlays[staged_overlays] = static_image.dsp_overlay_properties;
        staged_overlays++;
    }
        
    size_t full_image_width = GST_VIDEO_FRAME_WIDTH(video_frame);
    size_t full_image_height = GST_VIDEO_FRAME_HEIGHT(video_frame);
    for (osd::dateTime date_time : params->date_times)
    {
        int x_offset = date_time.x * full_image_width;
        int y_offset = date_time.y * full_image_height;
        std::vector<int> char_indices = osd::select_chars_for_timestamp();
        for (uint index = 0; index < char_indices.size(); ++index)
        {
            overlays[staged_overlays] = (dsp_overlay_properties_t){date_time.dsp_image_properties[char_indices[index]], (size_t)x_offset, (size_t)y_offset};
            staged_overlays++;
            x_offset += date_time.dsp_image_properties[char_indices[index]].width;
        }
    }

    // Perform blending for all overlays
    dsp_status status;
    status = perform_dsp_multiblend(&input_image_properties, overlays, staged_overlays);
    if (status != DSP_SUCCESS)
        throw std::runtime_error("DSP BLEND FAILED: " + std::to_string(status));

    // free the struct
    free_image_property_planes(&input_image_properties);

    ret = OSD_STATUS_OK;
    return ret;
}
