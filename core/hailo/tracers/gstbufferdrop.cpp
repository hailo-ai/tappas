/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
/**
 * SECTION:gstbufferdrop
 * @short_description: log number of buffer drops at every fpsdisplaysink
 *
 * A tracing module that logs the number of buffer drops at every fpsdisplaysink element over time.
 */

#include "gstbufferdrop.hpp"
#include "gstctf.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_buffer_drop_debug);
#define GST_CAT_DEFAULT gst_buffer_drop_debug

struct _GstBufferDropTracer
{
  GstSharkTracer parent;
};

#define _do_init \
  GST_DEBUG_CATEGORY_INIT(gst_buffer_drop_debug, "bufferdrop", 0, "bufferdrop tracer");

G_DEFINE_TYPE_WITH_CODE(GstBufferDropTracer, gst_buffer_drop_tracer, GST_SHARK_TYPE_TRACER, _do_init);

static void do_buffer_drop(GstTracer *tracer, guint64 ts, GstPad *pad);
static void do_buffer_drop_list(GstTracer *tracer, guint64 ts, GstPad *pad,
                                GstBufferList *list);
static gboolean is_fpsdisplaysink(GstElement *element);

static GstTracerRecord *tr_qlevel;

static const gchar buffer_drop_metadata_event[] = "event {\n\
    name = bufferdrop;\n\
    id = %d;\n\
    stream_id = %d;\n\
    fields := struct {\n\
        string bufferdrop;\n\
        integer { size = 64; align = 8; signed = 0; encoding = none; base = 10; } max_size_time;\n\
    };\n\
};\n\
\n";

static GstElement *
get_parent_element(GstPad *pad)
{
  GstElement *element;
  GstObject *parent;
  GstObject *child = GST_OBJECT(pad);

  do
  {
    parent = GST_OBJECT_PARENT(child);

    if (GST_IS_ELEMENT(parent))
      break;

    child = parent;

  } while (GST_IS_OBJECT(child));

  element = gst_pad_get_parent_element(GST_PAD(child));

  return element;
}

static void
do_buffer_drop(GstTracer *self, guint64 ts, GstPad *pad)
{
  GstElement *element;
  guint64 frames_dropped;
  const gchar *element_name;
  gchar *max_size_time_string;

  element = get_parent_element(pad);

  if (!is_fpsdisplaysink(element))
  {
    goto out;
  }

  element_name = GST_OBJECT_NAME(element);

  g_object_get(element, "frames-dropped", &frames_dropped, NULL);

  max_size_time_string =
      g_strdup_printf("%" GST_TIME_FORMAT, GST_TIME_ARGS(frames_dropped));

  gst_tracer_record_log(tr_qlevel, element_name, frames_dropped);

  g_free(max_size_time_string);


out:
{
  gst_object_unref(element);
}
}

static void
do_buffer_drop_list(GstTracer *tracer, guint64 ts, GstPad *pad,
                    GstBufferList *list)
{
  guint idx;

  for (idx = 0; idx < gst_buffer_list_length(list); ++idx)
  {
    do_buffer_drop(tracer, ts, pad);
  }
}

static gboolean
is_fpsdisplaysink(GstElement *element)
{
  static GstElementFactory *qfactory = NULL;
  GstElementFactory *efactory;

  g_return_val_if_fail(element, FALSE);

  if (NULL == qfactory)
  {
    qfactory = gst_element_factory_find("fpsdisplaysink");
  }

  efactory = gst_element_get_factory(element);

  return efactory == qfactory;
}

/* tracer class */
static void
gst_buffer_drop_tracer_class_init(GstBufferDropTracerClass *klass)
{
  tr_qlevel = gst_tracer_record_new("bufferdrop.class", "name",
                                    GST_TYPE_STRUCTURE, gst_structure_new("scope", "type", G_TYPE_GTYPE, G_TYPE_STRING, "related-to", GST_TYPE_TRACER_VALUE_SCOPE, GST_TRACER_VALUE_SCOPE_ELEMENT, NULL), "buffers_drop",
                                    GST_TYPE_STRUCTURE, gst_structure_new("scope", "type", G_TYPE_GTYPE, G_TYPE_UINT, "related-to", GST_TYPE_TRACER_VALUE_SCOPE, GST_TRACER_VALUE_SCOPE_ELEMENT, NULL), NULL);
}

static void
gst_buffer_drop_tracer_init(GstBufferDropTracer *self)
{
  GstSharkTracer *tracer = GST_SHARK_TRACER(self);

  gst_shark_tracer_register_hook(tracer, "pad-push-pre",
                                 G_CALLBACK(do_buffer_drop));

  gst_shark_tracer_register_hook(tracer, "pad-push-list-pre",
                                 G_CALLBACK(do_buffer_drop_list));

  gst_shark_tracer_register_hook(tracer, "pad-pull-range-pre",
                                 G_CALLBACK(do_buffer_drop));
}
