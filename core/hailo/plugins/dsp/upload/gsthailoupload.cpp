#include "dsp/gsthailodspbasetransform.hpp"
#include "dsp/upload/gsthailoupload.hpp"
#include <gst/gst.h>
#include <gst/video/video.h>
#include "dsp/gsthailodsp.h"

GST_DEBUG_CATEGORY_STATIC(gst_hailo_upload_debug);
#define GST_CAT_DEFAULT gst_hailo_upload_debug

#define _do_init \
    GST_DEBUG_CATEGORY_INIT(gst_hailo_upload_debug, "hailoupload", 0, "Hailo Upload");
#define gst_hailo_upload_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstHailoUpload, gst_hailo_upload, GST_TYPE_HAILO_DSP_BASE_TRANSFORM, _do_init);

static GstFlowReturn
gst_hailo_upload_transform(GstBaseTransform *base_transform, GstBuffer *inbuf, GstBuffer *outbuf);

static void gst_hailo_upload_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_hailo_upload_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static void
gst_hailo_upload_class_init(GstHailoUploadClass *klass)
{
    GObjectClass *const object_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *const base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);

    object_class->set_property = gst_hailo_upload_set_property;
    object_class->get_property = gst_hailo_upload_get_property;

    base_transform_class->transform = GST_DEBUG_FUNCPTR(gst_hailo_upload_transform);

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                          "Hailo Upload",
                                          "Hailo/Tools",
                                          "Uploads a buffer to the Hailo15 DSP memory",
                                          "hailo.ai <contact@hailo.ai>");

    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                                            GST_CAPS_ANY));
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                                            GST_CAPS_ANY));
}

static void
gst_hailo_upload_init(GstHailoUpload *self)
{
}

static void gst_hailo_upload_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    switch (prop_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_hailo_upload_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    switch (prop_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static GstFlowReturn
gst_hailo_upload_transform(GstBaseTransform *base_transform, GstBuffer *inbuf, GstBuffer *outbuf)
{
    GstHailoUpload *hailoupload = GST_HAILO_UPLOAD(base_transform);

    GST_DEBUG_OBJECT(hailoupload, "Transforming hailo DSP buffer");

    GstCaps *incaps, *outcaps;
    incaps = gst_pad_get_current_caps(base_transform->sinkpad);
    outcaps = gst_pad_get_current_caps(base_transform->srcpad);
    GstVideoInfo input_video_info;
    gst_video_info_from_caps(&input_video_info, incaps);

    GST_DEBUG_OBJECT(hailoupload, "Performing memcopy to DSP contiguous memory");

    GstVideoFrame video_frame;
    gst_video_frame_map(&video_frame, &input_video_info, inbuf, GST_MAP_READ);

    GstVideoFormat format = GST_VIDEO_FRAME_FORMAT(&video_frame);
    size_t image_height = GST_VIDEO_FRAME_HEIGHT(&video_frame);
    size_t n_planes = GST_VIDEO_FRAME_N_PLANES(&video_frame);
    GstFlowReturn ret = GST_FLOW_OK;

    switch (format)
    {
    case GST_VIDEO_FORMAT_NV12:
    {
        // Validate the number of planes
        if (n_planes != 2)
        {
            GST_ERROR_OBJECT(hailoupload, "Invalid number of planes for NV12 format");
            ret = GST_FLOW_ERROR;
            break;
        }

        // Gather Y channel info
        void *y_channel_data = (void *)GST_VIDEO_FRAME_PLANE_DATA(&video_frame, 0);
        size_t y_channel_stride = GST_VIDEO_FRAME_PLANE_STRIDE(&video_frame, 0);
        size_t y_channel_size = y_channel_stride * image_height;

        // Gather UV channel info
        void *uv_channel_data = (void *)GST_VIDEO_FRAME_PLANE_DATA(&video_frame, 1);
        size_t uv_channel_stride = GST_VIDEO_FRAME_PLANE_STRIDE(&video_frame, 1);
        size_t uv_channel_size = uv_channel_stride * image_height / 2;

        // Copy Y channel to the contiguous memory
        gsize size_copied = gst_buffer_fill(outbuf, 0, y_channel_data, y_channel_size);
        GST_DEBUG_OBJECT(hailoupload, "NV12 format: Copied Y channel %d bytes from input buffer to output buffer", (int)size_copied);

        // Copy UV channel to the contiguous memory
        size_copied = gst_buffer_fill(outbuf, y_channel_size, uv_channel_data, uv_channel_size);
        GST_DEBUG_OBJECT(hailoupload, "NV12 format: Copied UV channel %d bytes from input buffer to output buffer", (int)size_copied);
        break;
    }
    case GST_VIDEO_FORMAT_RGB:
    {
        void *date = (void *)GST_VIDEO_FRAME_PLANE_DATA(&video_frame, 0);
        size_t stride = GST_VIDEO_FRAME_PLANE_STRIDE(&video_frame, 0);
        size_t channel_size = stride * image_height;
        gsize size_copied = gst_buffer_fill(outbuf, 0, date, channel_size);
        GST_DEBUG_OBJECT(hailoupload, "RGB format: Copied %ld bytes to DSP memory", size_copied);
        break;
    }
    default:
    {
        GST_ERROR_OBJECT(hailoupload, "Unsupported video format");
        ret = GST_FLOW_ERROR;
        break;
    }
    }

    gst_video_frame_unmap(&video_frame);
    gst_caps_unref(incaps);
    gst_caps_unref(outcaps);

    return ret;
}