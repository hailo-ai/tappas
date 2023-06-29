/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include "gsthailograytonv12.hpp"
#include "tensor_meta.hpp"
#include "gst_hailo_meta.hpp"
#include "hailo/hailort.h"
#include <gst/video/video.h>
#include <gst/gst.h>
#include <dlfcn.h>
#include <map>
#include <iostream>

GST_DEBUG_CATEGORY_STATIC(gst_hailograytonv12_debug_category);
#define GST_CAT_DEFAULT gst_hailograytonv12_debug_category

static GstFlowReturn gst_hailograytonv12_transform(GstBaseTransform *trans, GstBuffer *inbuf, GstBuffer *outbuf);
static GstFlowReturn gst_hailograytonv12_prepare_output_buffer(GstBaseTransform *trans, GstBuffer *inbuf, GstBuffer **outbuf);
GstCaps *gst_hailograytonv12_transform_caps(GstBaseTransform *trans, GstPadDirection direction, GstCaps *caps, GstCaps *filter);

G_DEFINE_TYPE_WITH_CODE(GstHailograytonv12, gst_hailograytonv12, GST_TYPE_BASE_TRANSFORM,
                        GST_DEBUG_CATEGORY_INIT(gst_hailograytonv12_debug_category, "hailograytonv12", 0,
                                                "debug category for hailograytonv12 element"));

static void gst_hailograytonv12_class_init(GstHailograytonv12Class *klass)
{
    GstBaseTransformClass *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);

    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                                            gst_caps_from_string(GST_VIDEO_CAPS_MAKE("{ NV12 }"))));
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                                            gst_caps_from_string(GST_VIDEO_CAPS_MAKE("{ GRAY8 }"))));
    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                          "hailograytonv12 - postprocessing element", "Hailo/Tools", "Converts GRAY8 to NV12 by using the metadata that contains a pointer to the original NV12 buffer",
                                          "hailo.ai <contact@hailo.ai>");

    base_transform_class->prepare_output_buffer = GST_DEBUG_FUNCPTR(gst_hailograytonv12_prepare_output_buffer);
    base_transform_class->transform = GST_DEBUG_FUNCPTR(gst_hailograytonv12_transform);
    base_transform_class->transform_caps = GST_DEBUG_FUNCPTR(gst_hailograytonv12_transform_caps);
}

static void gst_hailograytonv12_init(GstHailograytonv12 *hailograytonv12) {}

static GstFlowReturn gst_hailograytonv12_prepare_output_buffer(GstBaseTransform *trans, GstBuffer *inbuf, GstBuffer **outbuf)
{
    // here we want to get the meta data that has the pointer to the original NV12 buffer. we are looking for the GstParentBufferMeta that doesn't have the tensor meta
    GstBuffer *original_buf = nullptr;
    GstMeta *meta;
    gpointer state = NULL;
    while ((meta = gst_buffer_iterate_meta_filtered(inbuf, &state, GST_PARENT_BUFFER_META_API_TYPE)))
    {
        GstParentBufferMeta *parent_meta = reinterpret_cast<GstParentBufferMeta *>(meta);
        if (!gst_buffer_get_meta(parent_meta->buffer, g_type_from_name(TENSOR_META_API_NAME)))
        {
            original_buf = parent_meta->buffer;
            gst_buffer_replace(outbuf, original_buf);
            gst_buffer_remove_meta(inbuf, (GstMeta *)parent_meta);
            break;
        }
    }

    return GST_FLOW_OK;
}

static GstFlowReturn gst_hailograytonv12_transform(GstBaseTransform *trans, GstBuffer *inbuf, GstBuffer *outbuf)
{
    GstHailograytonv12 *hailograytonv12 = GST_HAILO_GRAY_TO_NV12(trans);
    GST_DEBUG_OBJECT(hailograytonv12, "transform");
    gst_buffer_copy_into(outbuf, inbuf, GstBufferCopyFlags(GST_BUFFER_COPY_FLAGS | GST_BUFFER_COPY_TIMESTAMPS | GST_BUFFER_COPY_META), 0, -1);
    
    return GST_FLOW_OK;
}

GstCaps *gst_hailograytonv12_transform_caps(GstBaseTransform *trans, GstPadDirection direction, GstCaps *caps, GstCaps *filter)
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