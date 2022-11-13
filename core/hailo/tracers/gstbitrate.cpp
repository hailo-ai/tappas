/* GstShark - A Front End for GstTracer
 * Copyright (C) 2016-2018 RidgeRun Engineering <sebastian.fatjo@ridgerun.com>
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
 * SECTION: gstframerate
 * @short_description: shows the framerate on every pad in the pipeline.
 *
 * A tracing module that displays the amount of frames per second on every
 * SRC or SINK pad of every element of the running pipeline, depending on 
 * the scheduling mode.
 */

#include "gstbitrate.hpp"
#include "gstctf.hpp"

GST_DEBUG_CATEGORY_STATIC (gst_bitrate_debug);
#define GST_CAT_DEFAULT gst_bitrate_debug

struct _GstBitrateTracer
{
  GstPeriodicTracer parent;

  GHashTable *bitrate_counters;
};

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (gst_bitrate_debug, "bitrate", 0, "bitrate tracer");

#define gst_bitrate_tracer_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstBitrateTracer, gst_bitrate_tracer,
    GST_TYPE_PERIODIC_TRACER, _do_init);

static GstTracerRecord *tr_bitrate;

static gchar *make_char_array_valid (gchar * src);
static void create_metadata_event (GstPeriodicTracer * tracer);
static void add_bytes (GstBitrateTracer * self, GstClockTime ts, GstPad * pad,
    guint64 bytes);
static gboolean do_print_bitrate (GstPeriodicTracer * tracer);
static void reset_counters (GstPeriodicTracer * tracer);

typedef struct _GstBitrateHash GstBitrateHash;

struct _GstBitrateHash
{
  gchar *fullname;
  guint64 bitrate;
};

static const gchar bitrate_metadata_event[] = "event {\n\
    name = bitrate;\n\
    id = %d;\n\
    stream_id = %d;\n\
    fields := struct {\n\
        string pad;\n\
        integer { size = 64; align = 8; signed = 0; encoding = none; base = 10; } _bps;\n\
    };\n\
};\n\
\n";

static gboolean
do_print_bitrate (GstPeriodicTracer * tracer)
{
  GstBitrateTracer *self;
  GHashTableIter iter;
  gpointer key, value;
  GstBitrateHash *pad_table;

  self = GST_BITRATE_TRACER (tracer);

  /* Using the iterator functions to go through the Hash table and print the bitrate
     of every element stored */
  g_hash_table_iter_init (&iter, self->bitrate_counters);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    pad_table = (GstBitrateHash *) value;

    gst_tracer_record_log (tr_bitrate, pad_table->fullname, pad_table->bitrate);
    do_print_bitrate_event (BITRATE_EVENT_ID, pad_table->fullname,
        pad_table->bitrate);

    pad_table->bitrate = 0;
  }

  return TRUE;
}

static void
do_destroy_hashtable_value (gpointer data)
{
  GstBitrateHash *value;

  /* In order to properly free the memory of a element of the Hash table
     it is needed to free the memory of the structure in every value */
  value = (GstBitrateHash *) data;

  g_free (value->fullname);
  g_free (value);
}

static void
do_destroy_hashtable_key (gpointer data)
{
  gst_object_unref (data);
}

static void
add_bytes (GstBitrateTracer * self, GstClockTime ts, GstPad * pad,
    guint64 bytes)
{
  gchar *fullname;
  GstBitrateHash *pad_frames;

  pad_frames = (GstBitrateHash*)g_hash_table_lookup (self->bitrate_counters, pad);

  if (NULL == pad_frames) {
    /* The full name of every pad has the format elementName.padName and it is going 
       to be used for displaying the bitrate in a friendly user way */
    fullname = g_strdup_printf ("%s_%s", GST_DEBUG_PAD_NAME (pad));
    fullname = make_char_array_valid (fullname);

    GST_INFO_OBJECT (self, "The %s key was added to the Hash Table", fullname);

    pad_frames = (GstBitrateHash*)g_malloc0 (sizeof (GstBitrateHash));
    pad_frames->fullname = fullname;
    g_hash_table_insert (self->bitrate_counters, gst_object_ref (pad),
        (gpointer) pad_frames);
  }

  pad_frames->bitrate += bytes * 8;
}

