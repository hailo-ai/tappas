/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once

#include <gst/gst.h>
#include "hailo_objects.hpp"

G_BEGIN_DECLS

// Api Type
// First field of gst_meta_register (which returns GstMetaInfo)
// https://gstreamer.freedesktop.org/documentation/gstreamer/gstmeta.html?gi-language=c#gst_meta_register
#define GST_HAILO_META_API_TYPE (gst_hailo_meta_api_get_type())
#define GST_HAILO_META_INFO (gst_hailo_meta_get_info())

typedef struct _GstHailoMeta GstHailoMeta;

struct _GstHailoMeta
{

    // Required as it is base structure for metadata
    // https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/gstreamer-GstMeta.html
    GstMeta meta;
    // Custom fields
    HailoMainObjectPtr main_object;
};

GType gst_hailo_meta_api_get_type(void);

GST_EXPORT
const GstMetaInfo *gst_hailo_meta_get_info(void);

GST_EXPORT
GstHailoMeta *gst_buffer_add_hailo_meta(GstBuffer *buffer, HailoMainObjectPtr ptr);

GST_EXPORT
gboolean gst_buffer_remove_hailo_meta(GstBuffer *buffer);

GST_EXPORT
GstHailoMeta *gst_buffer_get_hailo_meta(GstBuffer *b);

HailoROIPtr get_hailo_main_roi(GstBuffer *buffer, gboolean create_if_missing = false);

G_END_DECLS