#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <cxxopts.hpp>
#include "apps_common.hpp"

const char *CONFIG_FILE_1 = "/home/root/apps/basic_security_camera_streaming/resources/configs/vision_config1.json";
const char *CONFIG_FILE_2 = "/home/root/apps/basic_security_camera_streaming/resources/configs/vision_config2.json";
const static uint cycle_frames_a = 200;
const static uint cycle_frames_b = 400;

static uint counter = 0;

std::string config_1 = "";
std::string config_2 = "";

/**
 * frontend's probe callback
 *
 * @param[in] pad               The sinkpad of the encoder.
 * @param[in] info              Info about the probe
 * @param[in] user_data         user specified data for the probe
 * @return GST_PAD_PROBE_OK
 * @note Example only - Switches between "CONFIG_1" and "CONFIG_2" every 200 frames, user_data is the pipeline.
 */
static GstPadProbeReturn probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    GstElement *pipeline = GST_ELEMENT(user_data);
    GstElement *element = gst_bin_get_by_name(GST_BIN(pipeline), "frontend");

    counter++;

    if (counter % cycle_frames_a == 0)
    {
        if (counter % cycle_frames_b != 0)
        {
            // Changing to configuration 2
            GST_INFO("Changing frontend to config 2");
            g_object_set(element, "config-string", config_2.c_str(), NULL);
        }
        else
        {
            // Changing to configuration 1
            GST_INFO("Changing frontend to config 1");
            g_object_set(element, "config-string", config_1.c_str(), NULL);
        }
    }

    gst_object_unref(element);
    return GST_PAD_PROBE_OK;
}

/**
 * Appsink's new_sample callback
 *
 * @param[in] appsink               The appsink object.
 * @param[in] callback_data         user data.
 * @return GST_FLOW_OK
 * @note Example only - only mapping the buffer to a GstMapInfo, than unmapping.
 */
