#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
#include <iostream>
#include <cxxopts.hpp>

#include "apps_common.hpp"

/**
 * Appsink's propose_allocation callback - Adding an GST_VIDEO_META_API_TYPE allocation meta
 *
 * @param[in] appsink               The appsink object.
 * @param[in] appsink               The allocation query.
 * @param[in] callback_data         user data.
 * @return TRUE
 * @note The adding of allocation meta is required to work with v4l2src without it copying each buffer.
 */
static gboolean appsink_propose_allocation(GstAppSink * appsink, GstQuery * query, gpointer callback_data)
{
    gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, NULL);
    return TRUE;
}

/**
 * Appsink's new_sample callback
 *
 * @param[in] appsink               The appsink object.
 * @param[in] callback_data         user data.
 * @return GST_FLOW_OK
 * @note Example only - only mapping the buffer to a GstVideoFrame, than unmapping.
 */
static GstFlowReturn appsink_new_sample(GstAppSink * appsink, gpointer callback_data)
{
    GstSample *sample;
    GstBuffer *buffer;
    GstCaps * caps;
    GstVideoInfo *info = gst_video_info_new();
    GstVideoFrame frame;

    sample = gst_app_sink_pull_sample(appsink);
    buffer = gst_sample_get_buffer(sample);
    caps = gst_sample_get_caps(sample);
    gst_video_info_from_caps(info, caps);
    gst_video_frame_map(&frame, info, buffer, GstMapFlags(GST_MAP_READ));
    
    GST_INFO("Got buffer video map"); 
    // Do Logic

    gst_video_frame_unmap(&frame);
    gst_video_info_free(info);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

/**
 * Create the gstreamer pipeline as string
 *
 * @return A string containing the gstreamer pipeline.
 * @note prints the return value to the stdout.
 */
std::string create_pipeline_string(std::string codec)
{
    std::string pipeline = "";

    pipeline = "v4l2src name=src_element num-buffers=500 device=/dev/video0 io-mode=mmap ! "
                "video/x-raw,format=NV12,width=3840,height=2160, framerate=30/1 ! "
                "tee name=t t. ! "
                "queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! "
                "hailo" + codec + "enc ! " + codec + "parse config-interval=-1 ! "
                "video/x-" + codec + ",framerate=30/1 ! filesink location=test.hevc "
                "t. ! queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! "
                "fpsdisplaysink name=display_sink text-overlay=false video-sink=\"appsink name=hailo_sink\" sync=true signal-fps-measurements=true";
                                           
    std::cout << "Pipeline:" << std::endl;
    std::cout << "gst-launch-1.0 " << pipeline << std::endl;

    return pipeline;
}

/**
 * Set the Appsink callbacks
 *
 * @param[in] pipeline        The pipeline as a GstElement.
 * @param[in] print_fps       To print FPS or not.
 * @note Sets the new_sample and propose_allocation callbacks, without callback user data (NULL).
 */
void set_callbacks(GstElement *pipeline, bool print_fps)
{
    GstAppSinkCallbacks callbacks={NULL};

    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "hailo_sink");
    callbacks.new_sample = appsink_new_sample;
    callbacks.propose_allocation = appsink_propose_allocation;

    gst_app_sink_set_callbacks(GST_APP_SINK(appsink), &callbacks, NULL, NULL);
    gst_object_unref(appsink);

    if (print_fps)
    {
        GstElement *display_sink = gst_bin_get_by_name(GST_BIN(pipeline), "display_sink");
        g_signal_connect(display_sink, "fps-measurements", G_CALLBACK(fps_measurements_callback), NULL);
    }
}

int main(int argc, char *argv[])
{
    std::string src_pipeline_string;
    GstFlowReturn ret;
    add_sigint_handler();
    std::string codec;
    bool print_fps = false;

    // Parse user arguments
    cxxopts::Options options = build_arg_parser();
    auto result = options.parse(argc, argv);
    std::vector<ArgumentType> argument_handling_results = handle_arguments(result, options, codec);

    for (ArgumentType argument: argument_handling_results)
    {
        switch (argument) {
            case ArgumentType::Help:
                return 0;
            case ArgumentType::Codec:
                break;
            case ArgumentType::PrintFPS:
                print_fps = true;
                break;
            case ArgumentType::Error:
                return 1;
        }
    }

    gst_init(&argc, &argv);

    std::string pipeline_string = create_pipeline_string(codec);
    GstElement *pipeline = gst_parse_launch(pipeline_string.c_str(), NULL);
    set_callbacks(pipeline, print_fps);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    ret = wait_for_end_of_pipeline(pipeline);

    // Free resources
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    gst_deinit();

    return ret;
}
