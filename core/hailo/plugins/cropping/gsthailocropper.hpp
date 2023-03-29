/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once

#include <gst/gst.h>
#include <opencv2/opencv.hpp>
#include <dlfcn.h>
#include "cropping/gsthailobasecropper.hpp"
#include "hailomat.hpp"

G_BEGIN_DECLS

#define GST_TYPE_HAILO_CROPPER (gst_hailocropper_get_type())
#define GST_HAILO_CROPPER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_CROPPER, GstHailoCropper))
#define GST_HAILO_CROPPER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_CROPPER, GstHailoCropperClass))
#define GST_IS_HAILO_CROPPER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_CROPPER))
#define GST_IS_HAILO_CROPPER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_CROPPER))
#define GST_HAILO_CROPPER_CAST(obj) ((GstHailoCropper *)obj)
#define GST_HAILO_CROPPER_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj),GST_TYPE_HAILO_CROPPER,GstHailoCropperClass))

typedef struct _GstHailoCropper GstHailoCropper;
typedef struct _GstHailoCropperClass GstHailoCropperClass;

struct _GstHailoCropper
{
    GstHailoBaseCropper element;
    gchar *lib_path;
    gchar *function_name;
    gboolean use_letterbox;
    cv::InterpolationFlags method;
    void *loaded_lib;
    std::vector<HailoROIPtr> (*handler)(std::shared_ptr<HailoMat>, HailoROIPtr);
};

struct _GstHailoCropperClass
{
    GstHailoBaseCropperClass parent_class;
};

G_GNUC_INTERNAL GType gst_hailocropper_get_type(void);

G_END_DECLS
