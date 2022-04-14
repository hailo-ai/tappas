/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include "gst_hailo_cropping_meta.hpp"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

static gboolean gst_hailo_cropping_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer);
static void gst_hailo_cropping_meta_free(GstMeta *meta, GstBuffer *buffer);
static gboolean gst_hailo_cropping_meta_transform(GstBuffer *transbuf, GstMeta *meta, GstBuffer *buffer,
                                                  GQuark type, gpointer data);

GType gst_hailo_cropping_meta_api_get_type(void)
{
    static const gchar *tags[] = {NULL};
    static volatile GType type;
    if (g_once_init_enter(const_cast<GType *>(&type)))
    {
        GType _type = gst_meta_api_type_register("GstHailoCroppingMetaAPI", tags);
        g_once_init_leave(&type, _type);
    }
    return type;
}

const GstMetaInfo *gst_hailo_cropping_meta_get_info(void)
{
    static const GstMetaInfo *gst_hailo_cropping_meta_info = NULL;

    if (g_once_init_enter(&gst_hailo_cropping_meta_info))
    {
        const GstMetaInfo *meta = gst_meta_register(GST_HAILO_CROPPING_META_API_TYPE, /* api type */
                                                    "GstHailoCroppingMeta",           /* implementation type */
                                                    sizeof(GstHailoCroppingMeta),     /* size of the structure */
                                                    gst_hailo_cropping_meta_init,
                                                    gst_hailo_cropping_meta_free,
                                                    gst_hailo_cropping_meta_transform);
        g_once_init_leave(&gst_hailo_cropping_meta_info, meta);
    }
    return gst_hailo_cropping_meta_info;
}

static gboolean gst_hailo_cropping_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer)
{
    GstHailoCroppingMeta *gst_hailo_cropping_meta = (GstHailoCroppingMeta *)meta;
    gst_hailo_cropping_meta->num_of_crops = 0;
    return TRUE;
}

static void gst_hailo_cropping_meta_free(GstMeta *meta, GstBuffer *buffer)
{
    GstHailoCroppingMeta *gst_hailo_cropping_meta = (GstHailoCroppingMeta *)meta;
    gst_hailo_cropping_meta->num_of_crops = 0;
}

static gboolean gst_hailo_cropping_meta_transform(GstBuffer *transbuf, GstMeta *meta, GstBuffer *buffer,
                                                  GQuark type, gpointer data)
{
    GstHailoCroppingMeta *gst_hailo_cropping_meta = (GstHailoCroppingMeta *)meta;
    gst_buffer_add_hailo_cropping_meta(transbuf, gst_hailo_cropping_meta->num_of_crops);
    return TRUE;
}

GstHailoCroppingMeta *gst_buffer_get_hailo_cropping_meta(GstBuffer *buffer)
{
    GstHailoCroppingMeta *meta = (GstHailoCroppingMeta *)gst_buffer_get_meta((buffer), GST_HAILO_CROPPING_META_API_TYPE);
    return meta;
}

GstHailoCroppingMeta *gst_buffer_add_hailo_cropping_meta(GstBuffer *buffer, guint number_of_crops)
{
    GstHailoCroppingMeta *gst_hailo_cropping_meta = NULL;

    g_return_val_if_fail((int)GST_IS_BUFFER(buffer), NULL);

    if (!gst_buffer_is_writable(buffer))
        return gst_hailo_cropping_meta;

    gst_hailo_cropping_meta = (GstHailoCroppingMeta *)gst_buffer_add_meta(buffer, GST_HAILO_CROPPING_META_INFO, NULL);

    gst_hailo_cropping_meta->num_of_crops = number_of_crops;

    return gst_hailo_cropping_meta;
}

gboolean gst_buffer_remove_hailo_cropping_meta(GstBuffer *buffer)
{
    g_return_val_if_fail((int)GST_IS_BUFFER(buffer), false);

    GstHailoCroppingMeta *meta = (GstHailoCroppingMeta *)gst_buffer_get_meta((buffer), GST_HAILO_CROPPING_META_API_TYPE);

    if (meta == NULL)
        return TRUE;

    if (!gst_buffer_is_writable(buffer))
        return FALSE;

    return gst_buffer_remove_meta(buffer, &meta->meta);
}
