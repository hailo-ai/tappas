/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include <gst/video/video.h>
#include <gst/gst.h>
#include <gst/gstobject.h>
#include <map>
#include <iostream>

#include "gsthailoosd.hpp"
#include "gst_hailo_meta.hpp"
#include "hailo/hailort.h"

GST_DEBUG_CATEGORY_STATIC(gst_hailoosd_debug_category);
#define GST_CAT_DEFAULT gst_hailoosd_debug_category
#define gst_hailoosd_parent_class parent_class

static void gst_hailoosd_set_property(GObject *object,
                                         guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_hailoosd_get_property(GObject *object,
                                         guint property_id, GValue *value, GParamSpec *pspec);
static void gst_hailoosd_dispose(GObject *object);
static void gst_hailoosd_finalize(GObject *object);

static gboolean gst_hailoosd_start(GstBaseTransform *trans);
static gboolean gst_hailoosd_stop(GstBaseTransform *trans);
static gboolean gst_hailoosd_set_caps(GstBaseTransform *trans, GstCaps *incaps, GstCaps *outcaps);
static gboolean gst_hailoosd_propose_allocation (GstBaseTransform *trans, GstQuery *decide_query, GstQuery *query);
static GstFlowReturn gst_hailoosd_transform_ip(GstBaseTransform *trans,
                                                  GstBuffer *buffer);
static void gst_hailoosd_before_transform(GstBaseTransform *trans, GstBuffer *buffer);

enum
{
    PROP_0,
    PROP_CONFIG_FILE_PATH,
    PROP_WAIT_FOR_WRITABLE_BUFFER,
};

G_DEFINE_TYPE_WITH_CODE(GstHailoOsd, gst_hailoosd, GST_TYPE_BASE_TRANSFORM,
                        GST_DEBUG_CATEGORY_INIT(gst_hailoosd_debug_category, "hailoosd", 0,
                                                "debug category for hailoosd element"));

static void
gst_hailoosd_class_init(GstHailoOsdClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);

    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                                            gst_caps_from_string(GST_VIDEO_CAPS_MAKE("{ NV12 }"))));
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                                            gst_caps_from_string(GST_VIDEO_CAPS_MAKE("{ NV12 }"))));

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                          "hailoosd - on-screen-display element",
                                          "Hailo/Tools",
                                          "Draws on-screen-display telemetry on frame.",
                                          "hailo.ai <contact@hailo.ai>");

    gobject_class->set_property = gst_hailoosd_set_property;
    gobject_class->get_property = gst_hailoosd_get_property;
    g_object_class_install_property(gobject_class, PROP_CONFIG_FILE_PATH,
                                    g_param_spec_string("config-path", "config-path",
                                                        "json config file path", NULL,
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));

    g_object_class_install_property(gobject_class, PROP_WAIT_FOR_WRITABLE_BUFFER,
                                    g_param_spec_boolean("wait-for-writable-buffer", "wait-for-writable-buffer",
                                                         "enables the element thread to wait until incomming buffer is writable", FALSE,
                                                         (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));

    gobject_class->dispose = gst_hailoosd_dispose;
    gobject_class->finalize = gst_hailoosd_finalize;
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_hailoosd_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_hailoosd_stop);
    base_transform_class->set_caps = GST_DEBUG_FUNCPTR(gst_hailoosd_set_caps);
    base_transform_class->propose_allocation = GST_DEBUG_FUNCPTR(gst_hailoosd_propose_allocation);
    base_transform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_hailoosd_transform_ip);
    base_transform_class->before_transform = GST_DEBUG_FUNCPTR(gst_hailoosd_before_transform);
}

static void
gst_hailoosd_init(GstHailoOsd *hailoosd)
{
    hailoosd->params = nullptr;
    hailoosd->config_path = g_strdup("NULL");
    hailoosd->wait_for_writable_buffer = false;
}

