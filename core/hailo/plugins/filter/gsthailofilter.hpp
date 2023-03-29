/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#pragma once

#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>
#include <map>
#include <vector>
#include "hailo_objects.hpp"

G_BEGIN_DECLS

#define GST_TYPE_HAILO_FILTER (gst_hailofilter_get_type())
#define GST_HAILO_FILTER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_FILTER, GstHailofilter))
#define GST_HAILO_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_FILTER, GstHailofilterClass))
#define GST_IS_HAILO_FILTER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_FILTER))
#define GST_IS_HAILO_FILTER_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_FILTER))

typedef struct _GstHailofilter GstHailofilter;
typedef struct _GstHailofilterClass GstHailofilterClass;

struct _GstHailofilter
{
    GstBaseTransform base_hailofilter;
    gchar *default_function_name;
    gchar *lib_path;
    gchar *config_path;
    gchar *function_name;
    void *loaded_lib;
    void * params;
    gboolean use_config;
    gboolean remove_tensors;

    void (*handler)(HailoROIPtr, void *);
    void (*handler_no_config)(HailoROIPtr);
    void (*handler_gst)(HailoROIPtr, GstVideoFrame *, void *);
    void (*handler_gst_no_config)(HailoROIPtr, GstVideoFrame *);
    gboolean use_gst_buffer;
};

struct _GstHailofilterClass
{
    GstBaseTransformClass base_hailofilter_class;
};

GType gst_hailofilter_get_type(void);

G_END_DECLS