static GstFlowReturn appsink_new_sample(GstAppSink *appsink, gpointer callback_data)
{
    GstSample *sample;
    GstBuffer *buffer;
    GstMapInfo mapinfo;

    sample = gst_app_sink_pull_sample(appsink);
    buffer = gst_sample_get_buffer(sample);
    gst_buffer_map(buffer, &mapinfo, GST_MAP_READ);

    GST_INFO_OBJECT(appsink, "Got Buffer from appsink: %p", mapinfo.data);
    // Do Logic

    gst_buffer_unmap(buffer, &mapinfo);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

/**
 * Create the gstreamer pipeline as string
 *
 * @return A string containing the gstreamer pipeline.
 * @note prints the return value to the stdout.
 */
std::string create_pipeline_string(std::string codec, std::string config_string)
{
    std::string pipeline = "";

    pipeline = "v4l2src name=src_element device=/dev/video0 io-mode=mmap ! "
               "video/x-raw, width=3840, height=2160, framerate=30/1, format=NV12 ! "
               "queue leaky=no max-size-buffers=2 max-size-bytes=0 max-size-time=0 ! "
               "hailofrontend config-string='" +
               config_string + "' name=frontend "
                               "frontend. ! "
                               "queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! "
                               "hailoh264enc bitrate=10000000 hrd=false ! video/x-h264 ! "
                               "tee name=stream1_tee "
                               "stream1_tee. ! "
                               "queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! "
                               "rtph264pay ! application/x-rtp, media=(string)video, encoding-name=(string)H264 ! "
                               "udpsink host=10.0.0.2 sync=false port=5000 "
                               "stream1_tee. ! "
                               "queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! "
                               "fpsdisplaysink name=display_sink1 text-overlay=false video-sink=\"appsink max-buffers=1 name=hailo_sink1\" sync=true signal-fps-measurements=true "
                               "frontend. ! "
                               "queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! "
                               "hailoh264enc bitrate=10000000 hrd=false ! video/x-h264 ! "
                               "tee name=stream2_tee "
                               "stream2_tee. ! "
                               "queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! "
                               "rtph264pay ! application/x-rtp, media=(string)video, encoding-name=(string)H264 ! "
                               "udpsink host=10.0.0.2 sync=false port=5002 "
                               "stream2_tee. ! "
                               "queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! "
                               "fpsdisplaysink name=display_sink2 text-overlay=false video-sink=\"appsink max-buffers=1 name=hailo_sink2\" sync=true signal-fps-measurements=true";
    return pipeline;
}

/**
 * Set the callbacks
 *
 * @param[in] pipeline        The pipeline as a GstElement.
 * @param[in] print_fps       To print FPS or not.
 * @note Sets the new_sample and propose_allocation callbacks, without callback user data (NULL).
 */
void set_callbacks(GstElement *pipeline, bool print_fps)
{
    GstAppSinkCallbacks callbacks = {NULL};
    callbacks.new_sample = appsink_new_sample;
    // Callback for stream 1
    GstElement *appsink1 = gst_bin_get_by_name(GST_BIN(pipeline), "hailo_sink1");
    gst_app_sink_set_callbacks(GST_APP_SINK(appsink1), &callbacks, NULL, NULL);
    gst_object_unref(appsink1);

    // Callback for stream 2
    GstElement *appsink2 = gst_bin_get_by_name(GST_BIN(pipeline), "hailo_sink2");
    gst_app_sink_set_callbacks(GST_APP_SINK(appsink2), &callbacks, NULL, NULL);
    gst_object_unref(appsink2);

    if (print_fps)
    {
        // Fps for stream 1
        GstElement *display_sink1 = gst_bin_get_by_name(GST_BIN(pipeline), "display_sink1");
        g_signal_connect(display_sink1, "fps-measurements", G_CALLBACK(fps_measurements_callback), NULL);
        gst_object_unref(display_sink1);

        // Fps for stream 2
        GstElement *display_sink2 = gst_bin_get_by_name(GST_BIN(pipeline), "display_sink2");
        g_signal_connect(display_sink2, "fps-measurements", G_CALLBACK(fps_measurements_callback), NULL);
        gst_object_unref(display_sink2);
    }
}

/**
 * Set the pipeline's probes
 *
 * @param[in] pipeline        The pipeline as a GstElement.
 * @note Sets a probe to the encoder sinkpad.
 */
void set_probes(GstElement *pipeline)
{
    // extract elements from pipeline
    GstElement *frontend = gst_bin_get_by_name(GST_BIN(pipeline), "frontend");
    // extract pads from elements
    GstPad *pad_frontend = gst_element_get_static_pad(frontend, "sink");
    // set probes
    gst_pad_add_probe(pad_frontend, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)probe_callback, pipeline, NULL);
    // free resources
    gst_object_unref(frontend);
}

std::string read_string_from_file(const char *file_path)
{
    std::ifstream file_to_read;
    file_to_read.open(file_path);
    if (!file_to_read.is_open())
        throw std::runtime_error("config path is not valid");
    std::string file_string((std::istreambuf_iterator<char>(file_to_read)), std::istreambuf_iterator<char>());
    file_to_read.close();
    std::cout << "Read config from file: " << file_path << std::endl;
    return file_string;
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

    for (ArgumentType argument : argument_handling_results)
    {
        switch (argument)
        {
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

    config_1 = read_string_from_file(CONFIG_FILE_1);
    config_2 = read_string_from_file(CONFIG_FILE_2);

    std::string pipeline_string = create_pipeline_string(codec, config_1);
    std::cout << "Created pipeline string." << std::endl;
    GstElement *pipeline = gst_parse_launch(pipeline_string.c_str(), NULL);
    std::cout << "Parsed pipeline." << std::endl;
    set_callbacks(pipeline, print_fps);
    set_probes(pipeline);
    std::cout << "Set probes and callbacks." << std::endl;
    std::cout << "Setting state to playing." << std::endl;
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    ret = wait_for_end_of_pipeline(pipeline);

    // Free resources
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    gst_deinit();

    return ret;
}
