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

#define GST_TYPE_HAILO_NV12_TO_GRAY (gst_hailonv12togray_get_type())
#define GST_HAILO_NV12_TO_GRAY(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_NV12_TO_GRAY, GstHailonv12togray))
#define GST_HAILO_NV12_TO_GRAY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_NV12_TO_GRAY, GstHailonv12tograyClass))
#define GST_IS_HAILO_NV12_TO_GRAY(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_NV12_TO_GRAY))
#define GST_IS_HAILO_NV12_TO_GRAY_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_NV12_TO_GRAY))

typedef struct _GstHailonv12togray GstHailonv12togray;
typedef struct _GstHailonv12tograyClass GstHailonv12tograyClass;

struct _GstHailonv12togray
{
    GstBaseTransform base_hailonv12togray;
};

struct _GstHailonv12tograyClass
{
    GstBaseTransformClass base_hailonv12togray_class;
};

GType gst_hailonv12togray_get_type(void);

G_END_DECLS