static void
reset_counters (GstPeriodicTracer * tracer)
{
  GstBitrateTracer *self;
  GstBitrateHash *pad_table;
  GHashTableIter iter;
  gpointer key, value;

  self = GST_BITRATE_TRACER (tracer);

  g_hash_table_iter_init (&iter, self->bitrate_counters);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    pad_table = (GstBitrateHash *) value;
    pad_table->bitrate = 0;
  }
}

static void
do_pad_push_buffer_pre (GstBitrateTracer * self, guint64 ts, GstPad * pad,
    GstBuffer * buffer)
{
  gsize bytes;

  bytes = gst_buffer_get_size (buffer);
  add_bytes (self, ts, pad, bytes);
}

static void
do_pad_push_list_pre (GstBitrateTracer * self, GstClockTime ts, GstPad * pad,
    GstBufferList * list)
{
  guint idx;
  GstBuffer *buffer;

  for (idx = 0; idx < gst_buffer_list_length (list); ++idx) {
    buffer = gst_buffer_list_get (list, idx);
    do_pad_push_buffer_pre (self, ts, pad, buffer);
  }
}

static void
do_pad_pull_range_pre (GstBitrateTracer * self, GstClockTime ts, GstPad * pad,
    guint64 offset, guint size)
{
  add_bytes (self, ts, pad, size);
}

/* tracer class */

static void
gst_bitrate_tracer_finalize (GObject * obj)
{
  GstBitrateTracer *self = GST_BITRATE_TRACER (obj);

  g_hash_table_destroy (self->bitrate_counters);

  G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
gst_bitrate_tracer_class_init (GstBitrateTracerClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstPeriodicTracerClass *ptracer_class = GST_PERIODIC_TRACER_CLASS (klass);

  gobject_class->finalize = gst_bitrate_tracer_finalize;

  ptracer_class->timer_callback = GST_DEBUG_FUNCPTR (do_print_bitrate);
  ptracer_class->reset = GST_DEBUG_FUNCPTR (reset_counters);
  ptracer_class->write_header = GST_DEBUG_FUNCPTR (create_metadata_event);

  tr_bitrate = gst_tracer_record_new ("bitrate.class",
      "pad", GST_TYPE_STRUCTURE, gst_structure_new ("scope",
          "type", G_TYPE_GTYPE, G_TYPE_STRING,
          "related-to", GST_TYPE_TRACER_VALUE_SCOPE, GST_TRACER_VALUE_SCOPE_PAD,
          NULL),
      "bitrate", GST_TYPE_STRUCTURE, gst_structure_new ("value",
          "type", G_TYPE_GTYPE, G_TYPE_UINT64,
          "description", G_TYPE_STRING, "Bitrate",
          "flags", GST_TYPE_TRACER_VALUE_FLAGS,
          GST_TRACER_VALUE_FLAGS_AGGREGATED, "min", G_TYPE_UINT64,
          G_GUINT64_CONSTANT (0), "max", G_TYPE_UINT64, G_MAXUINT64, NULL),
      NULL);
}

static void
gst_bitrate_tracer_init (GstBitrateTracer * self)
{
  GstSharkTracer *tracer = GST_SHARK_TRACER (self);

  self->bitrate_counters =
      g_hash_table_new_full (g_direct_hash, g_direct_equal,
      do_destroy_hashtable_key, do_destroy_hashtable_value);

  gst_shark_tracer_register_hook (tracer, "pad-push-pre",
      G_CALLBACK (do_pad_push_buffer_pre));
  gst_shark_tracer_register_hook (tracer, "pad-push-list-pre",
      G_CALLBACK (do_pad_push_list_pre));
  gst_shark_tracer_register_hook (tracer, "pad-pull-range-pre",
      G_CALLBACK (do_pad_pull_range_pre));
}

static void
create_metadata_event (GstPeriodicTracer * tracer)
{
  gchar *metadata_event;

  /* Add event in metadata file */
  metadata_event =
      g_strdup_printf (bitrate_metadata_event, BITRATE_EVENT_ID, 0);
  add_metadata_event_struct (metadata_event);
  g_free (metadata_event);
}

static gchar *
make_char_array_valid (gchar * src)
{
  gchar *c;

  g_return_val_if_fail (src, NULL);

  for (c = src; '\0' != *c; c++) {
    if ('-' == *c) {
      *c = '_';
    }
  }

  return src;
}
