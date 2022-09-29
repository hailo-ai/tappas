/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/*
 * GStreamer ROUND_ROBIN element
 *
 *
 * gsthailoroundrobin.hpp: Simple ROUND_ROBIN (N->1) element, waits on all sinks, passes the first one, with all metadata included.
 */

#pragma once

#include <gst/gst.h>
#include <vector>
#include <mutex>
#include <condition_variable>

G_BEGIN_DECLS

#define GST_TYPE_HAILO_ROUND_ROBIN \
    (gst_hailo_round_robin_get_type())
#define GST_HAILO_ROUND_ROBIN(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_ROUND_ROBIN, GstHailoRoundRobin))
#define GST_HAILO_ROUND_ROBIN_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_ROUND_ROBIN, GstHailoRoundRobinClass))
#define GST_IS_HAILO_ROUND_ROBIN(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_ROUND_ROBIN))
#define GST_IS_HAILO_ROUND_ROBIN_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_ROUND_ROBIN))
#define GST_HAILO_ROUND_ROBIN_CAST(obj) ((GstHailoRoundRobin *)(obj))

typedef struct _GstHailoRoundRobin GstHailoRoundRobin;
typedef struct _GstHailoRoundRobinClass GstHailoRoundRobinClass;

/**
 * GstHailoRoundRobin:
 *
 * Opaque #GstHailoRoundRobin data structure.
 */
struct _GstHailoRoundRobin
{
    GstElement element;
    /*< private >*/
    GstPad *srcpad;
    size_t current_pad_num;
    gboolean funnel_mode;
    std::vector<std::unique_ptr<std::mutex>> mutexes;
    std::vector<std::unique_ptr<std::condition_variable>> condition_vars;
};

struct _GstHailoRoundRobinClass
{
    GstElementClass parent_class;
};

G_GNUC_INTERNAL GType gst_hailo_round_robin_get_type(void);

G_END_DECLS
