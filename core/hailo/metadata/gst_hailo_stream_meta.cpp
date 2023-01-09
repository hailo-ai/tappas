/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "gst_hailo_stream_meta.hpp"

static gboolean gst_hailo_stream_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer);
static void gst_hailo_stream_meta_free(GstMeta *meta, GstBuffer *buffer);
static gboolean gst_hailo_stream_meta_transform(GstBuffer *transbuf, GstMeta *meta, GstBuffer *buffer,
                                                GQuark type, gpointer data);

static gboolean gst_hailo_stream_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer)
{
    GstHailoStreamMeta *stream_meta = (GstHailoStreamMeta *)meta;
    stream_meta->pad_name = NULL;
    stream_meta->stream_id = NULL;
    return TRUE;
}

static void gst_hailo_stream_meta_free(GstMeta *meta, GstBuffer *buffer)
{
    GstHailoStreamMeta *stream_meta = (GstHailoStreamMeta *)meta;
    if (stream_meta->pad_name)
    {
        g_free(stream_meta->pad_name);
        stream_meta->pad_name = NULL;
    }
    if (stream_meta->stream_id)
    {
        g_free(stream_meta->stream_id);
        stream_meta->stream_id = NULL;
    }
}

static gboolean gst_hailo_stream_meta_transform(GstBuffer *transbuf, GstMeta *meta, GstBuffer *buffer,
                                                 GQuark type, gpointer data)
{
    GstHailoStreamMeta *gst_hailo_stream_meta = (GstHailoStreamMeta *)meta;
    gst_buffer_add_hailo_stream_meta(transbuf, gst_hailo_stream_meta->pad_name,  gst_hailo_stream_meta->stream_id);
    return TRUE;
}

GType gst_hailo_stream_meta_api_get_type(void)
{
    static const gchar *tags[] = {NULL};
    static volatile GType type;
    if (g_once_init_enter(const_cast<GType *>(&type)))
    {
        GType _type = gst_meta_api_type_register("GstHailoStreamMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }
    return type;
}

const GstMetaInfo *gst_hailo_stream_meta_get_info(void)
{
    static const GstMetaInfo *gst_hailo_stream_meta_info = NULL;

    if (g_once_init_enter(&gst_hailo_stream_meta_info))
    {
        const GstMetaInfo *meta = gst_meta_register(GST_HAILO_STREAM_META_API_TYPE, /* api type */
                                                    "GstHailoStreamMeta",           /* implementation type */
                                                    sizeof(GstHailoStreamMeta),     /* size of the structure */
                                                    gst_hailo_stream_meta_init,
                                                    gst_hailo_stream_meta_free,
                                                    gst_hailo_stream_meta_transform);
        g_once_init_leave(&gst_hailo_stream_meta_info, meta);
    }
    return gst_hailo_stream_meta_info;
}

GstHailoStreamMeta *gst_buffer_add_hailo_stream_meta(GstBuffer *buffer, const gchar *pad_name, const gchar *stream_id)
{
    GstHailoStreamMeta *stream_meta = NULL;

    g_return_val_if_fail((int)GST_IS_BUFFER(buffer), NULL);
    if (!gst_buffer_is_writable(buffer))
    {
        GST_ERROR("gst_buffer_add_hailo_stream_meta: buffer is not writable");
        return stream_meta;
    }

    stream_meta = (GstHailoStreamMeta *)gst_buffer_add_meta(buffer, GST_HAILO_STREAM_META_INFO, NULL);
    
    stream_meta->pad_name = g_strdup(pad_name);
    stream_meta->stream_id = g_strdup(stream_id);
    return stream_meta;
}

gboolean gst_buffer_remove_hailo_stream_meta(GstBuffer *buffer)
{
    g_return_val_if_fail((int)GST_IS_BUFFER(buffer), false);

    GstHailoStreamMeta *meta = (GstHailoStreamMeta *)gst_buffer_get_meta((buffer), GST_HAILO_STREAM_META_API_TYPE);

    if (meta == NULL)
        return TRUE;

    if (!gst_buffer_is_writable(buffer))
        return FALSE;

    return gst_buffer_remove_meta(buffer, &meta->meta);
}

GstHailoStreamMeta *gst_buffer_get_hailo_stream_meta(GstBuffer *b)
{
    GstHailoStreamMeta *meta = (GstHailoStreamMeta *)gst_buffer_get_meta(b, GST_HAILO_STREAM_META_API_TYPE);
    return meta;
}