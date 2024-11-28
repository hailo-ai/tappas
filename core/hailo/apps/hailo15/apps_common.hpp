#pragma once
#include <gst/gst.h>
#include <cxxopts/cxxopts.hpp>

#include <iostream>
#include <map>
#include <utility> // For std::pair

enum class ArgumentType {
    Help,
    Codec,
    PrintFPS,
    Error
};

enum class StreamResolution {
    SD,    // 640x360
    ED,    // 720x480
    HD,    // 1280x720
    FHD,   // 1920x1080
    QHD,   // 2592x1944
    UHD_4K // 3840x2160
};

const std::map<StreamResolution, std::pair<int, int>> resolutionMap = {
    {StreamResolution::SD, {640, 360}},
    {StreamResolution::ED, {720, 480}},
    {StreamResolution::HD, {1280, 720}},
    {StreamResolution::FHD, {1920, 1080}},
    {StreamResolution::QHD, {2592, 1944}},
    {StreamResolution::UHD_4K, {3840, 2160}}
};

GstFlowReturn wait_for_end_of_pipeline(GstElement *pipeline);
void add_sigint_handler(void);
void print_help(const cxxopts::Options &options);
cxxopts::Options build_arg_parser();
void fps_measurements_callback(GstElement *fpsdisplaysink, gdouble fps, gdouble droprate, gdouble avgfps, gpointer udata);
std::vector<ArgumentType> handle_arguments(const cxxopts::ParseResult &result, const cxxopts::Options &options, std::string &codec);


