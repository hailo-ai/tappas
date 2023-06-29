/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
/**
 * SECTION:element-hailomuxer
 * @title: hailomuxer
 *
 * Takes packets from various input sinks into one output source.
 *
 */
#include "gsthailomuxer.hpp"
#include "gst_hailo_meta.hpp"
#include "gst_hailo_counter_meta.hpp"
#include <gst/video/video.h>
#include <iostream>
#include "gst_hailo_cropping_meta.hpp"
#include "hailo_objects.hpp"
#include "hailo_common.hpp"
using namespace std::chrono_literals;

GST_DEBUG_CATEGORY_STATIC(gst_hailomuxer_debug);
#define GST_CAT_DEFAULT gst_hailomuxer_debug

static void gst_hailomuxer_handle_sub_frame_roi(HailoROIPtr main_buffer_roi, HailoROIPtr sub_buffer_roi);
static GstStateChangeReturn gst_hailomuxer_change_state(GstElement *element, GstStateChange transition);

#define DEFAULT_FORWARD_STICKY_EVENTS TRUE

enum
{
    PROP_0,
    PROP_SYNC_COUNTERS,
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
    GST_DEBUG_CATEGORY_INIT(gst_hailomuxer_debug, "hailomuxer", 0, "hailomuxer element");
#define gst_hailomuxer_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstHailoMuxer, gst_hailomuxer, GST_TYPE_ELEMENT, _do_init);

static void
gst_hailomuxer_get_property(GObject *object, guint prop_id,
                            GValue *value, GParamSpec *pspec);
static void
gst_hailomuxer_set_property(GObject *object, guint prop_id,
                            const GValue *value, GParamSpec *pspec);

static gboolean gst_hailomuxer_sink_event(GstPad *pad,
                                          GstObject *parent,
                                          GstEvent *event);
static GstFlowReturn gst_hailomuxer_chain_main(GstPad *pad, GstObject *parent, GstBuffer *buf);
static GstFlowReturn gst_hailomuxer_chain_sub(GstPad *pad, GstObject *parent, GstBuffer *buf);
static GstFlowReturn gst_hailomuxer_sync_and_drop(GstBuffer *buf, GstHailoMuxer *hailomuxer, bool main_stream);
static void gst_hailomuxer_merge_rois(GstBuffer *buf, GstHailoMuxer *hailomuxer);
static void gst_hailomuxer_wait_for_main(GstHailoMuxer *hailomuxer, std::unique_lock<std::mutex> &lock);
static void gst_hailomuxer_wait_for_sub(GstBuffer *buf, GstHailoMuxer *hailomuxer, std::unique_lock<std::mutex> &lock);

static void
gst_hailomuxer_class_init(GstHailoMuxerClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);

    GstHailoMuxerClass *hailomuxer_class = GST_HAILO_MUXER_CLASS(klass);

    gobject_class->set_property = gst_hailomuxer_set_property;
    gobject_class->get_property = gst_hailomuxer_get_property;

    gst_element_class_set_static_metadata(gstelement_class,
                                          "Muxer pipeline merging",
                                          "Hailo/Tools",
                                          "2-to-1 pipeline merging",
                                          "hailo.ai <contact@hailo.ai>");
    gst_element_class_add_static_pad_template(gstelement_class, &sink_template);
    gst_element_class_add_static_pad_template(gstelement_class, &src_template);

    hailomuxer_class->handle_sub_frame_roi = gst_hailomuxer_handle_sub_frame_roi;
    gstelement_class->change_state = gst_hailomuxer_change_state;

