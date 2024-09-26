/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#pragma once
#include <stdint.h>
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_HAILO_NVALVE (gst_hailonvalve_get_type())
#define GST_HAILO_NVALVE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_NVALVE, GstHailoNValve))
#define GST_HAILO_NVALVE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_NVALVE, GstHailoNValveClass))
#define GST_IS_HAILO_NVALVE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_NVALVE))
#define GST_IS_HAILO_NVALVE_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_NVALVE))

typedef struct _GstHailoNValve GstHailoNValve;
typedef struct _GstHailoNValveClass GstHailoNValveClass;

struct _GstHailoNValve
{
    /*< private >*/
    GstElement parent;

    /* atomic boolean */
    volatile gint drop;

    /* Protected by the stream lock */
    gboolean discont;
    gboolean need_repush_sticky;

    GstPad *srcpad;
    GstPad *sinkpad;

    uint32_t count;
    uint32_t n_frames;
};

struct _GstHailoNValveClass
{
    GstElementClass parent_class;
};

GType gst_hailonvalve_get_type(void);

G_END_DECLS