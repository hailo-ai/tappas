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
#include "osd.hpp"
#include "dsp/gsthailodsp.h"

G_BEGIN_DECLS

#define GST_TYPE_HAILO_OSD (gst_hailoosd_get_type())
#define GST_HAILO_OSD(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_OSD, GstHailoOsd))
#define GST_HAILO_OSD_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_OSD, GstHailoOsdClass))
#define GST_IS_HAILO_OSD(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_OSD))
#define GST_IS_HAILO_OSD_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_OSD))

typedef struct _GstHailoOsd GstHailoOsd;
typedef struct _GstHailoOsdClass GstHailoOsdClass;

struct _GstHailoOsd
{
    GstBaseTransform base_hailoosd;
    gchar *config_path;
    OsdParams * params;
    gboolean wait_for_writable_buffer;
};

struct _GstHailoOsdClass
{
    GstBaseTransformClass parent_class;
};

GType gst_hailoosd_get_type(void);

G_END_DECLS