#include <gst/gst.h>
#include <gst/video/video.h>
#include <iostream>
#include <cxxopts.hpp>
#include "apps_common.hpp"

static int counter=0;

/**
 * Encoder's probe callback
 *
 * @param[in] pad               The sinkpad of the encoder.
 * @param[in] info              Info about the probe
 * @param[in] user_data         user specified data for the probe
 * @return GST_PAD_PROBE_OK
 * @note Example only - Forces Keyframe every 15 frames.
 */
static GstPadProbeReturn encoder_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    GstElement *pipeline = GST_ELEMENT(user_data);
    GstElement *encoder_element = gst_bin_get_by_name(GST_BIN(pipeline), "enco");
    GstEvent * event;

    if (counter % 10 == 0) {
        GST_WARNING_OBJECT(encoder_element, "Force Keyframe from application");
        event = gst_video_event_new_downstream_force_key_unit(GST_CLOCK_TIME_NONE, GST_CLOCK_TIME_NONE, GST_CLOCK_TIME_NONE, TRUE, 1);
        if (!gst_pad_send_event(pad, event))
        {
            GST_ERROR_OBJECT(encoder_element, "Failed to send force key unit event to encoder");
        }
    }
    counter++;

    gst_object_unref(encoder_element);
    return GST_PAD_PROBE_OK;
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
    std::string encoder_arguments;

    pipeline = "v4l2src name=src_element num-buffers=300 device=/dev/video0 io-mode=mmap ! "
               "video/x-raw,format=NV12,width=1920,height=1080, framerate=30/1 ! "
               "queue name=q0 leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! "
               "hailo" + codec + "enc name=enco ! "
               "" + codec + "parse config-interval=-1 ! video/x-" + codec + ",framerate=30/1 ! "
               "queue name=q1 leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! "
               "fpsdisplaysink name=display_sink text-overlay=false video-sink=\"filesink location=force_keyframe.hevc name=hailo_sink\" sync=true signal-fps-measurements=true";

                                           
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
    if (print_fps)
    {
        GstElement *display_sink = gst_bin_get_by_name(GST_BIN(pipeline), "display_sink");
        g_signal_connect(display_sink, "fps-measurements", G_CALLBACK(fps_measurements_callback), NULL);
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
    GstElement *encoder = gst_bin_get_by_name(GST_BIN(pipeline), "enco");
    // extract pads from elements
    GstPad *pad_encoder = gst_element_get_static_pad(encoder, "sink");
    // set probes
    gst_pad_add_probe(pad_encoder, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)encoder_probe_callback, pipeline, NULL);
    // free resources
    gst_object_unref(encoder);
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
    set_probes(pipeline);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    ret = wait_for_end_of_pipeline(pipeline);

    // Free resources
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    gst_deinit();

    return ret;
}
