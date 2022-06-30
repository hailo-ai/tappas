/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/* GStreamer
 * Copyright (C) 2021 FIXME <fixme@example.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#pragma once

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include "hailo_tracker.hpp"

G_BEGIN_DECLS

#define GST_TYPE_HAILO_TRACKER (gst_hailo_tracker_get_type())
#define GST_HAILO_TRACKER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_TRACKER, GstHailoTracker))
#define GST_HAILO_TRACKER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_TRACKER, GstHailoTrackerClass))
#define GST_IS_HAILO_TRACKER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_TRACKER))
#define GST_IS_HAILO_TRACKER_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_TRACKER))

typedef struct _GstHailoTracker GstHailoTracker;
typedef struct _GstHailoTrackerClass GstHailoTrackerClass;

struct _GstHailoTracker
{
    GstVideoFilter base_hailotracker;
    gboolean debug;
    gchar *current_stream_id;
    gint class_id;
    HailoTrackerParams tracker_params;
    std::vector<std::string> active_streams;
};

struct _GstHailoTrackerClass
{
    GstVideoFilterClass base_hailotracker_class;
};

GType gst_hailo_tracker_get_type(void);

G_END_DECLS
