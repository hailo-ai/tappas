#include "dsp/gsthailodspbasetransform.hpp"
#include "dsp/upload/gsthailoupload.hpp"
#include <gst/gst.h>
#include <gst/video/video.h>
#include "dsp/gsthailodsp.h"
#include <sys/ioctl.h>
#include <fcntl.h>

GST_DEBUG_CATEGORY_STATIC(gst_hailo_upload_debug);
#define GST_CAT_DEFAULT gst_hailo_upload_debug

enum
{
    PROP_0,
    PROP_USE_GPDMA,
};

#define _do_init \
    GST_DEBUG_CATEGORY_INIT(gst_hailo_upload_debug, "hailoupload", 0, "Hailo Upload");
#define gst_hailo_upload_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstHailoUpload, gst_hailo_upload, GST_TYPE_HAILO_DSP_BASE_TRANSFORM, _do_init);

#define DMA_MEMCPY_DEVICE "/dev/dma_memcpy"
#define GP_DMA_XFER _IOWR('a', 'a', struct channel_buffer_info *)

struct channel_buffer_info {
	unsigned long virt_src_addr;
	unsigned long virt_dst_addr;
	unsigned long length;
	int status;
};

static GstFlowReturn
gst_hailo_upload_transform(GstBaseTransform *base_transform, GstBuffer *inbuf, GstBuffer *outbuf);

static void gst_hailo_upload_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_hailo_upload_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);
static gboolean gst_hailo_upload_start(GstBaseTransform *base_transform);
static gboolean gst_hailo_upload_stop(GstBaseTransform *base_transform);

