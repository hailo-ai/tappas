/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/**
 * SECTION:element-hailoaggregator
 * @title: hailoaggregator
 *
 * Takes packets from various input sinks into one output source.
 *
 */
#include "cropping/gsthailoaggregator.hpp"
#include <gst/video/video.h>
#include <iostream>
#include "gst_hailo_cropping_meta.hpp"
#include "hailo_objects.hpp"
#include "hailo_common.hpp"
#include "gst_hailo_meta.hpp"
using namespace std::chrono_literals;

GST_DEBUG_CATEGORY_STATIC(gst_hailoaggregator_debug);
#define GST_CAT_DEFAULT gst_hailoaggregator_debug

static void gst_hailoaggregator_post_aggregation(GstHailoAggregator *hailoaggregator, HailoROIPtr hailo_roi);
static void gst_hailoaggregator_handle_sub_frame_roi(GstHailoAggregator *hailoaggregator, HailoROIPtr sub_buffer_roi);
static GstStateChangeReturn gst_hailoaggregator_change_state(GstElement *element, GstStateChange transition);

#define DEFAULT_FORWARD_STICKY_EVENTS TRUE

enum
{
    PROP_0,
    PROP_FLATTEN_DETECTIONS,
};

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE("sink",
                                                                    GST_PAD_SINK,
                                                                    GST_PAD_ALWAYS,
                                                                    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE("src",
                                                                   GST_PAD_SRC,
                                                                   GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS_ANY);

#define _do_init \
    GST_DEBUG_CATEGORY_INIT(gst_hailoaggregator_debug, "hailoaggregator", 0, "hailoaggregator element");
#define gst_hailoaggregator_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstHailoAggregator, gst_hailoaggregator, GST_TYPE_ELEMENT, _do_init);

static void
gst_hailoaggregator_get_property(GObject *object, guint prop_id,
                                 GValue *value, GParamSpec *pspec);
static void
gst_hailoaggregator_set_property(GObject *object, guint prop_id,
                                 const GValue *value, GParamSpec *pspec);

static gboolean gst_hailoaggregator_sink_event(GstPad *pad,
                                               GstObject *parent,
                                               GstEvent *event);
static GstFlowReturn gst_hailoaggregator_chain_main(GstPad *pad, GstObject *parent, GstBuffer *buf);
static GstFlowReturn gst_hailoaggregator_chain_sub(GstPad *pad, GstObject *parent, GstBuffer *buf);

static gboolean gst_hailoaggregator_sink_query(GstPad *pad,
                                                 GstObject *parent, GstQuery *query);

static void
gst_hailoaggregator_class_init(GstHailoAggregatorClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);

    GstHailoAggregatorClass *hailoaggregator_class = GST_HAILO_AGGREGATOR_CLASS(klass);

    gobject_class->set_property = gst_hailoaggregator_set_property;
    gobject_class->get_property = gst_hailoaggregator_get_property;

    gst_element_class_set_static_metadata(gstelement_class,
                                          "hailoaggregator - Cascading",
                                          "Hailo/Tools",
                                          "Aggregates related detections to the original Image",
                                          "hailo.ai <contact@hailo.ai>");
    gst_element_class_add_static_pad_template(gstelement_class, &sink_template);
    gst_element_class_add_static_pad_template(gstelement_class, &src_template);

    hailoaggregator_class->handle_main_roi_post_aggregation = gst_hailoaggregator_post_aggregation;
    hailoaggregator_class->handle_sub_frame_roi = gst_hailoaggregator_handle_sub_frame_roi;
    gstelement_class->change_state = gst_hailoaggregator_change_state;

    g_object_class_install_property(gobject_class, PROP_FLATTEN_DETECTIONS,
                                    g_param_spec_boolean("flatten-detections", "Flatten detections", "perform a 'flattening' functionality on the detection metadata when receiving each frame", false,
                                                         (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
}

static void
gst_hailoaggregator_init(GstHailoAggregator *hailoaggregator)
{
    // Bypass Sinkpad
    hailoaggregator->sinkpad_main = gst_pad_new_from_static_template(&sink_template, "sink_0");
    gst_pad_set_event_function(hailoaggregator->sinkpad_main, GST_DEBUG_FUNCPTR(gst_hailoaggregator_sink_event));
    gst_pad_set_chain_function(hailoaggregator->sinkpad_main, GST_DEBUG_FUNCPTR(gst_hailoaggregator_chain_main));
    gst_pad_set_query_function(hailoaggregator->sinkpad_main, GST_DEBUG_FUNCPTR(gst_hailoaggregator_sink_query));
    GST_PAD_SET_PROXY_CAPS(hailoaggregator->sinkpad_main);
    gst_element_add_pad(GST_ELEMENT(hailoaggregator), hailoaggregator->sinkpad_main);

    // Other Sinkpad
    hailoaggregator->sinkpad_sub = gst_pad_new_from_static_template(&sink_template, "sink_1");
    gst_pad_set_event_function(hailoaggregator->sinkpad_sub, GST_DEBUG_FUNCPTR(gst_hailoaggregator_sink_event));
    gst_pad_set_chain_function(hailoaggregator->sinkpad_sub, GST_DEBUG_FUNCPTR(gst_hailoaggregator_chain_sub));
    GST_PAD_SET_PROXY_CAPS(hailoaggregator->sinkpad_sub);
    gst_element_add_pad(GST_ELEMENT(hailoaggregator), hailoaggregator->sinkpad_sub);

    // SrcPad
    hailoaggregator->srcpad = gst_pad_new_from_static_template(&src_template, "src");
    gst_pad_use_fixed_caps(hailoaggregator->srcpad);

    gst_element_add_pad(GST_ELEMENT(hailoaggregator), hailoaggregator->srcpad);
    hailoaggregator->num_of_frames = 0;
    hailoaggregator->mainframe = NULL;

    hailoaggregator->flatten_detections = false;
    hailoaggregator->eos_main = false;
    hailoaggregator->eos_sub = false;
}

static void
gst_hailoaggregator_set_property(GObject *object, guint prop_id,
                                 const GValue *value, GParamSpec *pspec)
{
    GstHailoAggregator *hailoaggregator = GST_HAILO_AGGREGATOR_CAST(object);

    switch (prop_id)
    {
    case PROP_FLATTEN_DETECTIONS:
        hailoaggregator->flatten_detections = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_hailoaggregator_get_property(GObject *object, guint prop_id, GValue *value,
                                 GParamSpec *pspec)
{
    GstHailoAggregator *hailoaggregator = GST_HAILO_AGGREGATOR_CAST(object);

    switch (prop_id)
    {
    case PROP_FLATTEN_DETECTIONS:
        g_value_set_boolean(value, hailoaggregator->flatten_detections);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/**
 * Callback for gst_pad_sticky_events_foreach, that forward this event to a given srcpad
 *
 * @param[in] pad         The pad holding the event.
 * @param[in] event       Pointer to the event
 * @param[in] user_data   The srcpad to send the event to.
 * @return Upon success, returns true. Otherwise, returns false.
 */
static gboolean
forward_events(GstPad *pad, GstEvent **event, gpointer user_data)
{
    GstPad *srcpad = GST_PAD_CAST(user_data);

    if (GST_EVENT_TYPE(*event) != GST_EVENT_EOS)
        return gst_pad_push_event(srcpad, gst_event_ref(*event));

    return TRUE;
}

static gboolean gst_hailoaggregator_sink_query(GstPad *pad,
                                                 GstObject *parent, GstQuery *query)
{
    GstHailoAggregator *hailoaggregator = GST_HAILO_AGGREGATOR_CAST(parent);
    gboolean ret = FALSE;
    switch (GST_QUERY_TYPE(query))
    {
    case GST_QUERY_ALLOCATION:
    {
        GST_DEBUG_OBJECT(hailoaggregator, "Received allocation query from sinkpad in hailoaggregator");
        ret = gst_pad_peer_query(hailoaggregator->srcpad, query);
        if (!ret)
            GST_DEBUG_OBJECT(hailoaggregator, "Failed to query peer for allocation");
        ret = true;
        break;
    }
    default:
    {
        /* just call the default handler */
        ret = gst_pad_query_default(pad, parent, query);
        break;
    }
    }
    GST_DEBUG_OBJECT(hailoaggregator, "Received query from sinkpad in hailo aggregator %s", GST_QUERY_TYPE_NAME(query));
    return ret;
}

/**
 * Checks whether all sinkpads got eos.
 *
 * @param[in] hailoaggregator         aggregator element to check
 * @param[in] event       Pointer to the event
 * @param[in] user_data   The srcpad to send the event to.
 * @return Upon success, returns true. Otherwise, returns false.
 */
static gboolean
gst_hailoaggregator_all_sinkpads_eos_unlocked(GstHailoAggregator *hailoaggregator)
{
    return (hailoaggregator->eos_main && hailoaggregator->eos_sub);
}

static void
gst_hailoaggregator_update_eos(GstHailoAggregator *hailoaggregator, GstPad *pad, bool eos)
{
    if (pad == hailoaggregator->sinkpad_main)
    {
        hailoaggregator->eos_main = eos;
    }
    else
    {
        hailoaggregator->eos_sub = eos;
    }
}

static gboolean
gst_hailoaggregator_sink_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
    GstHailoAggregator *hailoaggregator = GST_HAILO_AGGREGATOR_CAST(parent);
    gboolean forward = TRUE;
    gboolean res = TRUE;
    gboolean unlock = FALSE;

    GST_DEBUG_OBJECT(pad, "received event %" GST_PTR_FORMAT, event);

    if (GST_EVENT_IS_STICKY(event))
    {
        unlock = TRUE;
        GST_PAD_STREAM_LOCK(hailoaggregator->srcpad);

        if (GST_EVENT_TYPE(event) == GST_EVENT_EOS)
        {
            GST_OBJECT_LOCK(hailoaggregator);
            gst_hailoaggregator_update_eos(hailoaggregator, pad, true);
            forward = gst_hailoaggregator_all_sinkpads_eos_unlocked(hailoaggregator);
            GST_OBJECT_UNLOCK(hailoaggregator);
        }
        else if (pad != hailoaggregator->sinkpad_main)
        {
            forward = FALSE;
        }
    }
    else if (GST_EVENT_TYPE(event) == GST_EVENT_FLUSH_STOP)
    {
        unlock = TRUE;
        GST_PAD_STREAM_LOCK(hailoaggregator->srcpad);
        GST_OBJECT_LOCK(hailoaggregator);
        gst_hailoaggregator_update_eos(hailoaggregator, pad, false);
        GST_OBJECT_UNLOCK(hailoaggregator);
    }

    if (forward && GST_EVENT_IS_SERIALIZED(event))
    {
        /* If no data is coming and we receive serialized event, need to forward all sticky events.
         * Otherwise downstream has an inconsistent set of sticky events when
         * handling the new event. */
        if (!unlock)
        {
            unlock = TRUE;
            GST_PAD_STREAM_LOCK(hailoaggregator->srcpad);
        }

        if (hailoaggregator->sinkpad_main == pad)
        {
            gst_pad_sticky_events_foreach(pad, forward_events, hailoaggregator->srcpad);
        }
    }

    if (forward)
        res = gst_pad_push_event(hailoaggregator->srcpad, event);
    else
        gst_event_unref(event);

    if (unlock)
        GST_PAD_STREAM_UNLOCK(hailoaggregator->srcpad);

    return res;
}

static GstFlowReturn
gst_hailoaggregator_chain_sub(GstPad *pad, GstObject *parent, GstBuffer *buf)
{
    GstHailoAggregator *hailoaggregator = GST_HAILO_AGGREGATOR_CAST(parent);
    GstHailoAggregatorClass *hailoaggregator_class = GST_HAILO_AGGREGATOR_GET_CLASS(hailoaggregator);

    std::unique_lock<std::mutex> lock(hailoaggregator->mutex);

    // Wait until main frame is not null & the offset of main frame is not smaller.
    if (hailoaggregator->mainframe == NULL || hailoaggregator->mainframe->offset < buf->offset)
    {
        hailoaggregator->cv_sub.wait(lock);
    }

    if (hailoaggregator->mainframe != NULL)
    {
        HailoROIPtr sub_buffer_roi = get_hailo_main_roi(buf);
        hailoaggregator_class->handle_sub_frame_roi(hailoaggregator, sub_buffer_roi);

        // Increase the number of received frames.
        hailoaggregator->num_of_frames++;
        if (hailoaggregator->num_of_frames >= hailoaggregator->expected_frames)
        {
            // Release the mutex in the main chain if all the frames received.
            hailoaggregator->cv_main.notify_one();
            hailoaggregator->num_of_frames = 0;
        }
    }

    gst_buffer_remove_hailo_meta(buf);
    gst_buffer_unref(buf);
    return GST_FLOW_OK;
}

static GstFlowReturn
gst_hailoaggregator_chain_main(GstPad *pad, GstObject *parent, GstBuffer *buf)
{
    GstFlowReturn ret = GST_FLOW_ERROR;
    GstHailoAggregator *hailoaggregator = GST_HAILO_AGGREGATOR_CAST(parent);
    GstHailoAggregatorClass *hailoaggregator_class = GST_HAILO_AGGREGATOR_GET_CLASS(hailoaggregator);
    std::unique_lock<std::mutex> lock(hailoaggregator->mutex);

    // Get excpected frames from the main frame ROI
    HailoROIPtr hailo_roi = get_hailo_main_roi(buf);

    hailoaggregator->expected_frames = gst_buffer_get_hailo_cropping_meta(buf)->num_of_crops;

    if (hailoaggregator->expected_frames != 0)
    {
        hailoaggregator->mainframe = buf;
        hailoaggregator->cv_sub.notify_one();
        hailoaggregator->cv_main.wait(lock);
        hailoaggregator->mainframe = NULL;
    }
    lock.unlock();

    hailoaggregator_class->handle_main_roi_post_aggregation(hailoaggregator, hailo_roi);

    gst_pad_sticky_events_foreach(hailoaggregator->sinkpad_main, forward_events, hailoaggregator->srcpad);

    // Remove the cropping meta from the main frame.
    if (! gst_buffer_remove_hailo_cropping_meta(buf))
    {
        GST_ERROR_OBJECT(hailoaggregator, "Failed to remove cropping meta from main frame");
    }

    // Push main buffer into the src pad.
    ret = gst_pad_push(hailoaggregator->srcpad, buf);
    return ret;
}

/**
 * Functionality to perform for each incoming sub frame.
 * Called from the chain_sub method before the releasing the mutex and the buffers.
 * Flatten detections from sub_buffer_roi sub to main_buffer_roi's scale.
 * Assure main_buffer_roi will contain the detections and that each detection scale and location will match the main_buffer_roi.
 * 
 * @param[in] hailoaggregator   GstHailoAggregator.
 * @param[in] sub_buffer_roi    HailoROIPtr, the ROI of the subframe taken from the metadata of the buffer.
 * @return void.
 */
static void gst_hailoaggregator_handle_sub_frame_roi(GstHailoAggregator *hailoaggregator, HailoROIPtr sub_buffer_roi)
{
    if (hailoaggregator->flatten_detections)
    {
        // Get HailoRoi from the main frame.
        HailoROIPtr main_buffer_roi = get_hailo_main_roi(hailoaggregator->mainframe);
        // Flatten sub_buffer_roi sub detections to main_buffer_roi's scales.
        // Passing HAILO_DETECTION as a filter type here request to flatten only HailoDetection objects.
        hailo_common::flatten_hailo_roi(sub_buffer_roi, main_buffer_roi, HAILO_DETECTION);
    }
}

/**
 * Functionality to perform after all frames are aggregated succesfully.
 * Called from the chain_main method before releasing the mutex and the buffers.
 * Base implementation does nothing, derived elements can override.
 * 
 * @param[in] hailoaggregator   GstHailoAggregator.
 * @param[in] hailo_roi    HailoROIPtr, the ROI of the main frame taken from the metadata of the buffer.
 * @return void.
 */
static void gst_hailoaggregator_post_aggregation(GstHailoAggregator *hailoaggregator, HailoROIPtr hailo_roi) {}

static GstStateChangeReturn
gst_hailoaggregator_change_state(GstElement *element, GstStateChange transition)
{
    GstHailoAggregator *aggregator = GST_HAILO_AGGREGATOR(element);
    switch (transition)
    {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
    {
        // Unlocking both condition variables in order to finish the chain function.
        // After that the pads can be freed by the change_state of base class.
        aggregator->cv_main.notify_all();
        aggregator->cv_sub.notify_all();
        break;
    }
    default:
        break;
    }
    GstStateChangeReturn ret;

    ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    if (ret == GST_STATE_CHANGE_FAILURE)
        return ret;

    return ret;
}
