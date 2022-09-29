/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include "gst_hailo_meta.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static gboolean gst_hailo_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer);
static void gst_hailo_meta_free(GstMeta *meta, GstBuffer *buffer);
static gboolean gst_hailo_meta_transform(GstBuffer *transbuf, GstMeta *meta, GstBuffer *buffer,
                                         GQuark type, gpointer data);

// Register metadata type and returns Gtype
// https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/gstreamer-GstMeta.html#gst-meta-api-type-register
GType gst_hailo_meta_api_get_type(void)
{
    static const gchar *tags[] = {NULL};
    static volatile GType type;
    if (g_once_init_enter(const_cast<GType *>(&type)))
    {
        GType _type = gst_meta_api_type_register("GstHailoMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }
    return type;
}

// GstMetaInfo provides info for specific metadata implementation
// https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/gstreamer-GstMeta.html#GstMetaInfo
const GstMetaInfo *gst_hailo_meta_get_info(void)
{
    static const GstMetaInfo *gst_hailo_meta_info = NULL;

    if (g_once_init_enter(&gst_hailo_meta_info))
    {
        // Explanation of fields
        // https://gstreamer.freedesktop.org/documentation/design/meta.html#gstmeta1
        const GstMetaInfo *meta = gst_meta_register(GST_HAILO_META_API_TYPE, /* api type */
                                                    "GstHailoMeta",          /* implementation type */
                                                    sizeof(GstHailoMeta),    /* size of the structure */
                                                    gst_hailo_meta_init,
                                                    (GstMetaFreeFunction)gst_hailo_meta_free,
                                                    gst_hailo_meta_transform);
        g_once_init_leave(&gst_hailo_meta_info, meta);
    }
    return gst_hailo_meta_info;
}

// Meta init function
// Fourth field in GstMetaInfo
// https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/gstreamer-GstMeta.html#GstMetaInitFunction
static gboolean gst_hailo_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer)
{
    GstHailoMeta *gst_hailo_meta = (GstHailoMeta *)meta;
    // GStreamer is allocating the GstHailoMeta struct with POD allocation (like malloc) when
    // it holds non POD type (shared_ptr). The memset assures there is no garbage data in this address.
    // This is a temporary solution because memset to non POD type is undefined behavior.
    // https://stackoverflow.com/questions/59747240/is-it-okay-to-memset-a-struct-which-has-an-another-struct-with-smart-pointer-mem?rq=1
    // Opened an issue to replace this line with right initialization - MAD-1158.
    memset((void *)&gst_hailo_meta->main_object, 0, sizeof(gst_hailo_meta->main_object));
    gst_hailo_meta->main_object = nullptr;
    return TRUE;
}

// Meta free function
// Fifth field in GstMetaInfo
// https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/gstreamer-GstMeta.html#GstMetaFreeFunction
static void gst_hailo_meta_free(GstMeta *meta, GstBuffer *buffer)
{
    GstHailoMeta *hailo_meta = (GstHailoMeta *)meta;
    hailo_meta->main_object = nullptr;
}

// Meta transform function
// Sixth field in GstMetaInfo
// https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/gstreamer-GstMeta.html#GstMetaTransformFunction
static gboolean gst_hailo_meta_transform(GstBuffer *transbuf, GstMeta *meta, GstBuffer *buffer,
                                         GQuark type, gpointer data)
{
    GstHailoMeta *gst_hailo_meta = (GstHailoMeta *)meta;
    HailoMainObjectPtr main_object = gst_hailo_meta->main_object;

    GstHailoMeta *new_hailo_meta = gst_buffer_add_hailo_meta(transbuf, main_object);
    if(!new_hailo_meta)
    {
        GST_ERROR("gst_hailo_meta_transform: failed to transform hailo_meta");
        return FALSE;
    }

    return TRUE;
}

GstHailoMeta *gst_buffer_get_hailo_meta(GstBuffer *buffer)
{
    // https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/GstBuffer.html#gst-buffer-get-meta
    GstHailoMeta *meta = (GstHailoMeta *)gst_buffer_get_meta((buffer), GST_HAILO_META_API_TYPE);
    return meta;
}
/**
 * @brief Addes a new GstHailoMeta to a given buffer, this meta is initialized with a given HailoMainObjectPtr.
 *
 * @param buffer Buffer to add the metadata on.
 * @param main_object HailoMainObjectPtr to initialize the meta with
 * @return GstHailoMeta* The meta structure that was added to the buffer.
 */
GstHailoMeta *gst_buffer_add_hailo_meta(GstBuffer *buffer, HailoMainObjectPtr main_object)
{
    GstHailoMeta *gst_hailo_meta = NULL;

    // check that gst_buffer valid
    g_return_val_if_fail((int)GST_IS_BUFFER(buffer), NULL);

    // check that gst_buffer writable
    if (!gst_buffer_is_writable(buffer))
        return gst_hailo_meta;

    // https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/GstBuffer.html#gst-buffer-add-meta
    gst_hailo_meta = (GstHailoMeta *)gst_buffer_add_meta(buffer, GST_HAILO_META_INFO, NULL);

    gst_hailo_meta->main_object = main_object;

    return gst_hailo_meta;
}

/**
 * @brief  Removes GstHailoMeta from a given buffer.
 *
 * @param buffer A buffer to remove meta from.
 * @return gboolean whether removal was successfull (TRUE if there isn't GstHailoMeta).
 * @note Removes only the first GstHailoMeta in this buffer.
 */
gboolean gst_buffer_remove_hailo_meta(GstBuffer *buffer)
{
    g_return_val_if_fail((int)GST_IS_BUFFER(buffer), false);

    GstHailoMeta *meta = (GstHailoMeta *)gst_buffer_get_meta((buffer), GST_HAILO_META_API_TYPE);

    if (meta == NULL)
        return TRUE;

    if (!gst_buffer_is_writable(buffer))
        return FALSE;

    // https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/GstBuffer.html#gst-buffer-remove-meta
    return gst_buffer_remove_meta(buffer, &meta->meta);
}

HailoROIPtr get_hailo_main_roi(GstBuffer *buffer, gboolean create_if_missing)
{
    GstHailoMeta *meta = gst_buffer_get_hailo_meta(buffer);
    HailoROIPtr roi = nullptr;
    if (meta)
    {
        roi = std::dynamic_pointer_cast<HailoROI>(meta->main_object);
    }
    if ((!roi) && (create_if_missing))
    {
        roi = std::make_shared<HailoROI>(HailoROI(HailoBBox(0.0f, 0.0f, 1.0f, 1.0f)));
        gst_buffer_add_hailo_meta(buffer, roi);
    }

    return roi;
}