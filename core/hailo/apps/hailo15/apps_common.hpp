#pragma once
#include <gst/gst.h>
#include <cxxopts.hpp>

enum class ArgumentType {
    Help,
    Codec,
    PrintFPS,
    Error
};

GstFlowReturn wait_for_end_of_pipeline(GstElement *pipeline);
void add_sigint_handler(void);
void print_help(const cxxopts::Options &options);
cxxopts::Options build_arg_parser();
void fps_measurements_callback(GstElement *fpsdisplaysink, gdouble fps, gdouble droprate, gdouble avgfps, gpointer udata);
std::vector<ArgumentType> handle_arguments(const cxxopts::ParseResult &result, const cxxopts::Options &options, std::string &codec);
