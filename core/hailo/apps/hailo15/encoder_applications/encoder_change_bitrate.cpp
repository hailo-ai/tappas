#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <iostream>

#include "apps_common.hpp"

#define BITRATE_FOR_VBR (1000000)
#define TOL_MOVING_BITRATE_FOR_VBR (2000)
#define PICTURE_RC_OFF (0)
#define PICTURE_RC_ON (1)
#define BITRATE_FOR_CBR (40000000)
#define TOL_MOVING_BITRATE_FOR_CBR (0)

static int counter=0;

/**
 * Encoder's probe callback
 *
 * @param[in] pad               The sinkpad of the encoder.
 * @param[in] info              Info about the probe
 * @param[in] user_data         user specified data for the probe
 * @return GST_PAD_PROBE_OK
 * @note Example only - Switches between "CBR" and "VBR" every 200 frames, user_data is the pipeline.
 */
static GstPadProbeReturn encoder_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    GstElement *pipeline = GST_ELEMENT(user_data);
    GstElement *encoder_element = gst_bin_get_by_name(GST_BIN(pipeline), "enco");

    counter++;

    if (counter % 200 == 0) {
        if (counter % 400 != 0) {
	        // Changing to VBR
            GST_INFO("Changing encoder to VBR");
            g_object_set(encoder_element, "tol-moving-bitrate", TOL_MOVING_BITRATE_FOR_VBR, NULL);
            g_object_set(encoder_element, "bitrate", BITRATE_FOR_VBR, NULL);
            g_object_set(encoder_element, "picture-rc", PICTURE_RC_OFF, NULL);
        }
        else
        {
            // Changing to CBR
            GST_INFO("Changing encoder to CBR");
            g_object_set(encoder_element, "tol-moving-bitrate", TOL_MOVING_BITRATE_FOR_CBR, NULL);
            g_object_set(encoder_element, "bitrate", BITRATE_FOR_CBR, NULL);
            g_object_set(encoder_element, "picture-rc", PICTURE_RC_ON, NULL);
        }
    }

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
static GstFlowReturn appsink_new_sample(GstAppSink * appsink, gpointer callback_data)
{
    GstSample *sample;
    GstBuffer *buffer;
    GstMapInfo mapinfo;

    sample = gst_app_sink_pull_sample(appsink);
    buffer = gst_sample_get_buffer(sample);
    gst_buffer_map(buffer, &mapinfo, GST_MAP_READ);

    GST_INFO_OBJECT(appsink, "Got Buffer from appsink: %p", mapinfo.data);
    // Do Logic

    gst_buffer_unmap(buffer,&mapinfo);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

/**
 * Create the gstreamer pipeline as string
 *
 * @return A string containing the gstreamer pipeline.
 * @note prints the return value to the stdout.
 */
std::string create_pipeline_string()
{
    std::string pipeline = "";
    std::string encoder_arguments;

    encoder_arguments = "tol-moving-bitrate=" + std::to_string(TOL_MOVING_BITRATE_FOR_CBR) + " "
                        "picture-rc=" + std::to_string(PICTURE_RC_ON) + " "
                        "bitrate=" + std::to_string(BITRATE_FOR_CBR) + " "
                        "ctb-rc=1";
                        

    pipeline = "v4l2src name=src_element num-buffers=900 device=/dev/video0 io-mode=mmap ! "
               "video/x-raw,format=NV12,width=1920,height=1080, framerate=30/1 ! "
               "queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! "
               "hailoh265enc name=enco " + encoder_arguments + " ! h265parse config-interval=-1 ! "
               "queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! "
               "video/x-h265,framerate=30/1 ! appsink wait-on-eos=false name=hailo_sink";
                                           
    std::cout << "Pipeline:" << std::endl;
    std::cout << "gst-launch-1.0 " << pipeline << std::endl;

    return pipeline;
}

/**
 * Set the Appsink callbacks
 *
 * @param[in] pipeline        The pipeline as a GstElement.
 * @note Sets the new_sample and propose_allocation callbacks, without callback user data (NULL).
 */
void set_callbacks(GstElement *pipeline)
{
    GstAppSinkCallbacks callbacks={NULL};

    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "hailo_sink");
    callbacks.new_sample = appsink_new_sample;

    gst_app_sink_set_callbacks(GST_APP_SINK(appsink), &callbacks, NULL, NULL);
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
}

int main(int argc, char *argv[])
{
    std::string src_pipeline_string;
    GstFlowReturn ret;

    gst_init(&argc, &argv);

    std::string pipeline_string = create_pipeline_string();
    GstElement *pipeline = gst_parse_launch(pipeline_string.c_str(), NULL);
    set_callbacks(pipeline);
    set_probes(pipeline);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    ret = wait_for_end_of_pipeline(pipeline);

    // Free resources
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_deinit();
    gst_object_unref(pipeline);

    return ret;
}