    g_object_class_install_property(gobject_class, PROP_SYNC_COUNTERS,
                                    g_param_spec_boolean("sync-counters", "sync-counters", "Sync frames by matching HailoCounterMeta (see HailoCounter element)", false,
                                                         (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
}

static void
gst_hailomuxer_init(GstHailoMuxer *hailomuxer)
{
    // Bypass Sinkpad
    hailomuxer->sinkpad_main = gst_pad_new_from_static_template(&sink_template, "sink_0");
    gst_pad_set_event_function(hailomuxer->sinkpad_main, GST_DEBUG_FUNCPTR(gst_hailomuxer_sink_event));
    gst_pad_set_chain_function(hailomuxer->sinkpad_main, GST_DEBUG_FUNCPTR(gst_hailomuxer_chain_main));
    GST_PAD_SET_PROXY_CAPS(hailomuxer->sinkpad_main);
    gst_element_add_pad(GST_ELEMENT(hailomuxer), hailomuxer->sinkpad_main);

    // Other Sinkpad
    hailomuxer->sinkpad_sub = gst_pad_new_from_static_template(&sink_template, "sink_1");
    gst_pad_set_event_function(hailomuxer->sinkpad_sub, GST_DEBUG_FUNCPTR(gst_hailomuxer_sink_event));
    gst_pad_set_chain_function(hailomuxer->sinkpad_sub, GST_DEBUG_FUNCPTR(gst_hailomuxer_chain_sub));
    GST_PAD_SET_PROXY_CAPS(hailomuxer->sinkpad_sub);
    gst_element_add_pad(GST_ELEMENT(hailomuxer), hailomuxer->sinkpad_sub);

    // SrcPad
    hailomuxer->srcpad = gst_pad_new_from_static_template(&src_template, "src");
    gst_pad_use_fixed_caps(hailomuxer->srcpad);

    gst_element_add_pad(GST_ELEMENT(hailomuxer), hailomuxer->srcpad);
    hailomuxer->mainframe = NULL;
    hailomuxer->current_counter_main = 0;
    hailomuxer->current_counter_sub = 0;

    hailomuxer->eos_main = false;
    hailomuxer->eos_sub = false;
}

static void
gst_hailomuxer_set_property(GObject *object, guint prop_id,
                            const GValue *value, GParamSpec *pspec)
{
    GstHailoMuxer *hailomuxer = GST_HAILO_MUXER_CAST(object);

    switch (prop_id)
    {
    case PROP_SYNC_COUNTERS:
        hailomuxer->sync_counters = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_hailomuxer_get_property(GObject *object, guint prop_id, GValue *value,
                            GParamSpec *pspec)
{
    GstHailoMuxer *hailomuxer = GST_HAILO_MUXER_CAST(object);

    switch (prop_id)
    {
    case PROP_SYNC_COUNTERS:
        g_value_set_boolean(value, hailomuxer->sync_counters);
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

/**
 * Checks whether all sinkpads got eos.
 *
 * @param[in] hailomuxer         muxer element to check
 * @param[in] event       Pointer to the event
 * @param[in] user_data   The srcpad to send the event to.
 * @return Upon success, returns true. Otherwise, returns false.
 */
static gboolean
gst_hailomuxer_all_sinkpads_eos_unlocked(GstHailoMuxer *hailomuxer)
{
    return (hailomuxer->eos_main && hailomuxer->eos_sub);
}

static void
gst_hailomuxer_update_eos(GstHailoMuxer *hailomuxer, GstPad *pad, bool eos)
{
    if (pad == hailomuxer->sinkpad_main)
    {
        hailomuxer->eos_main = eos;
    }
    else
    {
        hailomuxer->eos_sub = eos;
    }
}

static gboolean
gst_hailomuxer_sink_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
    GstHailoMuxer *hailomuxer = GST_HAILO_MUXER_CAST(parent);
    gboolean forward = TRUE;
    gboolean res = TRUE;
    gboolean unlock = FALSE;

    GST_DEBUG_OBJECT(pad, "received event %" GST_PTR_FORMAT, event);

    if (GST_EVENT_IS_STICKY(event))
    {
        unlock = TRUE;
        GST_PAD_STREAM_LOCK(hailomuxer->srcpad);

        if (GST_EVENT_TYPE(event) == GST_EVENT_EOS)
        {
            GST_OBJECT_LOCK(hailomuxer);
            gst_hailomuxer_update_eos(hailomuxer, pad, true);
            forward = gst_hailomuxer_all_sinkpads_eos_unlocked(hailomuxer);
            GST_OBJECT_UNLOCK(hailomuxer);
        }
        else if (pad != hailomuxer->sinkpad_main)
        {
            forward = FALSE;
        }
    }
    else if (GST_EVENT_TYPE(event) == GST_EVENT_FLUSH_STOP)
    {
        unlock = TRUE;
        GST_PAD_STREAM_LOCK(hailomuxer->srcpad);
        GST_OBJECT_LOCK(hailomuxer);
        gst_hailomuxer_update_eos(hailomuxer, pad, false);
        GST_OBJECT_UNLOCK(hailomuxer);
    }

    if (forward && GST_EVENT_IS_SERIALIZED(event))
    {
        /* If no data is coming and we receive serialized event, need to forward all sticky events.
         * Otherwise downstream has an inconsistent set of sticky events when
         * handling the new event. */
        if (!unlock)
        {
            unlock = TRUE;
            GST_PAD_STREAM_LOCK(hailomuxer->srcpad);
        }

        if (hailomuxer->sinkpad_main == pad)
        {
            gst_pad_sticky_events_foreach(pad, forward_events, hailomuxer->srcpad);
        }
    }

    if (forward)
        res = gst_pad_push_event(hailomuxer->srcpad, event);
    else
        gst_event_unref(event);

    if (unlock)
        GST_PAD_STREAM_UNLOCK(hailomuxer->srcpad);

    return res;
}

static GstFlowReturn
gst_hailomuxer_sync_and_drop(GstBuffer *buf, GstHailoMuxer *hailomuxer, bool main_stream)
{
    // Update the relevant stream counter
    if (main_stream)
        hailomuxer->current_counter_main = gst_buffer_get_hailo_counter_meta(buf)->counter;
    else
        hailomuxer->current_counter_sub = gst_buffer_get_hailo_counter_meta(buf)->counter;

    // syncing main stream: If the sub stream is ahead of the main stream, then drop this frame
    // or
    // syncing sub stream: If the main stream is ahead of the sub stream, then drop this frame
    if ((main_stream && (hailomuxer->current_counter_main < hailomuxer->current_counter_sub)) || (!main_stream && (hailomuxer->current_counter_main > hailomuxer->current_counter_sub)))
    {
        gst_buffer_remove_hailo_meta(buf);
        gst_buffer_unref(buf);
        return GST_FLOW_OK;
    }

    return GST_FLOW_CUSTOM_SUCCESS;
}

static void
gst_hailomuxer_wait_for_main(GstHailoMuxer *hailomuxer, std::unique_lock<std::mutex> &lock)
{
    // If the sub stream is ahead of the main stream, then wait for it to catch up
    if (hailomuxer->current_counter_main < hailomuxer->current_counter_sub)
        hailomuxer->cv_sub.wait(lock);
}

static void
gst_hailomuxer_wait_for_sub(GstBuffer *buf, GstHailoMuxer *hailomuxer, std::unique_lock<std::mutex> &lock)
{
    hailomuxer->mainframe = buf;
    hailomuxer->cv_sub.notify_one();
    hailomuxer->cv_main.wait(lock);
    hailomuxer->mainframe = NULL;
    lock.unlock();
}

static void
gst_hailomuxer_merge_rois(GstBuffer *buf, GstHailoMuxer *hailomuxer)
{
    GstHailoMuxerClass *hailomuxer_class = GST_HAILO_MUXER_GET_CLASS(hailomuxer);
    if (hailomuxer->mainframe != NULL)
    {
        HailoROIPtr main_buffer_roi = get_hailo_main_roi(hailomuxer->mainframe, true);
        HailoROIPtr sub_buffer_roi = get_hailo_main_roi(buf);
        hailomuxer_class->handle_sub_frame_roi(main_buffer_roi, sub_buffer_roi);

        // Release the mutex in the main chain if all the objects are handled.
        hailomuxer->cv_main.notify_one();
    }
}

static GstFlowReturn
gst_hailomuxer_chain_sub(GstPad *pad, GstObject *parent, GstBuffer *buf)
{
    GstHailoMuxer *hailomuxer = GST_HAILO_MUXER_CAST(parent);
    std::unique_lock<std::mutex> lock(hailomuxer->mutex);

    // Avoid locking on eos
    if (hailomuxer->eos_main)
        return GST_FLOW_OK;

    if (hailomuxer->sync_counters)
    {
        GstFlowReturn status = gst_hailomuxer_sync_and_drop(buf, hailomuxer, false);
        if (status == GST_FLOW_OK)
            return GST_FLOW_OK;

        gst_hailomuxer_wait_for_main(hailomuxer, lock); // Wait for the main stream if needed
    }
    else if (hailomuxer->mainframe == NULL)
    {
        // Wait until main frame is not null & the offset of main frame is not smaller.
        hailomuxer->cv_sub.wait(lock);
    }

    gst_hailomuxer_merge_rois(buf, hailomuxer);
    hailomuxer->mainframe = NULL;
    gst_buffer_remove_hailo_meta(buf);
    gst_buffer_unref(buf);
    return GST_FLOW_OK;
}

static GstFlowReturn
gst_hailomuxer_chain_main(GstPad *pad, GstObject *parent, GstBuffer *buf)
{
    GstFlowReturn ret = GST_FLOW_ERROR;
    GstHailoMuxer *hailomuxer = GST_HAILO_MUXER_CAST(parent);
    std::unique_lock<std::mutex> lock(hailomuxer->mutex);

    // Avoid locking on eos
    if (hailomuxer->eos_sub)
        return GST_FLOW_OK;

    if (hailomuxer->sync_counters)
    {
        GstFlowReturn status = gst_hailomuxer_sync_and_drop(buf, hailomuxer, true);
        if (status == GST_FLOW_OK)
            return GST_FLOW_OK;
    }

    gst_hailomuxer_wait_for_sub(buf, hailomuxer, lock);

    gst_pad_sticky_events_foreach(hailomuxer->sinkpad_main, forward_events, hailomuxer->srcpad);

    // Remove the counter meta from the main frame.
    if (hailomuxer->sync_counters && (!gst_buffer_remove_hailo_counter_meta(buf)))
        GST_ERROR_OBJECT(hailomuxer, "Failed to remove counter meta from main frame");

    // Push main buffer into the src pad.
    ret = gst_pad_push(hailomuxer->srcpad, buf);
    return ret;
}

/**
 * Functionality to perform for each incoming sub frame.
 * Called from the chain_sub method before the releasing the mutex and the buffers.
 * Add objects from sub_buffer_roi sub to main_buffer_roi.
 *
 * @param[in] hailomuxer   GstHailoMuxer.
 * @param[in] sub_buffer_roi    HailoROIPtr, the ROI of the subframe taken from the metadata of the buffer.
 * @return void.
 */
static void gst_hailomuxer_handle_sub_frame_roi(HailoROIPtr main_buffer_roi, HailoROIPtr sub_buffer_roi)
{
    // Copy all hailo objects from buffer2_roi to buffer1_roi
    if (sub_buffer_roi)
    {
        for (auto obj : sub_buffer_roi->get_objects())
        {
            main_buffer_roi->add_object(obj);
        }
    }
}

static GstStateChangeReturn
gst_hailomuxer_change_state(GstElement *element, GstStateChange transition)
{
    GstHailoMuxer *muxer = GST_HAILO_MUXER(element);
    switch (transition)
    {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
    {
        // Unlocking both condition variables in order to finish the chain function.
        // After that the pads can be freed by the change_state of base class.
        muxer->cv_main.notify_all();
        muxer->cv_sub.notify_all();
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
