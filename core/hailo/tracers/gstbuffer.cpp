/* GstShark - A Front End for GstTracer
 * Copyright (C) 2016-2018 RidgeRun Engineering <carlos.rodriguez@ridgerun.com>
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
 * SECTION:gstbuffer
 * @short_description: log current idendity
 *
 * A tracing module that prints buffer info at every sink pad
 */

#include "gstbuffer.hpp"
#include "gstctf.hpp"

GST_DEBUG_CATEGORY_STATIC (gst_buffer_debug);
#define GST_CAT_DEFAULT gst_buffer_debug

struct _GstBufferTracer
{
  GstSharkTracer parent;
};

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (gst_buffer_debug, "buffer", 0, "buffer tracer");

G_DEFINE_TYPE_WITH_CODE (GstBufferTracer, gst_buffer_tracer,
    GST_SHARK_TYPE_TRACER, _do_init);

static void gst_buffer_buffer_pre (GObject * self, GstClockTime ts,
    GstPad * pad, GstBuffer * buffer);
static void gst_buffer_buffer_list_pre (GObject * self, GstClockTime ts,
    GstPad * pad, GstBufferList * list);
static void gst_buffer_range_post (GObject * self, GstClockTime ts,
    GstPad * pad, GstBuffer * buffer, GstFlowReturn res);

static GstTracerRecord *tr_buffer;

static const gchar buffer_metadata_event[] = "event {\n\
    name = buffer;\n\
    id = %d;\n\
    stream_id = %d;\n\
    fields := struct {\n\
        string pad;\n\
        integer { size = 64; align = 8; signed = 0; encoding = none; base = 10; } pts;\n\
        integer { size = 64; align = 8; signed = 0; encoding = none; base = 10; } dts;\n\
        integer { size = 64; align = 8; signed = 0; encoding = none; base = 10; } duration;\n\
        integer { size = 64; align = 8; signed = 0; encoding = none; base = 10; } offset;\n\
        integer { size = 64; align = 8; signed = 0; encoding = none; base = 10; } offset_end;\n\
        integer { size = 64; align = 8; signed = 0; encoding = none; base = 10; } size;\n\
        integer { size = 32; align = 8; signed = 0; encoding = none; base = 10; } flags;\n\
        integer { size = 32; align = 8; signed = 0; encoding = none; base = 10; } refcount;\n\
    };\n\
};\n\
\n";

static void
gst_buffer_buffer_pre (GObject * self, GstClockTime ts, GstPad * pad,
    GstBuffer * buffer)
{
  gchar *pad_name;
  GstClockTime pts;
  gchar *spts;
  GstClockTime dts;
  gchar *sdts;
  GstClockTime duration;
  gchar *sduration;
  guint64 offset;
  guint64 offset_end;
  guint64 size;
  GstBufferFlags flags;
  GValue vflags = G_VALUE_INIT;
  gchar *sflags;
  guint refcount;

  pad_name = g_strdup_printf ("%s:%s", GST_DEBUG_PAD_NAME (pad));

    if (NULL == buffer) {
        return;
    }
  pts = GST_BUFFER_PTS (buffer);
  spts = g_strdup_printf ("%" GST_TIME_FORMAT, GST_TIME_ARGS (pts));

  dts = GST_BUFFER_DTS (buffer);
  sdts = g_strdup_printf ("%" GST_TIME_FORMAT, GST_TIME_ARGS (dts));

  duration = GST_BUFFER_DURATION (buffer);
  sduration = g_strdup_printf ("%" GST_TIME_FORMAT, GST_TIME_ARGS (duration));

  offset = GST_BUFFER_OFFSET (buffer);
  offset_end = GST_BUFFER_OFFSET_END (buffer);

  size = gst_buffer_get_size (buffer);

  flags = (GstBufferFlags)GST_BUFFER_FLAGS (buffer);
  g_value_init (&vflags, GST_TYPE_BUFFER_FLAGS);
  g_value_set_flags (&vflags, flags);
  sflags = gst_value_serialize (&vflags);

  refcount = GST_MINI_OBJECT_REFCOUNT_VALUE (buffer);

  gst_tracer_record_log (tr_buffer, pad_name, spts, sdts, sduration, offset,
      offset_end, size, sflags, refcount);

  do_print_buffer_event (BUFFER_EVENT_ID, pad_name, pts, dts, duration,
      offset, offset_end, size, flags, refcount);

  g_value_unset (&vflags);
  g_free (spts);
  g_free (sdts);
  g_free (sduration);
  g_free (sflags);
  g_free (pad_name);
}

