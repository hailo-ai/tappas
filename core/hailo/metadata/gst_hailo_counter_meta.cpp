/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include "gst_hailo_counter_meta.hpp"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

static gboolean gst_hailo_counter_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer);
static void gst_hailo_counter_meta_free(GstMeta *meta, GstBuffer *buffer);
static gboolean gst_hailo_counter_meta_transform(GstBuffer *transbuf, GstMeta *meta, GstBuffer *buffer,
                                                  GQuark type, gpointer data);

GType gst_hailo_counter_meta_api_get_type(void)
{
    static const gchar *tags[] = {NULL};
    static volatile GType type;
    if (g_once_init_enter(const_cast<GType *>(&type)))
    {
        GType _type = gst_meta_api_type_register("GstHailoCounterMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }
    return type;
}

const GstMetaInfo *gst_hailo_counter_meta_get_info(void)
{
    static const GstMetaInfo *gst_hailo_counter_meta_info = NULL;

    if (g_once_init_enter(&gst_hailo_counter_meta_info))
    {
        const GstMetaInfo *meta = gst_meta_register(GST_HAILO_COUNTER_META_API_TYPE, /* api type */
                                                    "GstHailoCounterMeta",           /* implementation type */
                                                    sizeof(GstHailoCounterMeta),     /* size of the structure */
                                                    gst_hailo_counter_meta_init,
                                                    gst_hailo_counter_meta_free,
                                                    gst_hailo_counter_meta_transform);
        g_once_init_leave(&gst_hailo_counter_meta_info, meta);
    }
    return gst_hailo_counter_meta_info;
}

static gboolean gst_hailo_counter_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer)
{
    GstHailoCounterMeta *gst_hailo_counter_meta = (GstHailoCounterMeta *)meta;
    gst_hailo_counter_meta->counter = 0;
    return TRUE;
}

static void gst_hailo_counter_meta_free(GstMeta *meta, GstBuffer *buffer)
{
    GstHailoCounterMeta *gst_hailo_counter_meta = (GstHailoCounterMeta *)meta;
    gst_hailo_counter_meta->counter = 0;
}

static gboolean gst_hailo_counter_meta_transform(GstBuffer *transbuf, GstMeta *meta, GstBuffer *buffer,
                                                  GQuark type, gpointer data)
{
    GstHailoCounterMeta *gst_hailo_counter_meta = (GstHailoCounterMeta *)meta;
    gst_buffer_add_hailo_counter_meta(transbuf, gst_hailo_counter_meta->counter);
    return TRUE;
}

GstHailoCounterMeta *gst_buffer_get_hailo_counter_meta(GstBuffer *buffer)
{
    GstHailoCounterMeta *meta = (GstHailoCounterMeta *)gst_buffer_get_meta((buffer), GST_HAILO_COUNTER_META_API_TYPE);
    return meta;
}

GstHailoCounterMeta *gst_buffer_add_hailo_counter_meta(GstBuffer *buffer, guint counter)
{
    GstHailoCounterMeta *gst_hailo_counter_meta = NULL;

    g_return_val_if_fail((int)GST_IS_BUFFER(buffer), NULL);

    if (!gst_buffer_is_writable(buffer))
        return gst_hailo_counter_meta;

    gst_hailo_counter_meta = (GstHailoCounterMeta *)gst_buffer_add_meta(buffer, GST_HAILO_COUNTER_META_INFO, NULL);

    gst_hailo_counter_meta->counter = counter;

    return gst_hailo_counter_meta;
}

gboolean gst_buffer_remove_hailo_counter_meta(GstBuffer *buffer)
{
    g_return_val_if_fail((int)GST_IS_BUFFER(buffer), false);

    GstHailoCounterMeta *meta = (GstHailoCounterMeta *)gst_buffer_get_meta((buffer), GST_HAILO_COUNTER_META_API_TYPE);

    if (meta == NULL)
        return TRUE;

    if (!gst_buffer_is_writable(buffer))
        return FALSE;

    return gst_buffer_remove_meta(buffer, &meta->meta);
}
