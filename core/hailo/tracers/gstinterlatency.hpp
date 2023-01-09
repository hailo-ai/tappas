/* GStreamer
 * Copyright (C) 2013 Stefan Sauer <ensonic@users.sf.net>
 * Copyright (C) 2016 RidgeRun Engineering <carlos.rodriguez@ridgerun.com> 
 *
 * gstinterlatency.h: tracing module that logs processing latencies
 * stats between source and intermediate elements
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

#include "gstsharktracer.hpp"

G_BEGIN_DECLS
#define GST_TYPE_INTERLATENCY_TRACER \
  (gst_interlatency_tracer_get_type())
#define GST_INTERLATENCY_TRACER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_INTERLATENCY_TRACER,GstInterLatencyTracer))
#define GST_INTERLATENCY_TRACER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_INTERLATENCY_TRACER,GstInterLatencyTracerClass))
#define GST_IS_INTERLATENCY_TRACER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_INTERLATENCY_TRACER))
#define GST_IS_INTERLATENCY_TRACER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_INTERLATENCY_TRACER))
#define GST_INTERLATENCY_TRACER_CAST(obj) ((GstInterLatencyTracer *)(obj))
typedef struct _GstInterLatencyTracer GstInterLatencyTracer;
typedef struct _GstInterLatencyTracerClass GstInterLatencyTracerClass;

/**
 * GstInterLatencyTracer:
 *
 * Opaque #GstInterLatencyTracer data structure
 */
struct _GstInterLatencyTracer
{
  GstSharkTracer parent;
  /*< private > */
};

struct _GstInterLatencyTracerClass
{
  GstSharkTracerClass parent_class;

  /* signals */
};

G_GNUC_INTERNAL GType gst_interlatency_tracer_get_type (void);

G_END_DECLS