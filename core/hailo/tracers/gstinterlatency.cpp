/* GStreamer
 * Copyright (C) 2013 Stefan Sauer <ensonic@users.sf.net>
 * Copyright (C) 2016 RidgeRun Engineering <carlos.rodriguez@ridgerun.com>
 *
 * gstinterlatency.c: tracing module that logs processing latencies
 * stats between source and intermediate elements
 *
 * This file is part of GstShark.
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
/**
 * SECTION:gstinterlatency
 * @short_description: log processing latencies stats
 *
 * A tracing module that determines latencies between src and intermediate elements
 * by injecting custom events at sources and process them in the pads.
 */
/* TODO(ensonic): if there are two sources feeding into a mixer/muxer and later
 * we fan-out with tee and have two sinks, each sink would get all two events,
 * the later event would overwrite the former. Unfortunately when the buffer
 * arrives on the sink we don't know to which event it correlates. Better would
 * be to use the buffer meta in 1.0 instead of the event. Or we track a min/max
 * latency.
 */


#include "gstinterlatency.hpp"
#include "gstctf.hpp"

GST_DEBUG_CATEGORY_STATIC (gst_interlatency_debug);
#define GST_CAT_DEFAULT gst_interlatency_debug

#define _do_init GST_DEBUG_CATEGORY_INIT (gst_interlatency_debug, "interlatency", 0, "interlatency tracer");
#define gst_interlatency_tracer_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstInterLatencyTracer, gst_interlatency_tracer,
    GST_SHARK_TYPE_TRACER, _do_init);

static GQuark latency_probe_id;
static GQuark latency_probe_pad;
static GQuark latency_probe_ts;

static GstTracerRecord *tr_interlatency;

static const gchar interlatency_metadata_event[] = "event {\n\
    name = interlatency;\n\
    id = %d;\n\
    stream_id = %d;\n\
    fields := struct {\n\
        string from_pad;\n\
        string to_pad;\n\
        integer { size = 64; align = 8; signed = 0; encoding = none; base = 10; } _time;\n\
    };\n\
};\n\
\n";

static void gst_interlatency_tracer_dispose (GObject * object);

/* data helpers */

/*
 * Get the element/bin owning the pad.
 *
 * in: a normal pad
 * out: the element
 *
 * in: a proxy pad
 * out: the element that contains the peer of the proxy
 *
 * in: a ghost pad
 * out: the bin owning the ghostpad
 */
/* TODO(ensonic): gst_pad_get_parent_element() would not work here, should we
 * add this as new api, e.g. gst_pad_find_parent_element();
 */
static GstElement *
get_real_pad_parent (GstPad * pad)
{
  GstObject *parent = NULL;

  if (!pad)
    return NULL;

  parent = GST_OBJECT_PARENT (pad);

  /* if parent of pad is a ghost-pad, then pad is a proxy_pad */
  if (parent && GST_IS_GHOST_PAD (parent)) {
    pad = GST_PAD_CAST (parent);
    parent = GST_OBJECT_PARENT (pad);
  }

  return GST_ELEMENT_CAST (parent);
}

/* hooks */

static void
log_latency (GstInterLatencyTracer * interlatency_tracer,
    const GstStructure * data, GstPad * sink_pad, guint64 sink_ts)
{
  GstPad *src_pad = NULL;
  guint64 src_ts;
  gchar *src = NULL, *sink = NULL;
  guint64 time;
  GString *time_string = NULL;

  gst_structure_id_get (data,
      latency_probe_pad, GST_TYPE_PAD, &src_pad,
      latency_probe_ts, G_TYPE_UINT64, &src_ts, NULL);

  src = g_strdup_printf ("%s_%s", GST_DEBUG_PAD_NAME (src_pad));
  sink = g_strdup_printf ("%s_%s", GST_DEBUG_PAD_NAME (sink_pad));

  time = GST_CLOCK_DIFF (src_ts, sink_ts);

  time_string = g_string_new ("");
  g_string_printf (time_string, "%" GST_TIME_FORMAT, GST_TIME_ARGS (time));

  gst_tracer_record_log (tr_interlatency, src, sink, time_string->str);

  do_print_interlatency_event (INTERLATENCY_EVENT_ID, src, sink, time);

  g_string_free (time_string, TRUE);
  g_free (src);
  g_free (sink);
}

