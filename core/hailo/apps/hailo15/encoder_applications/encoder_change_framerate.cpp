#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <iostream>

#include "apps_common.hpp"

#define BIG_FRAMERATE (30)
#define SMALL_FRAMERATE (10)

static int counter=0;

/**
 * Update the framerate of the pipeline
 *
 * @param[in] pipeline         The pipeline as a GstElement.
 * @param[in] framerate        The new framerate.
 * @note updates the framerate of the pipeline.
 */
static void update_framerate(GstElement *pipeline, int framerate)
{
    GstElement *videofilter = gst_bin_get_by_name(GST_BIN(pipeline), "videofilter");
    GstCaps *filtercaps;

    g_object_get(G_OBJECT(videofilter), "caps", &filtercaps, NULL);
    filtercaps = gst_caps_make_writable(filtercaps);
    gst_caps_set_simple(filtercaps, "framerate", GST_TYPE_FRACTION, framerate, 1, NULL);
    g_object_set(G_OBJECT(videofilter), "caps", filtercaps, NULL);
    gst_caps_unref(filtercaps);
}

/**
 * Update the framerate of the pipeline every 200 frames
 *
 * @param[in] pad               The sinkpad of the encoder.
 * @param[in] info              Info about the probe
 * @param[in] user_data         user specified data for the probe
 * @return GST_PAD_PROBE_OK
 * @note This function is called every frame, and updates the framerate every 200 frames.
 */
static GstPadProbeReturn update_framerate_probe_callback(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    GstElement *pipeline = GST_ELEMENT(user_data);
    counter++;

    if (counter % 200 == 0) 
        {
        if (counter % 400 == 0) 
        {
            GST_INFO("Changing pipeline to %d fps", BIG_FRAMERATE);
            update_framerate(pipeline, BIG_FRAMERATE);
        }
        else
        {
            GST_INFO("Changing pipeline to %d fps", SMALL_FRAMERATE);
            update_framerate(pipeline, SMALL_FRAMERATE);
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

    pipeline = "v4l2src name=src_element num-buffers=2000 device=/dev/video0 io-mode=mmap ! "
               "video/x-raw,format=NV12,width=1920,height=1080,framerate=30/1 ! "
               "videorate name=videorate ! capsfilter name=videofilter caps=video/x-raw,framerate=30/1 ! "
               "queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! "
               "hailoh265enc name=enco ! h265parse config-interval=-1 ! tee name=t t. ! "
               "queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! "
               "fpsdisplaysink name=display_sink text-overlay=false video-sink=\"appsink name=hailo_sink\" sync=true t. ! "
               "queue leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 ! "
               "rtph265pay ! application/x-rtp, media=(string)video, encoding-name=(string)H265 ! "
               "udpsink host=10.0.0.2 port=5000 sync=true name=udp_sink";
                                           
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

    GstElement *display_sink = gst_bin_get_by_name(GST_BIN(pipeline), "display_sink");
    GstElement *appsink = gst_bin_get_by_name(GST_BIN(display_sink), "hailo_sink");
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
    GstElement *encoder = gst_bin_get_by_name(GST_BIN(pipeline), "videorate");
    // extract pads from elements
    GstPad *pad_encoder = gst_element_get_static_pad(encoder, "sink");
    // set probes
    gst_pad_add_probe(pad_encoder, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)update_framerate_probe_callback, pipeline, NULL);
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
