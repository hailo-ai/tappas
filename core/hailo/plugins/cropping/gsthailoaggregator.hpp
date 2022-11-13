/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/*
 * GStreamer aggregator element for cascading networks.
 *
 * gsthailoaggregator.hpp:
 */

#pragma once
#include <gst/gst.h>
#include <mutex>
#include <condition_variable>

#include "hailo_objects.hpp"

G_BEGIN_DECLS

#define GST_TYPE_HAILO_AGGREGATOR \
    (gst_hailoaggregator_get_type())
#define GST_HAILO_AGGREGATOR(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_AGGREGATOR, GstHailoAggregator))
#define GST_HAILO_AGGREGATOR_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_AGGREGATOR, GstHailoAggregatorClass))
#define GST_IS_HAILO_AGGREGATOR(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_AGGREGATOR))
#define GST_IS_HAILO_AGGREGATOR_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_AGGREGATOR))
#define GST_HAILO_AGGREGATOR_CAST(obj) ((GstHailoAggregator *)(obj))
#define GST_HAILO_AGGREGATOR_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_HAILO_AGGREGATOR, GstHailoAggregatorClass))

typedef struct _GstHailoAggregator GstHailoAggregator;
typedef struct _GstHailoAggregatorClass GstHailoAggregatorClass;

struct _GstHailoAggregator
{
    GstElement element;
    /*< private >*/
    GstPad *srcpad;

    GstPad *sinkpad_main;
    bool eos_main;
    GstPad *sinkpad_sub;
    bool eos_sub;
    GstBuffer *mainframe;
    uint num_of_frames;
    uint expected_frames;
    uint last_offset;
    gboolean flatten_detections;

    std::mutex mutex;
    std::condition_variable cv_main;
    std::condition_variable cv_sub;
};

struct _GstHailoAggregatorClass
{
    GstElementClass parent_class;

    void (*handle_main_roi_post_aggregation) (GstHailoAggregator *hailoaggregator, HailoROIPtr hailo_roi);
    void (*handle_sub_frame_roi) (GstHailoAggregator *hailoaggregator, HailoROIPtr sub_buffer_roi);
};

G_GNUC_INTERNAL GType gst_hailoaggregator_get_type(void);

G_END_DECLS