static void
gst_hailo_upload_class_init(GstHailoUploadClass *klass)
{
    GObjectClass *const object_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *const base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);

    object_class->set_property = gst_hailo_upload_set_property;
    object_class->get_property = gst_hailo_upload_get_property;

    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_hailo_upload_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_hailo_upload_stop);
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

    g_object_class_install_property(object_class, PROP_USE_GPDMA,
                                    g_param_spec_boolean("use-gpdma", "Use GPDMA",
                                                         "Use GPDMA for memory copy", FALSE,
                                                         (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
}

static void
gst_hailo_upload_init(GstHailoUpload *self)
{
    self->dma_memcpy_fd = 0;
}

static gboolean gst_hailo_upload_start(GstBaseTransform *base_transform)
{
    GstHailoUpload *self = GST_HAILO_UPLOAD(base_transform);

    if (self->use_gpdma)
    {
        int memcpy_device_open_ret = open(DMA_MEMCPY_DEVICE, O_WRONLY);

        if (memcpy_device_open_ret == -1) {
            GST_ERROR_OBJECT(self, "Failed to open %s - %d", DMA_MEMCPY_DEVICE, self->dma_memcpy_fd);
            return FALSE;
        }

        self->dma_memcpy_fd = memcpy_device_open_ret;
    }

    return TRUE;
}

static gboolean gst_hailo_upload_stop(GstBaseTransform *base_transform)
{
    GstHailoUpload *self = GST_HAILO_UPLOAD(base_transform);

    if (self->dma_memcpy_fd != 0) {
        int close_ret = close(self->dma_memcpy_fd);

        if (close_ret) {
            GST_ERROR_OBJECT(self, "Failed to close %s [%d]", DMA_MEMCPY_DEVICE, close_ret);
            return FALSE;
        }
    }

    return TRUE;
}

static void gst_hailo_upload_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GstHailoUpload *self = GST_HAILO_UPLOAD(object);

    switch (prop_id)
    {
    case PROP_USE_GPDMA:
        {
            self->use_gpdma = g_value_get_boolean(value);
            break;
        }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_hailo_upload_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GstHailoUpload *self = GST_HAILO_UPLOAD(object);

    switch (prop_id)
    {
    case PROP_USE_GPDMA:
        {
            g_value_set_boolean(value, self->use_gpdma);
            break;
        }
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static GstFlowReturn gpdma_copy_channel(GstHailoUpload *hailoupload,
                                        void *channel_data,
                                        void *output_channel_data,
                                        size_t channel_size)
{
    struct channel_buffer_info ioctl_data = {
        .virt_src_addr = (unsigned long)channel_data,
        .virt_dst_addr = (unsigned long)output_channel_data,
        .length = channel_size,
        .status = 0,
    };

    GST_LOG_OBJECT(hailoupload, "virt_src_addr: %lu, virt_dst_addr: %lu, length: %lu, status: %d",
                   ioctl_data.virt_src_addr, ioctl_data.virt_dst_addr,
                   ioctl_data.length, ioctl_data.status);

    int ioctl_ret = ioctl(hailoupload->dma_memcpy_fd, GP_DMA_XFER, &ioctl_data);

    if (ioctl_ret)
    {
        GST_ERROR_OBJECT(hailoupload, "ioctl fail with return value %d and status %d", ioctl_ret, ioctl_data.status);
        return GST_FLOW_ERROR;
    }

    if (ioctl_data.status != 0)
    {
        GST_ERROR_OBJECT(hailoupload, "Failed to copy channel %d", ioctl_data.status);
        return GST_FLOW_ERROR;
    }

    GST_DEBUG_OBJECT(hailoupload, "Channel copied successfully");

    return GST_FLOW_OK;
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

    GstVideoFrame video_frame_input, video_frame_output;
    gst_video_frame_map(&video_frame_input, &input_video_info, inbuf, GST_MAP_READ);
    
    if (hailoupload->use_gpdma)
    {
        // There is no need to map the output buffer if we are not using GPDMA
        gst_video_frame_map(&video_frame_output, &input_video_info, outbuf, GST_MAP_WRITE);
    }

    GstVideoFormat format = GST_VIDEO_FRAME_FORMAT(&video_frame_input);
    size_t image_height = GST_VIDEO_FRAME_HEIGHT(&video_frame_input);
    size_t n_planes = GST_VIDEO_FRAME_N_PLANES(&video_frame_input);
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

        void *y_input_channel_data = (void *)GST_VIDEO_FRAME_PLANE_DATA(&video_frame_input, 0);
        size_t y_channel_stride = GST_VIDEO_FRAME_PLANE_STRIDE(&video_frame_input, 0);
        size_t y_channel_size = y_channel_stride * image_height;

        void *uv_channel_data = (void *)GST_VIDEO_FRAME_PLANE_DATA(&video_frame_input, 1);
        size_t uv_channel_stride = GST_VIDEO_FRAME_PLANE_STRIDE(&video_frame_input, 1);
        size_t uv_channel_size = uv_channel_stride * image_height / 2;

        if (hailoupload->use_gpdma)
        {
            GST_DEBUG_OBJECT(hailoupload, "Using GPDMA for memory copy");

            void *y_output_channel_data = (void *)GST_VIDEO_FRAME_PLANE_DATA(&video_frame_output, 0);
            void *uv_output_channel_data = (void *)GST_VIDEO_FRAME_PLANE_DATA(&video_frame_output, 1);

            if (gpdma_copy_channel(hailoupload, y_input_channel_data, y_output_channel_data, y_channel_size) != GST_FLOW_OK)
            {
                GST_ERROR_OBJECT(hailoupload, "Failed to copy Y channel");
                ret = GST_FLOW_ERROR;
                break;
            }

            if (gpdma_copy_channel(hailoupload, uv_channel_data, uv_output_channel_data, uv_channel_size) != GST_FLOW_OK)
            {
                GST_ERROR_OBJECT(hailoupload, "Failed to copy UV channel");
                ret = GST_FLOW_ERROR;
                break;
            }

            break;
        }
        else 
        {
            GST_DEBUG_OBJECT(hailoupload, "Using CPU for memory copy");

            // Copy Y channel to the contiguous memory
            gsize size_copied = gst_buffer_fill(outbuf, 0, y_input_channel_data, y_channel_size);
            GST_DEBUG_OBJECT(hailoupload, "NV12 format: Copied Y channel %d bytes from input buffer to output buffer", (int)size_copied);

            // Copy UV channel to the contiguous memory
            size_copied = gst_buffer_fill(outbuf, y_channel_size, uv_channel_data, uv_channel_size);
            GST_DEBUG_OBJECT(hailoupload, "NV12 format: Copied UV channel %d bytes from input buffer to output buffer", (int)size_copied);
            break;
        }
    }
    case GST_VIDEO_FORMAT_RGB:
    {
        if (hailoupload->use_gpdma)
        {
            GST_ERROR_OBJECT(hailoupload, "RGB format is not supported yet in GPDMA");
            ret = GST_FLOW_ERROR;
            break;
        }

        void *date = (void *)GST_VIDEO_FRAME_PLANE_DATA(&video_frame_input, 0);
        size_t stride = GST_VIDEO_FRAME_PLANE_STRIDE(&video_frame_input, 0);
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

    gst_video_frame_unmap(&video_frame_input);

    if (hailoupload->use_gpdma)
    {
        gst_video_frame_unmap(&video_frame_output);
    }

    gst_caps_unref(incaps);
    gst_caps_unref(outcaps);

    return ret;
}