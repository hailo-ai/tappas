/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include "gsthailocounter.hpp"
#include "gst_hailo_counter_meta.hpp"
#include <gst/gst.h>
#include <dlfcn.h>
#include <map>
#include <iostream>

GST_DEBUG_CATEGORY_STATIC(gst_hailocounter_debug_category);
#define GST_CAT_DEFAULT gst_hailocounter_debug_category

static void gst_hailocounter_set_property(GObject *object,
                                          guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_hailocounter_get_property(GObject *object,
                                          guint property_id, GValue *value, GParamSpec *pspec);
static void gst_hailocounter_dispose(GObject *object);
static void gst_hailocounter_finalize(GObject *object);

static gboolean gst_hailocounter_start(GstBaseTransform *trans);
static gboolean gst_hailocounter_stop(GstBaseTransform *trans);
static GstFlowReturn gst_hailocounter_transform_ip(GstBaseTransform *trans,
                                                   GstBuffer *buffer);

enum
{
    PROP_0,
};

G_DEFINE_TYPE_WITH_CODE(GstHailoCounter, gst_hailocounter, GST_TYPE_BASE_TRANSFORM,
                        GST_DEBUG_CATEGORY_INIT(gst_hailocounter_debug_category, "hailocounter", 0,
                                                "debug category for hailocounter element"));

static void
gst_hailocounter_class_init(GstHailoCounterClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);

    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                                            GST_CAPS_ANY));
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                                            GST_CAPS_ANY));

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                          "hailocounter - postprocessing element", "Hailo/Tools", "Allowes to user access Hailonet's output using an so file.",
                                          "hailo.ai <contact@hailo.ai>");

    gobject_class->set_property = gst_hailocounter_set_property;
    gobject_class->get_property = gst_hailocounter_get_property;

    gobject_class->dispose = gst_hailocounter_dispose;
    gobject_class->finalize = gst_hailocounter_finalize;
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_hailocounter_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_hailocounter_stop);
    base_transform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_hailocounter_transform_ip);
}

static void
gst_hailocounter_init(GstHailoCounter *hailocounter)
{
    hailocounter->counter = 0;
}

void gst_hailocounter_set_property(GObject *object, guint property_id,
                                   const GValue *value, GParamSpec *pspec)
{
    GstHailoCounter *hailocounter = GST_HAILO_COUNTER(object);

    GST_DEBUG_OBJECT(hailocounter, "set_property");

    switch (property_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailocounter_get_property(GObject *object, guint property_id,
                                   GValue *value, GParamSpec *pspec)
{
    GstHailoCounter *hailocounter = GST_HAILO_COUNTER(object);

    GST_DEBUG_OBJECT(hailocounter, "get_property");

    switch (property_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}
void gst_hailocounter_dispose(GObject *object)
{
    GstHailoCounter *hailocounter = GST_HAILO_COUNTER(object);
    GST_DEBUG_OBJECT(hailocounter, "dispose");
    
    hailocounter->counter = 0;

    /* clean up as possible.  may be called multiple times */

    G_OBJECT_CLASS(gst_hailocounter_parent_class)->dispose(object);
}

void gst_hailocounter_finalize(GObject *object)
{
    GstHailoCounter *hailocounter = GST_HAILO_COUNTER(object);

    GST_DEBUG_OBJECT(hailocounter, "finalize");

    /* clean up object here */

    G_OBJECT_CLASS(gst_hailocounter_parent_class)->finalize(object);
}

static gboolean gst_hailocounter_start(GstBaseTransform *trans)
{
    GstHailoCounter *hailocounter = GST_HAILO_COUNTER(trans);

    GST_DEBUG_OBJECT(hailocounter, "start");

    return TRUE;
}

static gboolean
gst_hailocounter_stop(GstBaseTransform *trans)
{
    GstHailoCounter *hailocounter = GST_HAILO_COUNTER(trans);

    GST_DEBUG_OBJECT(hailocounter, "stop");

    return TRUE;
}

static GstFlowReturn gst_hailocounter_transform_ip(GstBaseTransform *trans,
                                                   GstBuffer *buffer)
{
    GstHailoCounter *hailocounter = GST_HAILO_COUNTER(trans);
    gst_buffer_add_hailo_counter_meta(buffer, hailocounter->counter);
    hailocounter->counter++;
    GST_DEBUG_OBJECT(hailocounter, "Counter updated to %d, size is %zu", hailocounter->counter, gst_buffer_get_size(buffer));
    GST_DEBUG_OBJECT(hailocounter, "transform_ip");
    return GST_FLOW_OK;
}
