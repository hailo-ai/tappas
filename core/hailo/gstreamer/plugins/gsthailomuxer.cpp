/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
/*
 * GStreamer Muxer element
 *
 * gsthailomuxer.cpp: Simple Muxer (N->1) element, waits on all sinks, passes the first one, with all metadata included.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gsthailomuxer.hpp"
#include "gst_hailo_meta.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_hailo_muxer_debug);
#define GST_CAT_DEFAULT gst_hailo_muxer_debug

GType gst_hailo_muxer_pad_get_type(void);
#define GST_TYPE_HAILO_MUXER_PAD \
    (gst_hailo_muxer_pad_get_type())
#define GST_HAILO_MUXER_PAD(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_MUXER_PAD, GstHailoMuxerPad))
#define GST_HAILO_MUXER_PAD_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_MUXER_PAD, GstHailoMuxerPadClass))
#define GST_IS_MUXER_PAD(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_MUXER_PAD))
#define GST_IS_MUXER_PAD_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_MUXER_PAD))
#define GST_HAILO_MUXER_PAD_CAST(obj) \
    ((GstHailoMuxerPad *)(obj))

typedef struct _GstHailoMuxerPad GstHailoMuxerPad;
typedef struct _GstHailoMuxerPadClass GstHailoMuxerPadClass;

struct _GstHailoMuxerPad
{
    GstPad parent;
    gboolean got_eos;
};

struct _GstHailoMuxerPadClass
{
    GstPadClass parent;
};

G_DEFINE_TYPE(GstHailoMuxerPad, gst_hailo_muxer_pad, GST_TYPE_PAD);

#define DEFAULT_FORWARD_STICKY_EVENTS TRUE

enum
{
    PROP_0,
};

static void
gst_hailo_muxer_pad_class_init(GstHailoMuxerPadClass *klass)
{
}

static void
gst_hailo_muxer_pad_init(GstHailoMuxerPad *pad)
{
    pad->got_eos = FALSE;
}

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE("sink",
                                                                    GST_PAD_SINK,
                                                                    GST_PAD_ALWAYS,
                                                                    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE("src",
                                                                   GST_PAD_SRC,
                                                                   GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS_ANY);

#define _do_init \
    GST_DEBUG_CATEGORY_INIT(gst_hailo_muxer_debug, "hailo_muxer", 0, "hailo_muxer element");
#define gst_hailo_muxer_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstHailoMuxer, gst_hailo_muxer, GST_TYPE_ELEMENT, _do_init);

static GstStateChangeReturn gst_hailo_muxer_change_state(GstElement *element,
                                                         GstStateChange transition);

static gboolean gst_hailo_muxer_collect_event(GstCollectPads *pads,
                                              GstCollectData *data,
                                              GstEvent *event,
                                              gpointer user_data);

static GstFlowReturn gst_hailo_muxer_collect_pads(GstCollectPads *pads,
                                                  gpointer user_data);

static gboolean gst_hailo_muxer_sink_event(GstPad *pad,
                                           GstObject *parent,
                                           GstEvent *event);

static void
gst_hailo_muxer_set_property(GObject *object, guint prop_id,
                             const GValue *value, GParamSpec *pspec)
{
    switch (prop_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_hailo_muxer_get_property(GObject *object, guint prop_id, GValue *value,
                             GParamSpec *pspec)
{
    switch (prop_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_hailo_muxer_dispose(GObject *object)
{
    GstHailoMuxer *muxer = GST_HAILO_MUXER_CAST(object);

    gst_object_replace((GstObject **)&muxer->main_sinkpad, NULL);
    gst_object_replace((GstObject **)&muxer->other_sinkpad, NULL);

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void
gst_hailo_muxer_class_init(GstHailoMuxerClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);

    gobject_class->set_property = gst_hailo_muxer_set_property;
    gobject_class->get_property = gst_hailo_muxer_get_property;
    gobject_class->dispose = GST_DEBUG_FUNCPTR(gst_hailo_muxer_dispose);

    gst_element_class_set_static_metadata(gstelement_class,
                                          "Muxer pipe fitting", "Generic", "2-to-1 pipe fitting",
                                          "hailo.ai <contact@hailo.ai>");

    gst_element_class_add_static_pad_template(gstelement_class, &sink_template);
    gst_element_class_add_static_pad_template(gstelement_class, &src_template);

    gstelement_class->change_state = GST_DEBUG_FUNCPTR(gst_hailo_muxer_change_state);
}

static void
gst_hailo_muxer_init(GstHailoMuxer *muxer)
{
    GstPadTemplate *templ = gst_static_pad_template_get(&sink_template);
    muxer->srcpad = gst_pad_new_from_static_template(&src_template, "src");
    gst_pad_use_fixed_caps(muxer->srcpad);

    gst_element_add_pad(GST_ELEMENT(muxer), muxer->srcpad);

    muxer->collect_pads = gst_collect_pads_new();
    gst_collect_pads_set_function(muxer->collect_pads, gst_hailo_muxer_collect_pads, muxer);
    gst_collect_pads_set_event_function(muxer->collect_pads, gst_hailo_muxer_collect_event, muxer);

    // Bypass Sinkpad
    muxer->main_sinkpad = GST_PAD_CAST(g_object_new(GST_TYPE_HAILO_MUXER_PAD,
                                                    "name", "sink_0", "direction", templ->direction, "template", templ,
                                                    NULL));
    // Other Sinkpad
    muxer->other_sinkpad = GST_PAD_CAST(g_object_new(GST_TYPE_HAILO_MUXER_PAD,
                                                     "name", "sink_1", "direction", templ->direction, "template", templ,
                                                     NULL));
    gst_element_add_pad(GST_ELEMENT(muxer), muxer->main_sinkpad);
    gst_element_add_pad(GST_ELEMENT(muxer), muxer->other_sinkpad);
    muxer->collect_data1 = gst_collect_pads_add_pad(muxer->collect_pads, muxer->main_sinkpad, sizeof(*muxer->collect_data1), nullptr, TRUE);
    muxer->collect_data2 = gst_collect_pads_add_pad(muxer->collect_pads, muxer->other_sinkpad, sizeof(*muxer->collect_data2), nullptr, TRUE);
}

static gboolean
forward_events(GstPad *pad, GstEvent **event, gpointer user_data)
{
    GstPad *srcpad = GST_PAD_CAST(user_data);

    if (GST_EVENT_TYPE(*event) != GST_EVENT_EOS)
        gst_pad_push_event(srcpad, gst_event_ref(*event));

    return TRUE;
}

static GstFlowReturn
gst_hailo_muxer_collect_pads(GstCollectPads *pads, gpointer user_data)
{
    GstHailoMuxer *muxer = GST_HAILO_MUXER_CAST(user_data);

    GST_DEBUG_OBJECT(muxer->collect_data1->pad, "Forwarding sticky events");
    gst_pad_sticky_events_foreach(muxer->collect_data1->pad, forward_events, muxer->srcpad);
    
    GstBuffer *buffer1 = gst_collect_pads_pop(pads, muxer->collect_data1);
    GstBuffer *buffer2 = gst_collect_pads_pop(pads, muxer->collect_data2);

    // Create a writable copy of buffer1.
    GstBuffer *out_buffer = gst_buffer_copy(buffer1);

    HailoROIPtr out_buffer_roi = get_hailo_main_roi(out_buffer, true);
    HailoROIPtr buffer2_roi = get_hailo_main_roi(buffer2, false);

    // Copy all hailo objects from buffer2_roi to buffer1_roi
    if (buffer2_roi)
    {
        for (auto obj : buffer2_roi->get_objects())
        {
            out_buffer_roi->add_object(obj);
        }
    }

    // Push out_buffer forward.
    GstFlowReturn ret = gst_pad_push(muxer->srcpad, out_buffer);

    // Unref buffer1 and buffer2 which are no longer needed.
    gst_buffer_unref(buffer1);
    gst_buffer_unref(buffer2);
    return ret;
}

static gboolean
gst_hailo_muxer_all_sinkpads_eos_unlocked(GstHailoMuxer *muxer, GstPad *pad)
{
    GstElement *element = GST_ELEMENT_CAST(muxer);
    GList *item;
    gboolean all_eos = FALSE;

    if (element->numsinkpads == 0)
        goto done;

    for (item = element->sinkpads; item != NULL; item = g_list_next(item))
    {
        GstHailoMuxerPad *sinkpad = GST_HAILO_MUXER_PAD_CAST(item->data);

        if (!sinkpad->got_eos)
        {
            return FALSE;
        }
    }

    all_eos = TRUE;

done:
    return all_eos;
}

static gboolean
gst_hailo_muxer_collect_event(GstCollectPads *pads,
                              GstCollectData *data,
                              GstEvent *event,
                              gpointer user_data)
{
    return gst_hailo_muxer_sink_event(data->pad, GST_OBJECT(user_data), event);
}

static gboolean
gst_hailo_muxer_sink_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
    GstHailoMuxer *muxer = GST_HAILO_MUXER_CAST(parent);
    GstHailoMuxerPad *fpad = GST_HAILO_MUXER_PAD_CAST(pad);
    gboolean forward = TRUE;
    gboolean res = TRUE;
    gboolean unlock = FALSE;

    GST_DEBUG_OBJECT(pad, "received event %" GST_PTR_FORMAT, event);

    if (GST_EVENT_IS_STICKY(event))
    {
        unlock = TRUE;
        GST_PAD_STREAM_LOCK(muxer->srcpad);

        if (GST_EVENT_TYPE(event) == GST_EVENT_EOS)
        {
            GST_OBJECT_LOCK(muxer);
            fpad->got_eos = TRUE;
            forward = gst_hailo_muxer_all_sinkpads_eos_unlocked(muxer, pad);
            GST_OBJECT_UNLOCK(muxer);
        }
        else if (pad != muxer->main_sinkpad)
        {
            forward = FALSE;
        }
    }
    else if (GST_EVENT_TYPE(event) == GST_EVENT_FLUSH_STOP)
    {
        unlock = TRUE;
        GST_PAD_STREAM_LOCK(muxer->srcpad);
        GST_OBJECT_LOCK(muxer);
        fpad->got_eos = FALSE;
        GST_OBJECT_UNLOCK(muxer);
    }

    if (forward && GST_EVENT_IS_SERIALIZED(event))
    {
        /* If no data is coming and we receive serialized event, need to forward all sticky events.
         * Otherwise downstream has an inconsistent set of sticky events when
         * handling the new event. */
        if (!unlock)
        {
            unlock = TRUE;
            GST_PAD_STREAM_LOCK(muxer->srcpad);
        }

        if ((muxer->main_sinkpad == pad))
        {
            gst_pad_sticky_events_foreach(pad, forward_events, muxer->srcpad);
        }
    }

    if (forward)
        res = gst_pad_push_event(muxer->srcpad, event);
    else
        gst_event_unref(event);

    if (unlock)
        GST_PAD_STREAM_UNLOCK(muxer->srcpad);

    return res;
}

