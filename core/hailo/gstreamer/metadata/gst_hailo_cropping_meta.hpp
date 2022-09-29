/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_HAILO_CROPPING_META_API_TYPE (gst_hailo_cropping_meta_api_get_type())
#define GST_HAILO_CROPPING_META_INFO (gst_hailo_cropping_meta_get_info())

typedef struct _GstHailoCroppingMeta GstHailoCroppingMeta;
typedef struct _GstHailoCropping GstHailoCropping;

struct _GstHailoCroppingMeta
{

    GstMeta meta;
    guint num_of_crops;
};

GType gst_hailo_cropping_meta_api_get_type(void);

GST_EXPORT
const GstMetaInfo *gst_hailo_cropping_meta_get_info(void);

GST_EXPORT
GstHailoCroppingMeta *gst_buffer_add_hailo_cropping_meta(GstBuffer *buffer, guint number_of_crops);

GST_EXPORT
gboolean gst_buffer_remove_hailo_cropping_meta(GstBuffer *buffer);

GST_EXPORT
GstHailoCroppingMeta *gst_buffer_get_hailo_cropping_meta(GstBuffer *b);

G_END_DECLS