static void
send_latency_probe (GstElement * parent, GstPad * pad, guint64 ts)
{
  g_return_if_fail (parent);
  g_return_if_fail (pad);

  if (!GST_IS_BIN (parent)) {
    GstEvent *latency_probe = gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM,
        gst_structure_new_id (latency_probe_id,
            latency_probe_pad, GST_TYPE_PAD, pad,
            latency_probe_ts, G_TYPE_UINT64, ts,
            NULL));
    gst_pad_push_event (pad, latency_probe);
  }
}

static void
calculate_latency (GstInterLatencyTracer * interlatency_tracer,
    GstElement * parent, GstPad * pad, guint64 ts)
{
  g_return_if_fail (interlatency_tracer);
  g_return_if_fail (parent);
  g_return_if_fail (pad);

  if (!GST_IS_BIN (parent)) {
    GstEvent *ev = (GstEvent*)g_object_get_qdata ((GObject *) pad, latency_probe_id);

    if (GST_IS_EVENT (ev))
      log_latency (interlatency_tracer, gst_event_get_structure (ev), pad, ts);
  }
}

static void
do_push_buffer_pre (GstTracer * self, guint64 ts, GstPad * pad)
{
  GstElement *parent = get_real_pad_parent (pad);
  GstPad *peer_pad = GST_PAD_PEER (pad);
  GstElement *peer_parent = get_real_pad_parent (peer_pad);
  GstInterLatencyTracer *interlatency_tracer;

  interlatency_tracer = GST_INTERLATENCY_TRACER_CAST (self);

  /* Not having a peer pad means that the pad is not linked, which
     results in a segfault */
  if (!peer_pad) {
    return;
  }

  if (parent && GST_OBJECT_FLAG_IS_SET (parent, GST_ELEMENT_FLAG_SOURCE)) {
    send_latency_probe (parent, pad, ts);
    calculate_latency (interlatency_tracer, peer_parent, peer_pad, ts);
  } else
    calculate_latency (interlatency_tracer, parent, pad, ts);

  if (peer_parent
      && GST_OBJECT_FLAG_IS_SET (peer_parent, GST_ELEMENT_FLAG_SINK))
    calculate_latency (interlatency_tracer, peer_parent, peer_pad, ts);

}

static void
do_pull_range_pre (GstTracer * self, guint64 ts, GstPad * pad)
{
  GstInterLatencyTracer *interlatency_tracer;

  GstPad *peer_pad = GST_PAD_PEER (pad);
  GstElement *parent_peer = get_real_pad_parent (peer_pad);

  interlatency_tracer = GST_INTERLATENCY_TRACER_CAST (self);

  if (parent_peer
      && GST_OBJECT_FLAG_IS_SET (parent_peer, GST_ELEMENT_FLAG_SOURCE))
    send_latency_probe (parent_peer, peer_pad, ts);
  else
    calculate_latency (interlatency_tracer, parent_peer, peer_pad, ts);
}

static void
do_pull_range_post (GstTracer * self, guint64 ts, GstPad * pad)
{
  GstInterLatencyTracer *interlatency_tracer;
  GstElement *parent = get_real_pad_parent (pad);

  interlatency_tracer = GST_INTERLATENCY_TRACER_CAST (self);

  if (parent && GST_OBJECT_FLAG_IS_SET (parent, GST_ELEMENT_FLAG_SINK))
    calculate_latency (interlatency_tracer, parent, pad, ts);
}

