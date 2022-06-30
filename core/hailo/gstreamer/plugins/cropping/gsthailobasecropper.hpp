/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once
#include <map>
#include <gst/gst.h>
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

#define HAILO_BASE_CROPPER_SUPPORTED_FORMATS "{ RGB, YUY2 }"
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
    GstPad *sinkpad, *srcpad_crop, *srcpad_main;
    std::map<std::string, int> stream_ids_buff_offset;
};

struct _GstHailoBaseCropperClass
{
    GstElementClass parent_class;

    std::vector<HailoROIPtr> (*prepare_crops) (GstHailoBaseCropper *hailocropper,  GstBuffer *buf);
    void (*resize) (GstHailoBaseCropper *basecropper, cv::Mat &cropped_image, cv::Mat &resized_image, HailoROIPtr roi);
};

G_GNUC_INTERNAL GType gst_hailo_basecropper_get_type(void);
void resize_yuy2(cv::Mat &cropped_image, cv::Mat &resized_image, int interpolation);
void resize_bilinear(GstHailoBaseCropper *basecropper, cv::Mat &cropped_image, cv::Mat &resized_image, HailoROIPtr roi);
void resize_nearest_neighbor(GstHailoBaseCropper *basecropper, cv::Mat &cropped_image, cv::Mat &resized_image, HailoROIPtr roi);
void resize_letterbox(GstHailoBaseCropper *basecropper, cv::Mat &cropped_image, cv::Mat &resized_image, HailoROIPtr roi);

G_END_DECLS
