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

#include "gstframerate.hpp"
#include "gstctf.hpp"

GST_DEBUG_CATEGORY_STATIC (gst_framerate_debug);
#define GST_CAT_DEFAULT gst_framerate_debug

#define FRAMERATE_INTERVAL 1

struct _GstFramerateTracer
{
  GstPeriodicTracer parent;

  GHashTable *frame_counters;
  guint callback_id;
  guint pipes_running;
};

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (gst_framerate_debug, "framerate", 0, "framerate tracer");

G_DEFINE_TYPE_WITH_CODE (GstFramerateTracer, gst_framerate_tracer,
    GST_TYPE_PERIODIC_TRACER, _do_init);

static GstTracerRecord *tr_framerate;

static gchar *make_char_array_valid (gchar * src);
static void create_metadata_event (GstPeriodicTracer * tracer);
static gboolean print_framerate (GstPeriodicTracer * tracer);
static void reset_counters (GstPeriodicTracer * tracer);
static void consider_frames (GstFramerateTracer * self, GstPad * pad,
    guint amount);
static void destroy_hashtable_value (gpointer data);
static void pad_push_buffer_pre (GstFramerateTracer * self, guint64 ts,
    GstPad * pad, GstBuffer * buffer);
static void pad_push_list_pre (GstFramerateTracer * self, GstClockTime ts,
    GstPad * pad, GstBufferList * list);
static void pad_pull_range_pre (GstFramerateTracer * self, GstClockTime ts,
    GstPad * pad, guint64 offset, guint size);
static void gst_framerate_tracer_finalize (GObject * obj);

typedef struct _GstFramerateHash GstFramerateHash;

struct _GstFramerateHash
{
  gchar *fullname;
  guint counter;
};

static const gchar framerate_metadata_event[] = "event {\n\
    name = framerate;\n\
    id = %d;\n\
    stream_id = %d;\n\
    fields := struct {\n\
        string pad;\n\
        integer { size = 64; align = 8; signed = 0; encoding = none; base = 10; } _fps;\n\
    };\n\
};\n\
\n";

static void
gst_framerate_tracer_class_init (GstFramerateTracerClass * klass)
{
  GstPeriodicTracerClass *ptracer_class = GST_PERIODIC_TRACER_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  ptracer_class->reset = GST_DEBUG_FUNCPTR (reset_counters);
  ptracer_class->timer_callback = GST_DEBUG_FUNCPTR (print_framerate);
  ptracer_class->write_header = GST_DEBUG_FUNCPTR (create_metadata_event);

  gobject_class->finalize = gst_framerate_tracer_finalize;

  tr_framerate = gst_tracer_record_new ("framerate.class",
      "pad", GST_TYPE_STRUCTURE, gst_structure_new ("scope",
          "type", G_TYPE_GTYPE, G_TYPE_STRING,
          "related-to", GST_TYPE_TRACER_VALUE_SCOPE, GST_TRACER_VALUE_SCOPE_PAD,
          NULL),
      "fps", GST_TYPE_STRUCTURE, gst_structure_new ("value",
          "type", G_TYPE_GTYPE, G_TYPE_UINT,
          "description", G_TYPE_STRING, "Frames per second",
          "flags", GST_TYPE_TRACER_VALUE_FLAGS,
          GST_TRACER_VALUE_FLAGS_AGGREGATED, "min", G_TYPE_UINT, 0, "max",
          G_TYPE_UINT, G_MAXUINT, NULL), NULL);
}

static void
gst_framerate_tracer_init (GstFramerateTracer * self)
{
  GstSharkTracer *stracer = GST_SHARK_TRACER (self);

  self->frame_counters =
      g_hash_table_new_full (g_direct_hash, g_direct_equal,
      gst_object_unref, destroy_hashtable_value);

  gst_shark_tracer_register_hook (stracer, "pad-push-pre",
      G_CALLBACK (pad_push_buffer_pre));
  gst_shark_tracer_register_hook (stracer, "pad-push-list-pre",
      G_CALLBACK (pad_push_list_pre));
  gst_shark_tracer_register_hook (stracer, "pad-pull-range-pre",
      G_CALLBACK (pad_pull_range_pre));
}

