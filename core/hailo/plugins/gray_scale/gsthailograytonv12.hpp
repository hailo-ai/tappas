/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#pragma once

#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>
#include <map>
#include <vector>

G_BEGIN_DECLS

#define GST_TYPE_HAILO_GRAY_TO_NV12 (gst_hailograytonv12_get_type())
#define GST_HAILO_GRAY_TO_NV12(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_GRAY_TO_NV12, GstHailograytonv12))
#define GST_HAILO_GRAY_TO_NV12_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_GRAY_TO_NV12, GstHailograytonv12Class))
#define GST_IS_HAILO_GRAY_TO_NV12(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_GRAY_TO_NV12))
#define GST_IS_HAILO_GRAY_TO_NV12_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_GRAY_TO_NV12))

typedef struct _GstHailograytonv12 GstHailograytonv12;
typedef struct _GstHailograytonv12Class GstHailograytonv12Class;

struct _GstHailograytonv12
{
    GstBaseTransform base_hailograytonv12;
};

struct _GstHailograytonv12Class
{
    GstBaseTransformClass base_hailograytonv12_class;
};

GType gst_hailograytonv12_get_type(void);

G_END_DECLS