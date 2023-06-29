/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include "gsthailonv12togray.hpp"
#include "tensor_meta.hpp"
#include "gst_hailo_meta.hpp"
#include "hailo/hailort.h"
#include <gst/video/video.h>
#include <gst/gst.h>
#include <dlfcn.h>
#include <map>
#include <iostream>

GST_DEBUG_CATEGORY_STATIC(gst_hailonv12togray_debug_category);
#define GST_CAT_DEFAULT gst_hailonv12togray_debug_category

static GstFlowReturn gst_hailonv12togray_transform(GstBaseTransform *trans, GstBuffer *inbuf, GstBuffer *outbuf);
static GstFlowReturn gst_hailonv12togray_prepare_output_buffer(GstBaseTransform *trans, GstBuffer *inbuf, GstBuffer **outbuf);
GstCaps *gst_hailonv12togray_transform_caps(GstBaseTransform *trans, GstPadDirection direction, GstCaps *caps, GstCaps *filter);

G_DEFINE_TYPE_WITH_CODE(GstHailonv12togray, gst_hailonv12togray, GST_TYPE_BASE_TRANSFORM,
                        GST_DEBUG_CATEGORY_INIT(gst_hailonv12togray_debug_category, "hailonv12togray", 0,
                                                "debug category for hailonv12togray element"));

static void gst_hailonv12togray_class_init(GstHailonv12tograyClass *klass)
{
    GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);

    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                                            gst_caps_from_string(GST_VIDEO_CAPS_MAKE("{ GRAY8 }"))));
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                                            gst_caps_from_string(GST_VIDEO_CAPS_MAKE("{ NV12 }"))));

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                          "hailonv12togray - postprocessing element", "Hailo/Tools", "Converts NV12 to GRAY8 and keeps the original NV12 buffer as metadata",
                                          "hailo.ai <contact@hailo.ai>");

    base_transform_class->prepare_output_buffer = GST_DEBUG_FUNCPTR(gst_hailonv12togray_prepare_output_buffer);
    base_transform_class->transform = GST_DEBUG_FUNCPTR(gst_hailonv12togray_transform);
    base_transform_class->transform_caps = GST_DEBUG_FUNCPTR(gst_hailonv12togray_transform_caps);
}

static void gst_hailonv12togray_init(GstHailonv12togray *hailonv12togray){}

static GstFlowReturn gst_hailonv12togray_prepare_output_buffer(GstBaseTransform *trans, GstBuffer *inbuf, GstBuffer **outbuf)
{
    GstVideoFrame frame;
    GstPad *sinkpad = trans->sinkpad;
    GstCaps *caps = gst_pad_get_current_caps(sinkpad);
    GstVideoInfo info;
    gst_video_info_from_caps(&info, caps);
    gst_caps_unref(caps);
    if (!(gst_video_frame_map(&frame, &info, inbuf, GstMapFlags(GST_MAP_READ))))
    {
        throw std::runtime_error("Cannot map buffer to frame");
    }

    void *y_pointer = GST_VIDEO_FRAME_PLANE_DATA(&frame, 0);

    GstMapInfo map;
    gst_buffer_map(inbuf, &map, GST_MAP_READ);

    *outbuf = gst_buffer_new_wrapped(y_pointer, map.size * 2 / 3);
    gst_memory_ref(gst_buffer_get_all_memory(*outbuf));

    gst_buffer_unmap(inbuf, &map);
    gst_video_frame_unmap(&frame);
    return GST_FLOW_OK;
}

static GstFlowReturn gst_hailonv12togray_transform(GstBaseTransform *trans, GstBuffer *inbuf, GstBuffer *outbuf)
{
    /*
    Just add the inbuf as a meta to the outbuf so the original buffer can be retrieved later.
    */
    GstHailonv12togray *hailonv12togray = GST_HAILO_NV12_TO_GRAY(trans);
    gst_buffer_add_parent_buffer_meta(outbuf, inbuf); // this refs the inbuf

    GST_DEBUG_OBJECT(hailonv12togray, "transform");
    return GST_FLOW_OK;
}

GstCaps *gst_hailonv12togray_transform_caps(GstBaseTransform *trans, GstPadDirection direction, GstCaps *caps, GstCaps *filter)
{
    /*
    Removing the format field so that the caps can be intersected with the filter caps.
    The same logic was implemented in gstbayer2rgb element and is useful for the case the input and output caps of an element don't share a common format.
    */
    GstCaps *res_caps, *tmp_caps;
    GstStructure *structure;
    guint i, caps_size;

    res_caps = gst_caps_copy(caps);
    caps_size = gst_caps_get_size(res_caps);
    for (i = 0; i < caps_size; i++)
    {
        structure = gst_caps_get_structure(res_caps, i);
        gst_structure_remove_field(structure, "format");
    }

    if (filter)
    {
        tmp_caps = res_caps;
        res_caps = gst_caps_intersect_full(filter, tmp_caps, GST_CAPS_INTERSECT_FIRST);
        gst_caps_unref(tmp_caps);
    }
    return res_caps;
}