static void
reset_counters (GstPeriodicTracer * tracer)
{
  GstFramerateTracer *self;
  GHashTableIter iter;
  gpointer key, value;
  GstFramerateHash *pad_table;

  g_return_if_fail (tracer);

  self = GST_FRAMERATE_TRACER (tracer);

  GST_OBJECT_LOCK (self);
  g_hash_table_iter_init (&iter, self->frame_counters);

  while (g_hash_table_iter_next (&iter, &key, &value)) {
    pad_table = (GstFramerateHash *) value;
    pad_table->counter = 0;
  }
  GST_OBJECT_UNLOCK (self);
}

static void
create_metadata_event (GstPeriodicTracer * tracer)
{
  gchar *metadata_event;

  metadata_event = g_strdup_printf (framerate_metadata_event, FPS_EVENT_ID, 0);

  /* Add event in metadata file */
  add_metadata_event_struct (metadata_event);
  g_free (metadata_event);
}

static gboolean
print_framerate (GstPeriodicTracer * tracer)
{
  GstFramerateTracer *self;
  GHashTableIter iter;
  gpointer key, value;
  GstFramerateHash *pad_table;

  self = GST_FRAMERATE_TRACER (tracer);

  /* Lock the tracer to make sure no new pad is added while we are logging */
  GST_OBJECT_LOCK (self);

  /* Using the iterator functions to go through the Hash table and print the framerate 
     of every element stored */
  g_hash_table_iter_init (&iter, self->frame_counters);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    pad_table = (GstFramerateHash *) value;

    gst_tracer_record_log (tr_framerate, pad_table->fullname,
        pad_table->counter);
    do_print_framerate_event (FPS_EVENT_ID, pad_table->fullname,
        pad_table->counter);
    pad_table->counter = 0;
  }

  GST_OBJECT_UNLOCK (self);

  return TRUE;
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

static void
consider_frames (GstFramerateTracer * self, GstPad * pad, guint amount)
{
  GstFramerateHash *pad_frames;
  gchar *fullname;

  g_return_if_fail (self);
  g_return_if_fail (pad);

  pad_frames =
      (GstFramerateHash *) g_hash_table_lookup (self->frame_counters, pad);

  if (NULL != pad_frames) {
    pad_frames->counter += amount;
  } else {

    /* The full name of every pad has the format elementName_padName and it is going 
       to be used for displaying the framerate in a friendly user way */
    fullname = g_strdup_printf ("%s_%s", GST_DEBUG_PAD_NAME (pad));
    fullname = make_char_array_valid (fullname);

    pad_frames = (GstFramerateHash*)g_malloc (sizeof (GstFramerateHash));
    pad_frames->fullname = fullname;
    pad_frames->counter = amount;

    GST_OBJECT_LOCK (self);
    g_hash_table_insert (self->frame_counters, gst_object_ref (pad),
        pad_frames);
    GST_OBJECT_UNLOCK (self);

    GST_INFO_OBJECT (self, "The %s key was added to the Hash Table", fullname);
  }
}

static void
pad_push_buffer_pre (GstFramerateTracer * self, guint64 ts, GstPad * pad,
    GstBuffer * buffer)
{
  consider_frames (self, pad, 1);
}

static void
pad_push_list_pre (GstFramerateTracer * self, GstClockTime ts, GstPad * pad,
    GstBufferList * list)
{
  consider_frames (self, pad, gst_buffer_list_length (list));
}

static void
pad_pull_range_pre (GstFramerateTracer * self, GstClockTime ts, GstPad * pad,
    guint64 offset, guint size)
{
  consider_frames (self, pad, 1);
}

static void
destroy_hashtable_value (gpointer data)
{
  GstFramerateHash *value;

  /* In order to properly free the memory of a element of the Hash table
     it is needed to free the memory of the structure in every value */
  value = (GstFramerateHash *) data;

  g_free (value->fullname);
  g_free (value);
}

static void
gst_framerate_tracer_finalize (GObject * obj)
{
  GstFramerateTracer *self = GST_FRAMERATE_TRACER (obj);

  g_hash_table_destroy (self->frame_counters);

  G_OBJECT_CLASS (gst_framerate_tracer_parent_class)->finalize (obj);
}
