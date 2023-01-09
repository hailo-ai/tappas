/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#pragma once

#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define GST_TYPE_HAILO_COUNTER (gst_hailocounter_get_type())
#define GST_HAILO_COUNTER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_COUNTER, GstHailoCounter))
#define GST_HAILO_COUNTER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_COUNTER, GstHailoCounterClass))
#define GST_IS_HAILO_COUNTER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_COUNTER))
#define GST_IS_HAILO_COUNTER_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_COUNTER))

typedef struct _GstHailoCounter GstHailoCounter;
typedef struct _GstHailoCounterClass GstHailoCounterClass;

struct _GstHailoCounter
{
    GstBaseTransform base_hailocounter;
    guint counter;
};

struct _GstHailoCounterClass
{
    GstBaseTransformClass base_hailocounter_class;
};

GType gst_hailocounter_get_type(void);

G_END_DECLS