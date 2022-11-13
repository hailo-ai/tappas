/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/*
 * GStreamer Muxer element
 *
 * gsthailomuxer.hpp: Simple Muxer (N->1) element, waits on all sinks, passes the first one, with all metadata included.
 */


#pragma once
#include <gst/gst.h>
#include <mutex>
#include <condition_variable>

#include "hailo_objects.hpp"

G_BEGIN_DECLS

#define GST_TYPE_HAILO_MUXER \
    (gst_hailomuxer_get_type())
#define GST_HAILO_MUXER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_MUXER, GstHailoMuxer))
#define GST_HAILO_MUXER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_MUXER, GstHailoMuxerClass))
#define GST_IS_HAILO_MUXER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_MUXER))
#define GST_IS_HAILO_MUXER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_MUXER))
#define GST_HAILO_MUXER_CAST(obj) ((GstHailoMuxer *)(obj))
#define GST_HAILO_MUXER_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_HAILO_MUXER, GstHailoMuxerClass))

typedef struct _GstHailoMuxer GstHailoMuxer;
typedef struct _GstHailoMuxerClass GstHailoMuxerClass;

struct _GstHailoMuxer
{
    GstElement element;
    /*< private >*/
    GstPad *srcpad;

    GstPad *sinkpad_main;
    bool eos_main;
    GstPad *sinkpad_sub;
    bool eos_sub;
    GstBuffer *mainframe;
    uint last_offset;
    gboolean sync_counters;
    uint current_counter_main;
    uint current_counter_sub;

    std::mutex mutex;
    std::condition_variable cv_main;
    std::condition_variable cv_sub;
};

struct _GstHailoMuxerClass
{
    GstElementClass parent_class;

    void (*handle_sub_frame_roi) (HailoROIPtr main_buffer_roi, HailoROIPtr sub_buffer_roi);
};

G_GNUC_INTERNAL GType gst_hailomuxer_get_type(void);

G_END_DECLS