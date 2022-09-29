/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once
#include <gst/gst.h>

G_BEGIN_DECLS


#define GST_HAILO_STREAM_META_API_TYPE (gst_hailo_stream_meta_api_get_type())
#define GST_HAILO_STREAM_META_INFO (gst_hailo_stream_meta_get_info())

typedef struct _GstHailoStreamMeta GstHailoStreamMeta;
typedef struct _GstHailoStream GstHailoStream;

struct _GstHailoStreamMeta
{

    GstMeta meta;
    gchar *pad_name;
    gchar *stream_id;
};

GType gst_hailo_stream_meta_api_get_type(void);

GST_EXPORT
const GstMetaInfo *gst_hailo_stream_meta_get_info(void);

GST_EXPORT
GstHailoStreamMeta *gst_buffer_add_hailo_stream_meta(GstBuffer *buffer, const gchar *pad_name, const gchar *stream_id);

GST_EXPORT
gboolean gst_buffer_remove_hailo_stream_meta(GstBuffer *buffer);

GST_EXPORT
GstHailoStreamMeta *gst_buffer_get_hailo_stream_meta(GstBuffer *b);

G_END_DECLS