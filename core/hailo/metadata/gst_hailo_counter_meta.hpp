/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_HAILO_COUNTER_META_API_TYPE (gst_hailo_counter_meta_api_get_type())
#define GST_HAILO_COUNTER_META_INFO (gst_hailo_counter_meta_get_info())

typedef struct _GstHailoCounterMeta GstHailoCounterMeta;
typedef struct _GstHailoCounter GstHailoCounter;

struct _GstHailoCounterMeta
{

    GstMeta meta;
    guint counter;
};

GType gst_hailo_counter_meta_api_get_type(void);

GST_EXPORT
const GstMetaInfo *gst_hailo_counter_meta_get_info(void);

GST_EXPORT
GstHailoCounterMeta *gst_buffer_add_hailo_counter_meta(GstBuffer *buffer, guint number_of_crops);

GST_EXPORT
gboolean gst_buffer_remove_hailo_counter_meta(GstBuffer *buffer);

GST_EXPORT
GstHailoCounterMeta *gst_buffer_get_hailo_counter_meta(GstBuffer *b);

G_END_DECLS