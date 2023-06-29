#include <gst/gst.h>
#include <gst/video/video.h>
#include <iostream>
#include "dsp/gsthailodspbasetransform.hpp"
#include "dsp/gsthailovideoscale.hpp"
#include "dsp/gsthailodsp.h"

GST_DEBUG_CATEGORY_STATIC(gst_hailo_videoscale_debug);
#define GST_CAT_DEFAULT gst_hailo_videoscale_debug

#define _do_init \
    GST_DEBUG_CATEGORY_INIT(gst_hailo_videoscale_debug, "hailovideoscale", 0, "Hailo Videoscale");
#define gst_hailo_videoscale_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstHailoVideoScale, gst_hailo_videoscale, GST_TYPE_HAILO_DSP_BASE_TRANSFORM, _do_init);

static GstFlowReturn
gst_hailo_videoscale_transform(GstBaseTransform *base_transform, GstBuffer *inbuf, GstBuffer *outbuf);

static void gst_hailo_videoscale_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_hailo_videoscale_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static GstCaps *gst_hailo_videoscale_transform_caps(GstBaseTransform *trans,
                                                    GstPadDirection direction, GstCaps *caps, GstCaps *filter);

static void
gst_hailo_videoscale_class_init(GstHailoVideoScaleClass *klass)
{
    GObjectClass *const object_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *const base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);

    object_class->set_property = gst_hailo_videoscale_set_property;
    object_class->get_property = gst_hailo_videoscale_get_property;

    base_transform_class->transform = GST_DEBUG_FUNCPTR(gst_hailo_videoscale_transform);

    base_transform_class->transform_caps = GST_DEBUG_FUNCPTR(gst_hailo_videoscale_transform_caps);

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                          "Hailo Videoscale",
                                          "Hailo/Tools",
                                          "Perform resize on a buffer using Hailo15 DSP",
                                          "hailo.ai <contact@hailo.ai>");

    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                                            gst_caps_from_string(HAILO_VIDEOSCALE_VIDEO_CAPS)));
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                                             gst_caps_from_string(HAILO_VIDEOSCALE_VIDEO_CAPS)));
}

static void
gst_hailo_videoscale_init(GstHailoVideoScale *self)
{
    gst_base_transform_set_in_place(GST_BASE_TRANSFORM(self), FALSE);
}

static void
gst_hailo_videoscale_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    switch (prop_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_hailo_videoscale_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    switch (prop_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static GstCaps *
gst_hailo_videoscale_transform_caps(GstBaseTransform *trans,
                                    GstPadDirection direction, GstCaps *caps, GstCaps *filter)
{
    GstCaps *res_caps, *tmp_caps;
    GstStructure *structure;
    guint i, caps_size;

    // Remove width and height from caps - to allow basetransform negotiate properly
    res_caps = gst_caps_copy(caps);
    caps_size = gst_caps_get_size(res_caps);
    for (i = 0; i < caps_size; i++)
    {
        structure = gst_caps_get_structure(res_caps, i);
        gst_structure_remove_field(structure, "width");
        gst_structure_remove_field(structure, "height");
    }

    if (filter)
    {
        tmp_caps = res_caps;
        res_caps = gst_caps_intersect_full(filter, tmp_caps, GST_CAPS_INTERSECT_FIRST);
        gst_caps_unref(tmp_caps);
    }
    return res_caps;
}

static GstFlowReturn
gst_hailo_videoscale_transform(GstBaseTransform *base_transform, GstBuffer *inbuf, GstBuffer *outbuf)
{
    GstHailoVideoScale *hailovideoscale = GST_HAILO_VIDEOSCALE(base_transform);

    GstFlowReturn ret = GST_FLOW_OK;

    GST_DEBUG_OBJECT(hailovideoscale, "Hailo Videoscale transform");

    // Get input and output caps
    GstCaps *incaps, *outcaps;
    incaps = gst_pad_get_current_caps(base_transform->sinkpad);
    outcaps = gst_pad_get_current_caps(base_transform->srcpad);

    GstVideoInfo input_video_info;
    gst_video_info_from_caps(&input_video_info, incaps);

    GstVideoInfo output_video_info;
    gst_video_info_from_caps(&output_video_info, outcaps);

    // Map input and output buffers to GstVideoFrame
    GstVideoFrame input_video_frame;
    if (!gst_video_frame_map(&input_video_frame, &input_video_info, inbuf, GST_MAP_READ))
    {
        GST_ERROR_OBJECT(hailovideoscale, "Cannot map input buffer to frame");
        throw std::runtime_error("Cannot map input buffer to frame");
    }

    GstVideoFrame output_video_frame;
    if (!gst_video_frame_map(&output_video_frame, &output_video_info, outbuf, GST_MAP_READWRITE))
    {
        GST_ERROR_OBJECT(hailovideoscale, "Cannot map output buffer to frame");
        throw std::runtime_error("Cannot map output buffer to frame");
    }

    // Create dsp image properties from both input and output video frame objects
    dsp_image_properties_t input_image_properties = create_image_properties_from_video_frame(&input_video_frame);
    dsp_image_properties_t output_image_properties = create_image_properties_from_video_frame(&output_video_frame);

    GST_DEBUG_OBJECT(hailovideoscale, "DSP Resize: Input Width: %ld, Height: %ld. \
                                        Resize target Width: %ld Height: %ld",
                     input_image_properties.width, input_image_properties.height,
                     output_image_properties.width, output_image_properties.height);

    // Perform the resize operation on the DSP
    dsp_status result = perform_dsp_resize(&input_image_properties, &output_image_properties, INTERPOLATION_TYPE_BILINEAR);

    if (result != DSP_SUCCESS)
    {
        GST_ERROR_OBJECT(hailovideoscale, "Failed to perform dsp resize. return status: %d", result);
        ret = GST_FLOW_ERROR;
    }

    // Free resources
    free_image_property_planes(&input_image_properties);
    free_image_property_planes(&output_image_properties);

    gst_video_frame_unmap(&input_video_frame);
    gst_video_frame_unmap(&output_video_frame);
 
    gst_caps_unref(incaps);
    gst_caps_unref(outcaps);

    return ret;
}