void gst_hailoosd_set_property(GObject *object, guint property_id,
                                  const GValue *value, GParamSpec *pspec)
{
    GstHailoOsd *hailoosd = GST_HAILO_OSD(object);

    GST_DEBUG_OBJECT(hailoosd, "set_property");

    switch (property_id)
    {
    case PROP_CONFIG_FILE_PATH:
        hailoosd->config_path = g_strdup(g_value_get_string(value));
        break;
    case PROP_WAIT_FOR_WRITABLE_BUFFER:
        hailoosd->wait_for_writable_buffer = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailoosd_get_property(GObject *object, guint property_id,
                                  GValue *value, GParamSpec *pspec)
{
    GstHailoOsd *hailoosd = GST_HAILO_OSD(object);

    GST_DEBUG_OBJECT(hailoosd, "get_property");

    switch (property_id)
    {
    case PROP_CONFIG_FILE_PATH:
        g_value_set_string(value, hailoosd->config_path);
        break;
    case PROP_WAIT_FOR_WRITABLE_BUFFER:
        g_value_set_boolean(value, hailoosd->wait_for_writable_buffer);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}
void gst_hailoosd_dispose(GObject *object)
{
    GstHailoOsd *hailoosd = GST_HAILO_OSD(object);

    GST_DEBUG_OBJECT(hailoosd, "dispose");

    /* clean up as possible.  may be called multiple times */

    G_OBJECT_CLASS(gst_hailoosd_parent_class)->dispose(object);
}

void gst_hailoosd_finalize(GObject *object)
{
    GstHailoOsd *hailoosd = GST_HAILO_OSD(object);

    GST_DEBUG_OBJECT(hailoosd, "finalize");

    /* clean up object here */

    G_OBJECT_CLASS(gst_hailoosd_parent_class)->finalize(object);
}

static gboolean gst_hailoosd_start(GstBaseTransform *trans)
{
    GstHailoOsd *hailoosd = GST_HAILO_OSD(trans);

    // Load params
    hailoosd->params = load_json_config(hailoosd->config_path);

    // Acquire DSP device
    dsp_status status = acquire_device();
    if (status != DSP_SUCCESS)
    {
        GST_ERROR_OBJECT(hailoosd, "Accuire DSP device failed with status code %d", status);
    }

    GST_DEBUG_OBJECT(hailoosd, "start");

    return TRUE;
}

static gboolean
gst_hailoosd_stop(GstBaseTransform *trans)
{
    GstHailoOsd *hailoosd = GST_HAILO_OSD(trans);

    // Release param resources
    free_param_resources(hailoosd->params);

    // Release DSP device
    dsp_status result = release_device();
    if (result != DSP_SUCCESS)
    {
        GST_ERROR_OBJECT(hailoosd, "Release DSP device failed with status code %d", result);
    }

    GST_DEBUG_OBJECT(hailoosd, "stop");

    return TRUE;
}

static void
gst_hailoosd_wait_for_writable_buffer(GstHailoOsd *hailoosd, GstBuffer *buffer)
{
    GST_DEBUG_OBJECT(hailoosd, "Buffer (offset: %ld) is not writable, refcount: %d. waiting... ", GST_BUFFER_OFFSET(buffer), GST_OBJECT_REFCOUNT(buffer));
    while(gst_buffer_is_writable(buffer) == FALSE)
    {
        // Wait for buffer to be writable
        g_usleep (100);
    }
}

static void 
gst_hailoosd_before_transform(GstBaseTransform *trans, GstBuffer *buffer)
{
    GstHailoOsd *hailoosd = GST_HAILO_OSD(trans);

    if (gst_buffer_is_writable(buffer) == FALSE)
    {
        if(hailoosd->wait_for_writable_buffer)
        {
            gst_hailoosd_wait_for_writable_buffer(hailoosd, buffer);
        }
        else
        {
            GST_ERROR_OBJECT(hailoosd, "Buffer (offset: %ld) is not writable!, buffer refcount is %d. Aborting...", GST_BUFFER_OFFSET(buffer), GST_OBJECT_REFCOUNT(buffer));
            throw std::runtime_error("HailoOSD -> Buffer is not writable! please validate that that the pipline is sharing the buffer properly, or use wait-for-writable-buffer parameter");
        }
    }

    GST_DEBUG_OBJECT(hailoosd, "Buffer is writable, refcount: %d, continuing...", GST_OBJECT_REFCOUNT(buffer));
}

static gboolean
gst_hailoosd_set_caps(GstBaseTransform *trans, GstCaps *incaps, GstCaps *outcaps)
{
    GstHailoOsd *hailoosd = GST_HAILO_OSD(trans);
    osd_status_t ret = OSD_STATUS_UNINITIALIZED;

    // now that caps are negotiated, get the size of the image for relative scaling
    GstVideoInfo *full_image_info = gst_video_info_new();
    gst_video_info_from_caps(full_image_info, incaps);

    // init overlay images from the json params
    ret = initialize_overlay_images(hailoosd->params, full_image_info->width, full_image_info->height);

    // cleanup
    gst_video_info_free(full_image_info);
    
    // check success status
    if (ret != OSD_STATUS_OK)
        return FALSE;

    return TRUE;
}

static gboolean
gst_hailoosd_propose_allocation (GstBaseTransform *trans,
                                 GstQuery *decide_query, GstQuery *query)
{
    GstHailoOsd *hailoosd = GST_HAILO_OSD(trans);
    GST_DEBUG_OBJECT(hailoosd, "hailoosd propose allocation callback");
    gboolean ret;
    gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, NULL);
    ret = GST_BASE_TRANSFORM_CLASS(parent_class)->propose_allocation(trans, decide_query, query);
    return ret;
}

static GstFlowReturn gst_hailoosd_transform_ip(GstBaseTransform *trans,
                                                  GstBuffer *buffer)
{
    osd_status_t ret = OSD_STATUS_UNINITIALIZED;
    GstHailoOsd *hailoosd = GST_HAILO_OSD(trans);
    GST_DEBUG_OBJECT(hailoosd, "transform_ip");

    // acquire caps and metas
    GstCaps *caps;
    caps = gst_pad_get_current_caps(trans->sinkpad);
    GstVideoInfo *info = gst_video_info_new();
    gst_video_info_from_caps(info, caps);
    GstVideoFrame video_frame;
    gst_video_frame_map(&video_frame, info, buffer, GST_MAP_READ);

    // perform blending
    ret = blend_all(&video_frame, hailoosd->params);

    // cleanup
    gst_video_frame_unmap (&video_frame);
    gst_video_info_free(info);
    gst_caps_unref(caps);

    // check success status
    if (ret != OSD_STATUS_OK)
        return GST_FLOW_ERROR;
    return GST_FLOW_OK;
}