static void
do_push_event_pre (GstTracer * self, guint64 ts, GstPad * pad, GstEvent * ev)
{
  GstPad *peer_pad = GST_PAD_PEER (pad);
  GstElement *parent = get_real_pad_parent (pad);
  GstElement *parent_peer = get_real_pad_parent (peer_pad);

  if (parent_peer && (!GST_IS_BIN (parent_peer)) &&
      !GST_OBJECT_FLAG_IS_SET (parent, GST_ELEMENT_FLAG_SOURCE)) {
    if (GST_EVENT_TYPE (ev) == GST_EVENT_CUSTOM_DOWNSTREAM) {
      const GstStructure *data = gst_event_get_structure (ev);

      if (gst_structure_get_name_id (data) == latency_probe_id) {
        /* store event and calculate latency when the buffer that follows
         * has been processed */
        g_object_set_qdata_full ((GObject *) pad, latency_probe_id,
            gst_event_ref (ev), (GDestroyNotify) gst_event_unref);
        if (GST_OBJECT_FLAG_IS_SET (parent_peer, GST_ELEMENT_FLAG_SINK))
          g_object_set_qdata_full ((GObject *) peer_pad, latency_probe_id,
              gst_event_ref (ev), (GDestroyNotify) gst_event_unref);
      }
    }
  }
}

/* tracer class */

static void
gst_interlatency_tracer_class_init (GstInterLatencyTracerClass * klass)
{
  GObjectClass *oclass;
  gchar *metadata_event;

  oclass = G_OBJECT_CLASS (klass);

  latency_probe_id = g_quark_from_static_string ("latency_probe.id");
  latency_probe_pad = g_quark_from_static_string ("latency_probe.pad");
  latency_probe_ts = g_quark_from_static_string ("latency_probe.ts");

  /* announce trace formats */
  tr_interlatency = gst_tracer_record_new ("interlatency.class",
      "from_pad", GST_TYPE_STRUCTURE, gst_structure_new ("scope",
          "type", G_TYPE_GTYPE, G_TYPE_STRING,
          "related-to", GST_TYPE_TRACER_VALUE_SCOPE, GST_TRACER_VALUE_SCOPE_PAD,
          NULL),
      "to_pad", GST_TYPE_STRUCTURE, gst_structure_new ("scope",
          "type", G_TYPE_GTYPE, G_TYPE_STRING,
          "related-to", GST_TYPE_TRACER_VALUE_SCOPE, GST_TRACER_VALUE_SCOPE_PAD,
          NULL),
      "time", GST_TYPE_STRUCTURE, gst_structure_new ("scope",
          "type", G_TYPE_GTYPE, G_TYPE_STRING,
          "related-to", GST_TYPE_TRACER_VALUE_SCOPE, GST_TRACER_VALUE_SCOPE_PROCESS,
          NULL),
      NULL);


  oclass->dispose = gst_interlatency_tracer_dispose;

  metadata_event =
      g_strdup_printf (interlatency_metadata_event, INTERLATENCY_EVENT_ID, 0);
  add_metadata_event_struct (metadata_event);
  g_free (metadata_event);
}

static void
gst_interlatency_tracer_init (GstInterLatencyTracer * self)
{
  GstTracer *tracer = GST_TRACER (self);

  /* In push mode, pre/post will be called before/after the peer chain
   * function has been called. For this reason, we only use -pre to avoid
   * accounting for the processing time of the peer element (the sink).
   */
  gst_tracing_register_hook (tracer, "pad-push-pre",
      G_CALLBACK (do_push_buffer_pre));
  gst_tracing_register_hook (tracer, "pad-push-list-pre",
      G_CALLBACK (do_push_buffer_pre));

  /* While in pull mode, pre/post will happend before and after the upstream
   * pull_range call is made, so it already only account for the upstream
   * processing time. As a side effect, in pull mode, we can measure the
   * source processing latency, while in push mode, we can't .
   */
  gst_tracing_register_hook (tracer, "pad-pull-range-pre",
      G_CALLBACK (do_pull_range_pre));
  gst_tracing_register_hook (tracer, "pad-pull-range-post",
      G_CALLBACK (do_pull_range_post));
  gst_tracing_register_hook (tracer, "pad-push-event-pre",
      G_CALLBACK (do_push_event_pre));
}

static void
gst_interlatency_tracer_dispose (GObject * object)
{
}
