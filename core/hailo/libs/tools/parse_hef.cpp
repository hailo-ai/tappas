/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include <iostream>
#include <typeinfo>
#include <cstdlib>
#include <unistd.h>
#include <cxxopts.hpp>
#include "hailo/hef.hpp"
#include "hailo/expected.hpp"
#include "hailo/hailort.h"

using namespace hailort;
//******************************************************************
// MAIN
//******************************************************************
/**
 * @brief Build command line arguments.
 *
 * @return cxxopts::Options
 *         The available user arguments.
 */
cxxopts::Options build_arg_parser()
{
    cxxopts::Options options("License Plate Recognition");
    options.allow_unrecognised_options();
    options.add_options()
    ("h,help", "Show this help")
    ("i, input", "Input hef", cxxopts::value<std::string>());
    return options;
}

void parse_vstreams(std::vector<hailo_vstream_info_t> infos)
{
    for (auto &info : infos)
    {
        std::cout << "Layer Name: " << info.name << std::endl;
        switch (info.format.order)
        {
        case HAILO_FORMAT_ORDER_HAILO_NMS:
            std::cout << "Shape: " << std::endl;
            std::cout << "  num_of_classes: " << info.nms_shape.number_of_classes << std::endl;
            std::cout << "  max_bboxes_per_class: " << info.nms_shape.max_bboxes_per_class << std::endl;
            break;
        default:
            std::cout << "Shape: " << info.shape.height << "x" << info.shape.width << "x" << info.shape.features << std::endl;
            break;
        }

        std::cout << "qp_zp: " << (float)info.quant_info.qp_zp << std::endl;
        std::cout << "qp_scale: " << (float)info.quant_info.qp_scale << std::endl;
        std::cout << "limvals_min: " << (float)info.quant_info.limvals_min << std::endl;
        std::cout << "limvals_max: " << (float)info.quant_info.limvals_max << std::endl;
        std::cout << std::endl;
    }
}

hailo_status parse_hef(Hef hef)
{
    hailo_status status = HAILO_UNINITIALIZED;
    // Adding std::fixed to get float output without e.
    std::cout << std::fixed;

    auto input_vstream_info_exp = hef.get_input_vstream_infos();
    if (input_vstream_info_exp)
    {
        std::cout << "Input Vstreams: " << std::endl;
        std::cout << std::endl;
        parse_vstreams(input_vstream_info_exp.release());
        std::cout << std::endl;
    }
    else
    {
        status = input_vstream_info_exp.status();
        std::cerr << "Failed to parse hef vstream infos: " << status << std::endl;
    }
    auto output_vstream_info_exp = hef.get_output_vstream_infos();
    if (output_vstream_info_exp)
    {
        std::cout << "Output Vstreams: " << std::endl;
        std::cout << std::endl;
        parse_vstreams(output_vstream_info_exp.release());
        std::cout << std::endl;

    }
    else
    {
        status = input_vstream_info_exp.status();
        std::cerr << "Failed to parse hef vstream infos: " << status << std::endl;
    }
    auto bottleneck_fps_exp = hef.get_bottleneck_fps();
    if (bottleneck_fps_exp)
    {
        std::cout << "Expected FPS: " << bottleneck_fps_exp.release() << std::endl;
    }
    return status;
}

int main(int argc, char *argv[])
{
    hailo_status status = HAILO_UNINITIALIZED;

    // Parse user arguments
    cxxopts::Options options = build_arg_parser();
    auto result = options.parse(argc, argv);
    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        exit(0);
    }
    if (result.count("input"))
    {
        const std::string hef_path = result["input"].as<std::string>();
        auto hef_exp = Hef::create(hef_path);
        if (hef_exp)
        {
            status = parse_hef(hef_exp.release());
            if (!status)
            {
                std::cerr << "Failed to parse hef: " << status << std::endl;
            }
        }
        else
        {
            status = hef_exp.status();
            std::cerr << "Failed to parse hef: " << status << std::endl;
        }
    }

    return status;
}