/* GstShark - A Front End for GstTracer
 * Copyright (C) 2016 RidgeRun Engineering <manuel.leiva@ridgerun.com>
 *
 * This file is part of GstShark.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/**
 * SECTION:gstproctime
 * @short_description: log cpu usage stats
 *
 * A tracing module that take proctime() snapshots and logs them.
 */

#include "gstproctimecompute.hpp"
#include "gstproctime.hpp"
#include "gstctf.hpp"

GST_DEBUG_CATEGORY_STATIC (gst_proc_time_debug);
#define GST_CAT_DEFAULT gst_proc_time_debug

/**
 * GstProcTimeTracer:
 *
 * Opaque #GstProcTimeTracer data structure
 */
struct _GstProcTimeTracer
{
  GstSharkTracer parent;

  GstProcTime *proc_time;
};

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (gst_proc_time_debug, "proctime", 0, "proctime tracer");

G_DEFINE_TYPE_WITH_CODE (GstProcTimeTracer, gst_proc_time_tracer,
    GST_SHARK_TYPE_TRACER, _do_init);

static GstTracerRecord *tr_proc_time;

static const gchar proc_time_metadata_event[] = "event {\n\
    name = proctime;\n\
    id = %d;\n\
    stream_id = %d;\n\
    fields := struct {\n\
        string element; \n\
        integer { size = 64; align = 8; signed = 0; encoding = none; base = 10; } _time;\n\
    };\n\
};\n\
\n";

static void
do_push_buffer_pre (GstTracer * self, guint64 ts, GstPad * pad)
{
  GstProcTimeTracer *proc_time_tracer;
  GstSharkTracer *shark_tracer;
  GstProcTime *proc_time;

  GstPad *pad_peer;
  gchar *name;
  GstClockTime time;
  gchar *time_string;
  gboolean should_log;
  gboolean should_calculate;

  proc_time_tracer = GST_PROC_TIME_TRACER (self);
  shark_tracer = GST_SHARK_TRACER (proc_time_tracer);
  proc_time = proc_time_tracer->proc_time;
  name = GST_OBJECT_NAME (GST_OBJECT_PARENT (pad));

  pad_peer = gst_pad_get_peer (pad);
  if (!pad_peer) {
    return;
  }

  should_calculate = gst_shark_tracer_element_is_filtered (shark_tracer, name);
  should_log =
      gst_proctime_proc_time (proc_time, &time, pad_peer, pad, ts,
      should_calculate);

  if (should_log) {
    time_string = g_strdup_printf ("%" GST_TIME_FORMAT, GST_TIME_ARGS (time));

    gst_tracer_record_log (tr_proc_time, name, time_string);

    do_print_proctime_event (PROCTIME_EVENT_ID, name, time);

    g_free (time_string);
  }

  gst_object_unref (pad_peer);
}

static void
do_element_new (GObject * self, GstClockTime ts, GstElement * element)
{
  GstProcTimeTracer *proc_time_tracer;
  GstProcTime *proc_time;

  proc_time_tracer = GST_PROC_TIME_TRACER (self);
  proc_time = proc_time_tracer->proc_time;

  gst_proctime_add_new_element (proc_time, element);
}

/* tracer class */

static void
gst_proc_time_tracer_finalize (GObject * obj)
{
  GstProcTimeTracer *self;

  self = GST_PROC_TIME_TRACER (obj);

  gst_proctime_free (self->proc_time);
  self->proc_time = NULL;

  G_OBJECT_CLASS (gst_proc_time_tracer_parent_class)->finalize (obj);
}

static void
gst_proc_time_tracer_class_init (GstProcTimeTracerClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  gchar *metadata_event;

  gobject_class->finalize = gst_proc_time_tracer_finalize;

  tr_proc_time = gst_tracer_record_new ("proctime.class",
      "element", GST_TYPE_STRUCTURE, gst_structure_new ("scope",
          "type", G_TYPE_GTYPE, G_TYPE_STRING,
          "related-to", GST_TYPE_TRACER_VALUE_SCOPE,
          GST_TRACER_VALUE_SCOPE_ELEMENT, NULL), "time", GST_TYPE_STRUCTURE,
      gst_structure_new ("scope", "type", G_TYPE_GTYPE, G_TYPE_STRING,
          "related-to", GST_TYPE_TRACER_VALUE_SCOPE,
          GST_TRACER_VALUE_SCOPE_PROCESS, NULL), NULL);

  metadata_event =
      g_strdup_printf (proc_time_metadata_event, PROCTIME_EVENT_ID, 0);
  add_metadata_event_struct (metadata_event);
  g_free (metadata_event);
}


static void
gst_proc_time_tracer_init (GstProcTimeTracer * self)
{
  GstTracer *tracer = GST_TRACER (self);


  self->proc_time = gst_proctime_new ();

  gst_tracing_register_hook (tracer, "pad-push-pre",
      G_CALLBACK (do_push_buffer_pre));

  gst_tracing_register_hook (tracer, "element-new",
      G_CALLBACK (do_element_new));
}
