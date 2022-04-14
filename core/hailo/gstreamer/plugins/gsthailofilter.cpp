/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/* GStreamer
 * Copyright (C) 2021 FIXME <fixme@example.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gsthailofilter
 *
 * The hailofilter element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! hailofilter ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <chrono>
#include <ctime>
#include "gsthailofilter.hpp"
#include <iomanip>
#include <iostream>
#include <dlfcn.h>
#include <vector>

GST_DEBUG_CATEGORY_STATIC(gst_hailofilter_debug_category);
#define GST_CAT_DEFAULT gst_hailofilter_debug_category
#define DEFAULT_FUNCTION_NAME "filter"
#define HAILO_SUPPORTED_FORMATS "{ RGB, YUY2 }"
#define HAILO_VIDEO_CAPS \
    GST_VIDEO_CAPS_MAKE(HAILO_SUPPORTED_FORMATS)
/* prototypes */

static void gst_hailofilter_set_property(GObject *object,
                                         guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_hailofilter_get_property(GObject *object,
                                         guint property_id, GValue *value, GParamSpec *pspec);
static void gst_hailofilter_dispose(GObject *object);
static void gst_hailofilter_finalize(GObject *object);

static gboolean gst_hailofilter_start(GstBaseTransform *trans);
static gboolean gst_hailofilter_stop(GstBaseTransform *trans);
static gboolean gst_hailofilter_set_info(GstVideoFilter *filter,
                                         GstCaps *incaps, GstVideoInfo *in_info, GstCaps *outcaps,
                                         GstVideoInfo *out_info);
static GstFlowReturn gst_hailofilter_transform_frame_ip(GstVideoFilter *filter,
                                                        GstVideoFrame *frame);
static gboolean gst_hailofilter_sink_event(GstBaseTransform *trans, GstEvent *event);

enum
{
    PROP_0,
    PROP_DEBUG,
    PROP_PROCESS_LIB,
    PROP_PROCESS_FUNC_NAME,
};

/* class initialization */

G_DEFINE_TYPE_WITH_CODE(GstHailoFilter, gst_hailofilter, GST_TYPE_VIDEO_FILTER,
                        GST_DEBUG_CATEGORY_INIT(gst_hailofilter_debug_category, "hailofilter", 0,
                                                "debug category for hailofilter element"));

static void
gst_hailofilter_class_init(GstHailoFilterClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);
    GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS(klass);

    /* Setting up pads and setting metadata should be moved to
       base_class_init if you intend to subclass this class. */
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                                            gst_caps_from_string(HAILO_VIDEO_CAPS)));
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                                            gst_caps_from_string(HAILO_VIDEO_CAPS)));

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                          "hailofilter - postprocessing element", "Hailo/Tools", "Allowes to user access Hailonet's output using an so file.",
                                          "hailo.ai <contact@hailo.ai>");

    gobject_class->set_property = gst_hailofilter_set_property;
    gobject_class->get_property = gst_hailofilter_get_property;
    g_object_class_install_property(gobject_class, PROP_PROCESS_LIB,
                                    g_param_spec_string("so-path", "process so Path Location",
                                                        "Location of the so file to load", NULL,
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
    g_object_class_install_property(gobject_class, PROP_PROCESS_FUNC_NAME,
                                    g_param_spec_string("function-name", "Name of function in the so file",
                                                        "function-name", "filter",
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
    g_object_class_install_property(gobject_class, PROP_DEBUG,
                                    g_param_spec_boolean("debug", "debug", "debug", false,
                                                         (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    gobject_class->dispose = gst_hailofilter_dispose;
    gobject_class->finalize = gst_hailofilter_finalize;
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_hailofilter_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_hailofilter_stop);
    video_filter_class->set_info = GST_DEBUG_FUNCPTR(gst_hailofilter_set_info);
    video_filter_class->transform_frame_ip =
        GST_DEBUG_FUNCPTR(gst_hailofilter_transform_frame_ip);
    base_transform_class->sink_event = GST_DEBUG_FUNCPTR(gst_hailofilter_sink_event);
}

static void
gst_hailofilter_init(GstHailoFilter *hailofilter)
{
    hailofilter->debug = false;
    hailofilter->default_function_name = g_strdup(DEFAULT_FUNCTION_NAME);
}

void gst_hailofilter_set_property(GObject *object, guint property_id,
                                  const GValue *value, GParamSpec *pspec)
{
    GstHailoFilter *hailofilter = GST_HAILO_FILTER(object);

    GST_DEBUG_OBJECT(hailofilter, "set_property");

    switch (property_id)
    {
    case PROP_DEBUG:
        hailofilter->debug = g_value_get_boolean(value);
        break;
    case PROP_PROCESS_LIB:
        hailofilter->lib_path = g_strdup(g_value_get_string(value));
        break;
    case PROP_PROCESS_FUNC_NAME:
        hailofilter->function_name.push_back(g_strdup(g_value_get_string(value)));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailofilter_get_property(GObject *object, guint property_id,
                                  GValue *value, GParamSpec *pspec)
{
    GstHailoFilter *hailofilter = GST_HAILO_FILTER(object);

    GST_DEBUG_OBJECT(hailofilter, "get_property");

    switch (property_id)
    {
    case PROP_DEBUG:
        g_value_set_boolean(value, hailofilter->debug);
        break;
    case PROP_PROCESS_LIB:
        g_value_set_string(value, hailofilter->lib_path);
        break;
    case PROP_PROCESS_FUNC_NAME:
        g_value_set_string(value, DEFAULT_FUNCTION_NAME);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailofilter_dispose(GObject *object)
{
    GstHailoFilter *hailofilter = GST_HAILO_FILTER(object);
    if (hailofilter->loaded_lib != nullptr)
    {
        dlclose(hailofilter->loaded_lib);
    }

    GST_DEBUG_OBJECT(hailofilter, "dispose");

    /* clean up as possible.  may be called multiple times */

    G_OBJECT_CLASS(gst_hailofilter_parent_class)->dispose(object);
}

void gst_hailofilter_finalize(GObject *object)
{
    GstHailoFilter *hailofilter = GST_HAILO_FILTER(object);

    GST_DEBUG_OBJECT(hailofilter, "finalize");

    /* clean up object here */

    G_OBJECT_CLASS(gst_hailofilter_parent_class)->finalize(object);
}

static gboolean
gst_hailofilter_start(GstBaseTransform *trans)
{
    GstHailoFilter *hailofilter = GST_HAILO_FILTER(trans);
    hailofilter->loaded_lib = dlopen(hailofilter->lib_path, RTLD_LAZY);
    if (!hailofilter->loaded_lib)
    {
        std::cerr << "Could not load lib " << dlerror() << std::endl;
    }
    // reset errors
    dlerror();

    if (hailofilter->function_name.size() == 0)
    {
        hailofilter->function_name.push_back(hailofilter->default_function_name);
    }
    for (uint i = 0; i < hailofilter->function_name.size(); i++)
    {
        auto f = hailofilter->function_name[i];
        hailofilter->handler.push_back((void (*)(HailoFramePtr))dlsym(hailofilter->loaded_lib, f));
    }
    const char *dlsym_error = dlerror();
    if (dlsym_error)
    {
        std::cerr << "Cannot load symbol: " << dlsym_error << std::endl;
        dlclose(hailofilter->loaded_lib);
    }
    GST_DEBUG_OBJECT(hailofilter, "start");

    return TRUE;
}

static gboolean
gst_hailofilter_stop(GstBaseTransform *trans)
{
    GstHailoFilter *hailofilter = GST_HAILO_FILTER(trans);

    GST_DEBUG_OBJECT(hailofilter, "stop");

    return TRUE;
}

static gboolean
gst_hailofilter_set_info(GstVideoFilter *filter, GstCaps *incaps,
                         GstVideoInfo *in_info, GstCaps *outcaps, GstVideoInfo *out_info)
{
    GstHailoFilter *hailofilter = GST_HAILO_FILTER(filter);

    GST_DEBUG_OBJECT(hailofilter, "set_info");

    return TRUE;
}

static GstFlowReturn
gst_hailofilter_transform_frame_ip(GstVideoFilter *filter,
                                   GstVideoFrame *frame)
{
    GstHailoFilter *hailofilter = GST_HAILO_FILTER(filter);
    HailoFramePtr hailo_frame = std::make_shared<HailoFrame>(frame, hailofilter->stream_id);

    if (hailofilter->debug)
    {
        std::chrono::duration<double, std::milli> latency;
        std::chrono::time_point<std::chrono::system_clock> start_time;
        start_time = std::chrono::system_clock::now();
        for (uint i = 0; i < hailofilter->handler.size(); i++)
        {
            auto handler = hailofilter->handler[i];
            handler(hailo_frame);
        }
        latency = std::chrono::system_clock::now() - start_time;
        g_print("hailofilter latency: %f milliseconds\n", latency.count());
    }
    else
    {
        for (uint i = 0; i < hailofilter->handler.size(); i++)
        {
            auto handler = hailofilter->handler[i];
            handler(hailo_frame);
        }
    }
    GST_DEBUG_OBJECT(hailofilter, "transform_frame_ip");
    return GST_FLOW_OK;
}

static gboolean
gst_hailofilter_sink_event(GstBaseTransform *trans,
                           GstEvent *event)
{
    GstHailoFilter *hailofilter = GST_HAILO_FILTER(trans);
    switch (GST_EVENT_TYPE(event))
    {
    case GST_EVENT_STREAM_START:
        const gchar *stream_id;
        gst_event_parse_stream_start(event, &stream_id);
        if (!stream_id)
        {
            GST_ERROR_OBJECT(hailofilter, "No stream ID");
            return FALSE;
        }
        else
        {
            GST_DEBUG_OBJECT(hailofilter, "filtering stream %s", stream_id);
            hailofilter->stream_id = strdup(stream_id);
        }
    default:
        return GST_BASE_TRANSFORM_CLASS(gst_hailofilter_parent_class)->sink_event(trans, event);
    }
}