static void
gst_buffer_range_post (GObject * self, GstClockTime ts, GstPad * pad,
    GstBuffer * buffer, GstFlowReturn res)
{
  gst_buffer_buffer_pre (self, ts, pad, buffer);
}

static void
gst_buffer_buffer_list_pre (GObject * self, GstClockTime ts, GstPad * pad,
    GstBufferList * list)
{
  guint idx;
  GstBuffer *buffer;

  for (idx = 0; idx < gst_buffer_list_length (list); ++idx) {
    buffer = gst_buffer_list_get (list, idx);
    gst_buffer_buffer_pre (self, ts, pad, buffer);
  }
}

/* tracer class */
static void
gst_buffer_tracer_class_init (GstBufferTracerClass * klass)
{
  gchar *metadata_event;

  tr_buffer = gst_tracer_record_new ("buffer.class",
      "pad", GST_TYPE_STRUCTURE, gst_structure_new ("value",
          "type", G_TYPE_GTYPE, G_TYPE_STRING,
          "description", G_TYPE_STRING,
          "The pad which the buffer is going through", NULL), "pts",
      GST_TYPE_STRUCTURE, gst_structure_new ("value", "type", G_TYPE_GTYPE,
          G_TYPE_STRING, "description", G_TYPE_STRING, "Presentation Timestamp",
          NULL), "dts", GST_TYPE_STRUCTURE, gst_structure_new ("value", "type",
          G_TYPE_GTYPE, G_TYPE_STRING, "description", G_TYPE_STRING,
          "Decoding Timestamp", NULL), "duration", GST_TYPE_STRUCTURE,
      gst_structure_new ("value", "type", G_TYPE_GTYPE, G_TYPE_STRING,
          "description", G_TYPE_STRING, "Duration", NULL), "offset",
      GST_TYPE_STRUCTURE, gst_structure_new ("value", "type", G_TYPE_GTYPE,
          G_TYPE_UINT64, "description", G_TYPE_STRING, "Offset", "min",
          G_TYPE_UINT64, G_GUINT64_CONSTANT (0), "max", G_TYPE_UINT64,
          G_MAXUINT64, NULL), "offset_end", GST_TYPE_STRUCTURE,
      gst_structure_new ("value", "type", G_TYPE_GTYPE, G_TYPE_UINT64,
          "description", G_TYPE_STRING, "Offset End", "min", G_TYPE_UINT64,
          G_GUINT64_CONSTANT (0), "max", G_TYPE_UINT64, G_MAXUINT64, NULL),
      "size", GST_TYPE_STRUCTURE, gst_structure_new ("value", "type",
          G_TYPE_GTYPE, G_TYPE_UINT64, "description", G_TYPE_STRING,
          "Data Size", "min", G_TYPE_UINT64, G_GUINT64_CONSTANT (0), "max",
          G_TYPE_UINT64, G_MAXUINT64, NULL), "flags", GST_TYPE_STRUCTURE,
      gst_structure_new ("value", "type", G_TYPE_GTYPE, G_TYPE_STRING,
          "description", G_TYPE_STRING, "Flags", NULL), "refcount",
      GST_TYPE_STRUCTURE, gst_structure_new ("value", "type", G_TYPE_GTYPE,
          G_TYPE_UINT, "description", G_TYPE_STRING, "Ref Count", "min",
          G_TYPE_UINT, 0, "max", G_TYPE_UINT, G_MAXUINT32, NULL), NULL);

  metadata_event = g_strdup_printf (buffer_metadata_event, BUFFER_EVENT_ID, 0);
  add_metadata_event_struct (metadata_event);
  g_free (metadata_event);
}

static void
gst_buffer_tracer_init (GstBufferTracer * self)
{
  GstSharkTracer *tracer = GST_SHARK_TRACER (self);

  gst_shark_tracer_register_hook (tracer, "pad-push-pre",
      G_CALLBACK (gst_buffer_buffer_pre));

  gst_shark_tracer_register_hook (tracer, "pad-push-list-pre",
      G_CALLBACK (gst_buffer_buffer_list_pre));

  gst_shark_tracer_register_hook (tracer, "pad-pull-range-post",
      G_CALLBACK (gst_buffer_range_post));
}
