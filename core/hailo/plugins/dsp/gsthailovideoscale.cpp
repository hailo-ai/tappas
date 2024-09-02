#include "dsp/gsthailovideoscale.hpp"
#include "buffer_utils.hpp"
#include "gst_hailo_meta.hpp"
#include "hailo_objects.hpp"
#include "media_library/buffer_pool.hpp"
#include <gst/gst.h>
#include <gst/video/video.h>

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
static gboolean gst_hailo_videoscale_start(GstBaseTransform *base_transform);
static gboolean gst_hailo_videoscale_stop(GstBaseTransform *base_transform);

enum
{
    PROP_PAD_0,
    PROP_USE_LETTERBOX,
};

static void
gst_hailo_videoscale_class_init(GstHailoVideoScaleClass *klass)
{
    GObjectClass *const object_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *const base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);

    object_class->set_property = gst_hailo_videoscale_set_property;
    object_class->get_property = gst_hailo_videoscale_get_property;

    base_transform_class->transform = GST_DEBUG_FUNCPTR(gst_hailo_videoscale_transform);
    base_transform_class->transform_caps = GST_DEBUG_FUNCPTR(gst_hailo_videoscale_transform_caps);
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_hailo_videoscale_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_hailo_videoscale_stop);

    g_object_class_install_property(object_class, PROP_USE_LETTERBOX,
                                    g_param_spec_boolean("use-letterbox", "Use letterbox", "Should we do the resize using letterbox.", false,
                                                         (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));

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
    GstHailoVideoScale *hailovideoscale = (GstHailoVideoScale *)(object);

    switch (prop_id)
    {
    case PROP_USE_LETTERBOX:
        hailovideoscale->use_letterbox = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_hailo_videoscale_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GstHailoVideoScale *hailovideoscale = (GstHailoVideoScale *)(object);

    switch (prop_id)
    {
    case PROP_USE_LETTERBOX:
        g_value_set_boolean(value, hailovideoscale->use_letterbox);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static gboolean gst_hailo_videoscale_start(GstBaseTransform *base_transform)
{
    auto status = dsp_utils::acquire_device();
    if (status != DSP_SUCCESS)
    {
        GST_ERROR_OBJECT(base_transform, "Failed to acquire device. return status: %d", status);
        return FALSE;
    }

    return TRUE;
}

static gboolean gst_hailo_videoscale_stop(GstBaseTransform *base_transform)
{
    auto staus = dsp_utils::release_device();

    if (staus != DSP_SUCCESS)
    {
        GST_ERROR_OBJECT(base_transform, "Failed to release device. return status: %d", staus);
        return FALSE;
    }

    return TRUE;
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

static HailoBBox calculate_scale_bbox(dsp_image_properties_t *input_image, dsp_image_properties_t *output_image)
{
    // Calculate the scaling ratio
    float ratio = std::min(float(output_image->width) / input_image->width,
                           float(output_image->height) / input_image->height);

    // Calculate the new dimensions of the input image
    int new_width = std::round(input_image->width * ratio);
    int new_height = std::round(input_image->height * ratio);

    // Calculate the margins for letterboxing
    int left = (output_image->width - new_width) / 2;
    int top = (output_image->height - new_height) / 2;
    int right = output_image->width - new_width - left;
    int bottom = output_image->height - new_height - top;

    // Adjust for rounding errors
    int cols_diff = output_image->width - (new_width + left + right);
    int rows_diff = output_image->height - (new_height + top + bottom);
    left += cols_diff;
    top += rows_diff;

    // Construct and return the transformation
    HailoBBox letterboxed_scale = HailoBBox(-(left / float(new_width)),                       // x-offset
                                             -(top / float(new_height)),                       // y-offset
                                             1.0 / (new_width / float(output_image->height)),  // width factor
                                             1.0 / (new_height / float(output_image->width))); // height factor

    return letterboxed_scale;
}

static GstFlowReturn
gst_hailo_videoscale_transform(GstBaseTransform *base_transform, GstBuffer *inbuf, GstBuffer *outbuf)
{
    GstHailoVideoScale *hailovideoscale = GST_HAILO_VIDEOSCALE(base_transform);

    GstFlowReturn ret = GST_FLOW_OK;

    GST_DEBUG_OBJECT(hailovideoscale, "Hailo Videoscale transform");

    GstCaps *incaps = gst_pad_get_current_caps(base_transform->sinkpad);
    HailoMediaLibraryBufferPtr input_frame_ptr = hailo_buffer_from_gst_buffer(inbuf, incaps);
    if (!input_frame_ptr)
    {
        GST_ERROR_OBJECT(hailovideoscale, "Cannot create hailo input buffer from GstBuffer");
        return GST_FLOW_ERROR;
    }

    GstCaps *outcaps = gst_pad_get_current_caps(base_transform->srcpad);
    if(!outcaps)
    {
        GST_ERROR_OBJECT(hailovideoscale, "Failed to get output caps");
        return GST_FLOW_ERROR;
    }

    gint size = gst_caps_get_size(outcaps);

    if (size != 1)
    {
        GST_ERROR_OBJECT(hailovideoscale, "Output caps size is not 1 (%d)", size);
        return GST_FLOW_ERROR;
    }
    
    HailoMediaLibraryBufferPtr output_frame_ptr = hailo_buffer_from_gst_buffer(outbuf, outcaps);
    if (!output_frame_ptr)
    {
        GST_ERROR_OBJECT(hailovideoscale, "Cannot create hailo output buffer from GstBuffer");
        return GST_FLOW_ERROR;
    }

    GST_DEBUG_OBJECT(hailovideoscale, "DSP Resize: Input Width: %ld, Height: %ld. \
                                        Resize target Width: %ld Height: %ld",
                     input_frame_ptr->hailo_pix_buffer->width, input_frame_ptr->hailo_pix_buffer->height,
                     output_frame_ptr->hailo_pix_buffer->width, output_frame_ptr->hailo_pix_buffer->height);

    // Perform the resize operation on the DSP

    dsp_letterbox_properties_t letterbox_params{
        .alignment = hailovideoscale->use_letterbox ? DSP_LETTERBOX_MIDDLE : DSP_NO_LETTERBOX,
        .color = {.y = 0, .u = 128, .v = 128}, // Black letterbox border
    };

    dsp_status result = dsp_utils::perform_resize(input_frame_ptr->hailo_pix_buffer.get(), output_frame_ptr->hailo_pix_buffer.get(), 
                                                  INTERPOLATION_TYPE_BILINEAR, letterbox_params);

    if (result != DSP_SUCCESS)
    {
        GST_ERROR_OBJECT(hailovideoscale, "Failed to perform dsp resize. return status: %d", result);
        ret = GST_FLOW_ERROR;
    }

    HailoROIPtr hailo_roi = get_hailo_main_roi(outbuf, true);
    auto scailing_bbox = calculate_scale_bbox(input_frame_ptr->hailo_pix_buffer.get(), output_frame_ptr->hailo_pix_buffer.get());
    hailo_roi->set_scaling_bbox(scailing_bbox);

    gst_caps_unref(incaps);
    gst_caps_unref(outcaps);

    return ret;
}