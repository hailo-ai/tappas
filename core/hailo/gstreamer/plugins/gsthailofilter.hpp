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

#ifndef _GST_HAILO_FILTER_H_
#define _GST_HAILO_FILTER_H_

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <map>
#include "hailo_frame.hpp"

G_BEGIN_DECLS

#define GST_TYPE_HAILO_FILTER (gst_hailofilter_get_type())
#define GST_HAILO_FILTER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_FILTER, GstHailoFilter))
#define GST_HAILO_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_FILTER, GstHailoFilterClass))
#define GST_IS_HAILO_FILTER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_FILTER))
#define GST_IS_HAILO_FILTER_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_FILTER))

typedef struct _GstHailoFilter GstHailoFilter;
typedef struct _GstHailoFilterClass GstHailoFilterClass;

struct _GstHailoFilter
{
    GstVideoFilter base_hailofilter;
    gchar *default_function_name;
    gboolean debug;
    gchar *lib_path;
    std::vector<gchar *> function_name;
    void *loaded_lib;
    gchar *stream_id;
    std::vector<void (*)(HailoFramePtr hailo_frame)> handler;
};

struct _GstHailoFilterClass
{
    GstVideoFilterClass base_hailofilter_class;
};

GType gst_hailofilter_get_type(void);

G_END_DECLS

#endif