static void
reset_pad(const GValue *data, gpointer user_data)
{
    GstPad *pad = GST_PAD_CAST(g_value_get_object(data));
    GstHailoMuxerPad *fpad = GST_HAILO_MUXER_PAD_CAST(pad);
    GstHailoMuxer *muxer = GST_HAILO_MUXER_CAST(user_data);

    GST_OBJECT_LOCK(muxer);
    fpad->got_eos = FALSE;
    GST_OBJECT_UNLOCK(muxer);
}

static GstStateChangeReturn
gst_hailo_muxer_change_state(GstElement *element, GstStateChange transition)
{
    GstStateChangeReturn ret;

    switch (transition)
    {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        gst_collect_pads_stop(GST_HAILO_MUXER_CAST(element)->collect_pads);
        break;
    default:
        break;
    }

    ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    if (ret == GST_STATE_CHANGE_FAILURE)
        return ret;

    switch (transition)
    {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        gst_collect_pads_start(GST_HAILO_MUXER_CAST(element)->collect_pads);
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
    {
        GstIterator *iter = gst_element_iterate_sink_pads(element);
        GstIteratorResult res;

        do
        {
            res = gst_iterator_foreach(iter, reset_pad, element);
            if (res == GST_ITERATOR_RESYNC)
                gst_iterator_resync(iter);
        } while (res == GST_ITERATOR_RESYNC);
        gst_iterator_free(iter);

        if (res == GST_ITERATOR_ERROR)
            return GST_STATE_CHANGE_FAILURE;
    }
    break;
    default:
        break;
    }

    return ret;
}
