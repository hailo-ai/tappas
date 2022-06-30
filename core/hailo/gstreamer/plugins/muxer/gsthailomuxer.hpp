/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/*
 * GStreamer Muxer element
 *
 *
 * gsthailomuxer.hpp: Simple Muxer (N->1) element, waits on all sinks, passes the first one, with all metadata included.
 */

#ifndef __GST_HAILO_MUXER_H__
#define __GST_HAILO_MUXER_H__

#include <gst/gst.h>
#include <gst/base/gstcollectpads.h>

G_BEGIN_DECLS

#define GST_TYPE_HAILO_MUXER \
    (gst_hailo_muxer_get_type())
#define GST_HAILO_MUXER(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_MUXER, GstHailoMuxer))
#define GST_HAILO_MUXER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_MUXER, GstHailoMuxerClass))
#define GST_IS_HAILO_MUXER(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_MUXER))
#define GST_IS_HAILO_MUXER_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_MUXER))
#define GST_HAILO_MUXER_CAST(obj) ((GstHailoMuxer *)(obj))

typedef struct _GstHailoMuxer GstHailoMuxer;
typedef struct _GstHailoMuxerClass GstHailoMuxerClass;

/**
 * GstHailoMuxer:
 *
 * Opaque #GstHailoMuxer data structure.
 */
struct _GstHailoMuxer
{
    GstElement element;
    /*< private >*/
    GstPad *srcpad;

    GstPad *main_sinkpad;
    GstPad *other_sinkpad;
    GstCollectPads *collect_pads;
    GstCollectData *collect_data1;
    GstCollectData *collect_data2;
};

struct _GstHailoMuxerClass
{
    GstElementClass parent_class;
};

G_GNUC_INTERNAL GType gst_hailo_muxer_get_type(void);

G_END_DECLS

#endif /* __GST_HAILO_MUXER_H__ */
