/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once
#include <map>
#include <gst/gst.h>
#include <gst/video/video-format.h>
#include <opencv2/opencv.hpp>
#include "hailo_objects.hpp"

G_BEGIN_DECLS

#define GST_TYPE_HAILO_BASE_CROPPER (gst_hailo_basecropper_get_type())
#define GST_HAILO_BASE_CROPPER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_BASE_CROPPER, GstHailoBaseCropper))
#define GST_HAILO_BASE_CROPPER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_BASE_CROPPER, GstHailoBaseCropperClass))
#define GST_IS_HAILO_BASE_CROPPER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_BASE_CROPPER))
#define GST_IS_HAILO_BASE_CROPPER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_BASE_CROPPER))
#define GST_HAILO_BASE_CROPPER_CAST(obj) ((GstHailoBaseCropper *)obj)
#define GST_HAILO_BASE_CROPPER_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj),GST_TYPE_HAILO_BASE_CROPPER,GstHailoBaseCropperClass))

#define GST_HAILO_CROPPER_MAX_FILTER_STREAMS 40
#define HAILO_BASE_CROPPER_SUPPORTED_FORMATS "{ RGB, RGBA, YUY2, NV12 }"
#define HAILO_BASE_CROPPER_VIDEO_CAPS \
    GST_VIDEO_CAPS_MAKE(HAILO_BASE_CROPPER_SUPPORTED_FORMATS)

typedef struct _GstHailoBaseCropper GstHailoBaseCropper;
typedef struct _GstHailoBaseCropperClass GstHailoBaseCropperClass;

struct _GstHailoBaseCropper
{
    GstElement element;
    gboolean use_internal_offset;
    gboolean drop_uncropped_buffers;
    uint internal_offset;
    uint cropping_period;
    #ifdef HAILO15_TARGET
    bool use_dsp;
    guint bufferpool_max_size;
    guint bufferpool_min_size;
    #endif
    GstBufferPool *buffer_pool;
    uint num_streams_to_filter = 0;
    GstPad *sinkpad, *srcpad_crop, *srcpad_main;
    std::map<std::string, int> stream_ids_buff_offset;
    const gchar *filter_streams[GST_HAILO_CROPPER_MAX_FILTER_STREAMS];
};

struct _GstHailoBaseCropperClass
{
    GstElementClass parent_class;

    std::vector<HailoROIPtr> (*prepare_crops) (GstHailoBaseCropper *hailocropper,  GstBuffer *buf);
    void (*resize) (GstHailoBaseCropper *basecropper, cv::Mat &cropped_image, cv::Mat &resized_image, HailoROIPtr roi, GstVideoFormat image_format);
};

G_GNUC_INTERNAL GType gst_hailo_basecropper_get_type(void);
void resize_normal(cv::InterpolationFlags method, cv::Mat &cropped_image, cv::Mat &resized_image, GstVideoFormat image_format);
void resize_letterbox(cv::InterpolationFlags method, cv::Mat &cropped_image, cv::Mat &resized_image, HailoROIPtr roi, GstVideoFormat image_format);

G_END_DECLS
