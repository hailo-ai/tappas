/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include "gst_buffer_info_meta.hpp"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

static gboolean gst_buffer_info_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer);
static gboolean gst_buffer_info_meta_transform(GstBuffer *transbuf, GstMeta *meta, GstBuffer *buffer,
                                               GQuark type, gpointer data);

GstBufferInfo *empty()
{
    static GstBufferInfo info;
    info.description = (gchar *)"";
    return &info;
}

// Register metadata type and returns Gtype
// https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/gstreamer-GstMeta.html#gst-meta-api-type-register
GType gst_buffer_info_meta_api_get_type(void)
{
    static const gchar *tags[] = {NULL};
    static volatile GType type;
    if (g_once_init_enter(const_cast<GType *>(&type)))
    {
        GType _type = gst_meta_api_type_register("GstBufferInfoMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }
    return type;
}

// GstMetaInfo provides info for specific metadata implementation
// https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/gstreamer-GstMeta.html#GstMetaInfo

const GstMetaInfo *gst_buffer_info_meta_get_info(void)
{
    static const GstMetaInfo *gst_buffer_info_meta_info = NULL;

    if (g_once_init_enter(&gst_buffer_info_meta_info))
    {
        // Explanation of fields
        // https://gstreamer.freedesktop.org/documentation/design/meta.html#gstmeta1
        const GstMetaInfo *meta = gst_meta_register(GST_BUFFER_INFO_META_API_TYPE, /* api type */
                                                    "GstBufferInfoMeta",           /* implementation type */
                                                    sizeof(GstBufferInfoMeta),     /* size of the structure */
                                                    gst_buffer_info_meta_init,
                                                    (GstMetaFreeFunction)NULL,
                                                    gst_buffer_info_meta_transform);
        g_once_init_leave(&gst_buffer_info_meta_info, meta);
    }
    return gst_buffer_info_meta_info;
}

// Meta init function
// 4-th field in GstMetaInfo
static gboolean gst_buffer_info_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer)
{
    GstBufferInfoMeta *gst_buffer_info_meta = (GstBufferInfoMeta *)meta;
    gst_buffer_info_meta->info.description = (gchar *)"";
    return TRUE;
}

// Meta transform function
// 5-th field in GstMetaInfo
// https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/gstreamer-GstMeta.html#GstMetaTransformFunction
static gboolean gst_buffer_info_meta_transform(GstBuffer *transbuf, GstMeta *meta, GstBuffer *buffer,
                                               GQuark type, gpointer data)
{
    GstBufferInfoMeta *gst_buffer_info_meta = (GstBufferInfoMeta *)meta;
    gst_buffer_add_buffer_info_meta(transbuf, &(gst_buffer_info_meta->info));
    return TRUE;
}

// Only for Python : return GstBufferInfo instead of GstBufferInfoMeta
// To avoid GstMeta (C) map to Gst.Meta (Python)
GstBufferInfo *gst_buffer_get_buffer_info_meta(GstBuffer *buffer)
{
    // https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/GstBuffer.html#gst-buffer-get-meta
    GstBufferInfoMeta *meta = (GstBufferInfoMeta *)gst_buffer_get_meta((buffer), GST_BUFFER_INFO_META_API_TYPE);

    if (meta == NULL)
        return empty();
    else
        return &meta->info;
}

GstBufferInfoMeta *gst_buffer_add_buffer_info_meta(GstBuffer *buffer, GstBufferInfo *buffer_info)
{
    GstBufferInfoMeta *gst_buffer_info_meta = NULL;

    // check that gst_buffer valid
    g_return_val_if_fail((int)GST_IS_BUFFER(buffer), NULL);

    // check that gst_buffer writable
    if (!gst_buffer_is_writable(buffer))
        return gst_buffer_info_meta;

    // https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/GstBuffer.html#gst-buffer-add-meta
    gst_buffer_info_meta = (GstBufferInfoMeta *)gst_buffer_add_meta(buffer, GST_BUFFER_INFO_META_INFO, NULL);

    // copy fields to buffer's meta
    if (buffer_info->description != NULL)
    {
        gst_buffer_info_meta->info.description = (gchar *)malloc(strlen(buffer_info->description) + 1);
        strcpy(gst_buffer_info_meta->info.description, buffer_info->description);
    }

    return gst_buffer_info_meta;
}

// Removes metadata (GstBufferInfo) from buffer
gboolean gst_buffer_remove_buffer_info_meta(GstBuffer *buffer)
{
    g_return_val_if_fail((int)GST_IS_BUFFER(buffer), false);

    GstBufferInfoMeta *meta = (GstBufferInfoMeta *)gst_buffer_get_meta((buffer), GST_BUFFER_INFO_META_API_TYPE);

    if (meta == NULL)
        return TRUE;

    if (!gst_buffer_is_writable(buffer))
        return FALSE;

    // https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/GstBuffer.html#gst-buffer-remove-meta
    return gst_buffer_remove_meta(buffer, &meta->meta);
}
