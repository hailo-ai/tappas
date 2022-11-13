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

#include "gstsharktracer.hpp"

GST_DEBUG_CATEGORY_STATIC (gst_shark_debug);
#define GST_CAT_DEFAULT gst_shark_debug

#define GST_SHARK_TRACER_PRIVATE(o) \
  (GstSharkTracerPrivate*)gst_shark_tracer_get_instance_private(GST_SHARK_TRACER(o))

typedef struct _GstSharkTracerPrivate GstSharkTracerPrivate;
struct _GstSharkTracerPrivate
{
  GHashTable *params;
  GHashTable *hooks;
  GHashTable *myhooks;
};

static void gst_shark_tracer_constructed (GObject * object);
static void gst_shark_tracer_finalize (GObject * object);
static void gst_shark_tracer_save_params (GstSharkTracer * self);
static void gst_shark_tracer_dump_params (GstSharkTracer * self);
static void gst_shark_tracer_free_params (GstSharkTracerPrivate * priv);
static void gst_shark_tracer_fill_hooks (GstSharkTracerPrivate * priv);

/* Our hooks */
static void gst_shark_tracer_hook_pad_push_pre (GObject * self, GstClockTime ts,
    GstPad * pad, GstBuffer * buffer);
static void gst_shark_tracer_hook_pad_push_post (GObject * self,
    GstClockTime ts, GstPad * pad, GstFlowReturn res);
static void gst_shark_tracer_hook_pad_push_list_pre (GObject * self,
    GstClockTime ts, GstPad * pad, GstBufferList * list);
static void gst_shark_tracer_hook_pad_push_list_post (GObject * self,
    GstClockTime ts, GstPad * pad, GstFlowReturn res);
static void gst_shark_tracer_hook_pad_pull_range_pre (GObject * self,
    GstClockTime ts, GstPad * pad, guint64 offset, guint size);
static void gst_shark_tracer_hook_pad_pull_range_post (GObject * self,
    GstClockTime ts, GstPad * pad, GstBuffer * buffer, GstFlowReturn res);
static void gst_shark_tracer_hook_pad_push_event_pre (GObject * self,
    GstClockTime ts, GstPad * pad, GstEvent * event);
static void gst_shark_tracer_hook_pad_push_event_post (GObject * self,
    GstClockTime ts, GstPad * pad, gboolean res);
static void gst_shark_tracer_hook_pad_query_pre (GObject * self,
    GstClockTime ts, GstPad * pad, GstQuery * query);
static void gst_shark_tracer_hook_pad_query_post (GObject * self,
    GstClockTime ts, GstPad * pad, GstQuery * query, gboolean res);
static void gst_shark_tracer_hook_element_post_message_pre (GObject * self,
    GstClockTime ts, GstElement * element, GstMessage * message);
static void gst_shark_tracer_hook_element_post_message_post (GObject * self,
    GstClockTime ts, GstElement * element, gboolean res);
static void gst_shark_tracer_hook_element_query_pre (GObject * self,
    GstClockTime ts, GstElement * element, GstQuery * query);
static void gst_shark_tracer_hook_element_query_post (GObject * self,
    GstClockTime ts, GstElement * element, GstQuery * query, gboolean res);
static void gst_shark_tracer_hook_element_new (GObject * self, GstClockTime ts,
    GstElement * element);
static void gst_shark_tracer_hook_element_add_pad (GObject * self,
    GstClockTime ts, GstElement * element, GstPad * pad);
static void gst_shark_tracer_hook_element_remove_pad (GObject * self,
    GstClockTime ts, GstElement * element, GstPad * pad);
static void gst_shark_tracer_hook_element_change_state_pre (GObject * self,
    GstClockTime ts, GstElement * element, GstStateChange transition);
static void gst_shark_tracer_hook_element_change_state_post (GObject * self,
    GstClockTime ts, GstElement * element, GstStateChange transition,
    GstStateChangeReturn result);
static void gst_shark_tracer_hook_bin_add_pre (GObject * self, GstClockTime ts,
    GstBin * bin, GstElement * element);
static void gst_shark_tracer_hook_bin_add_post (GObject * self, GstClockTime ts,
    GstBin * bin, GstElement * element, gboolean result);
static void gst_shark_tracer_hook_bin_remove_pre (GObject * self,
    GstClockTime ts, GstBin * bin, GstElement * element);
static void gst_shark_tracer_hook_bin_remove_post (GObject * self,
    GstClockTime ts, GstBin * bin, gboolean result);
static void gst_shark_tracer_hook_pad_link_pre (GObject * self, GstClockTime ts,
    GstPad * srcpad, GstPad * sinkpad);
static void gst_shark_tracer_hook_pad_link_post (GObject * self,
    GstClockTime ts, GstPad * srcpad, GstPad * sinkpad,
    GstPadLinkReturn result);
static void gst_shark_tracer_hook_pad_unlink_pre (GObject * self,
    GstClockTime ts, GstPad * srcpad, GstPad * sinkpad);
static void gst_shark_tracer_hook_pad_unlink_post (GObject * self,
    GstClockTime ts, GstPad * srcpad, GstPad * sinkpad, gboolean result);
static void gst_shark_tracer_hook_mini_object_created (GObject * self,
    GstClockTime ts, GstMiniObject * object);
static void gst_shark_tracer_hook_mini_object_destroyed (GObject * self,
    GstClockTime ts, GstMiniObject * object);
static void gst_shark_tracer_hook_object_unreffed (GObject * self,
    GstClockTime ts, GstObject * object, gint new_refcount);
static void gst_shark_tracer_hook_object_reffed (GObject * self,
    GstClockTime ts, GstObject * object, gint new_refcount);
static void gst_shark_tracer_hook_mini_object_unreffed (GObject * self,
    GstClockTime ts, GstMiniObject * object, gint new_refcount);
static void gst_shark_tracer_hook_mini_object_reffed (GObject * self,
    GstClockTime ts, GstMiniObject * object, gint new_refcount);
static void gst_shark_tracer_hook_object_created (GObject * self,
    GstClockTime ts, GstObject * object);
static void gst_shark_tracer_hook_object_destroyed (GObject * self,
    GstClockTime ts, GstObject * object);


G_DEFINE_TYPE_WITH_PRIVATE (GstSharkTracer, gst_shark_tracer, GST_TYPE_TRACER);

static void
gst_shark_tracer_class_init (GstSharkTracerClass * klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_shark_debug, "sharktracer", 0,
      "base shark tracer");

  oclass->constructed = gst_shark_tracer_constructed;
  oclass->finalize = gst_shark_tracer_finalize;
}

static void
gst_shark_tracer_init (GstSharkTracer * self)
{
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);

  priv->params = g_hash_table_new (g_str_hash, g_str_equal);
  priv->hooks = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  priv->myhooks = g_hash_table_new (g_str_hash, g_str_equal);

  gst_shark_tracer_fill_hooks (priv);
}

static void
gst_shark_tracer_fill_hooks (GstSharkTracerPrivate * priv)
{
  g_hash_table_insert (priv->myhooks, (gpointer) "pad-push-pre",
      (void*)gst_shark_tracer_hook_pad_push_pre);
  g_hash_table_insert (priv->myhooks, (gpointer) "pad-push-post",
      (void*)gst_shark_tracer_hook_pad_push_post);
  g_hash_table_insert (priv->myhooks, (gpointer) "pad-push-list-pre",
      (void*)gst_shark_tracer_hook_pad_push_list_pre);
  g_hash_table_insert (priv->myhooks, (gpointer) "pad-push-list-post",
      (void*)gst_shark_tracer_hook_pad_push_list_post);
  g_hash_table_insert (priv->myhooks, (gpointer) "pad-pull-range-pre",
      (void*)gst_shark_tracer_hook_pad_pull_range_pre);
  g_hash_table_insert (priv->myhooks, (gpointer) "pad-pull-range-post",
      (void*)gst_shark_tracer_hook_pad_pull_range_post);
  g_hash_table_insert (priv->myhooks, (gpointer) "pad-push-event-pre",
      (void*)gst_shark_tracer_hook_pad_push_event_pre);
  g_hash_table_insert (priv->myhooks, (gpointer) "pad-push-event-post",
      (void*)gst_shark_tracer_hook_pad_push_event_post);
  g_hash_table_insert (priv->myhooks, (gpointer) "pad-query-pre",
      (void*)gst_shark_tracer_hook_pad_query_pre);
  g_hash_table_insert (priv->myhooks, (gpointer) "pad-query-post",
      (void*)gst_shark_tracer_hook_pad_query_post);
  g_hash_table_insert (priv->myhooks, (gpointer) "element-post-message-pre",
      (void*)gst_shark_tracer_hook_element_post_message_pre);
  g_hash_table_insert (priv->myhooks, (gpointer) "element-post-message-post",
      (void*)gst_shark_tracer_hook_element_post_message_post);
  g_hash_table_insert (priv->myhooks, (gpointer) "element-query-pre",
      (void*)gst_shark_tracer_hook_element_query_pre);
  g_hash_table_insert (priv->myhooks, (gpointer) "element-query-post",
      (void*)gst_shark_tracer_hook_element_query_post);
  g_hash_table_insert (priv->myhooks, (gpointer) "element-new",
      (void*)gst_shark_tracer_hook_element_new);
  g_hash_table_insert (priv->myhooks, (gpointer) "element-add-pad",
      (void*)gst_shark_tracer_hook_element_add_pad);
  g_hash_table_insert (priv->myhooks, (gpointer) "element-remove-pad",
      (void*)gst_shark_tracer_hook_element_remove_pad);
  g_hash_table_insert (priv->myhooks, (gpointer) "bin-add-pre",
      (void*)gst_shark_tracer_hook_bin_add_pre);
  g_hash_table_insert (priv->myhooks, (gpointer) "bin-add-post",
      (void*)gst_shark_tracer_hook_bin_add_post);
  g_hash_table_insert (priv->myhooks, (gpointer) "bin-remove-pre",
      (void*)gst_shark_tracer_hook_bin_remove_pre);
  g_hash_table_insert (priv->myhooks, (gpointer) "bin-remove-post",
      (void*)gst_shark_tracer_hook_bin_remove_post);
  g_hash_table_insert (priv->myhooks, (gpointer) "pad-link-pre",
      (void*)gst_shark_tracer_hook_pad_link_pre);
  g_hash_table_insert (priv->myhooks, (gpointer) "pad-link-post",
      (void*)gst_shark_tracer_hook_pad_link_post);
  g_hash_table_insert (priv->myhooks, (gpointer) "pad-unlink-pre",
      (void*)gst_shark_tracer_hook_pad_unlink_pre);
  g_hash_table_insert (priv->myhooks, (gpointer) "pad-unlink-post",
      (void*)gst_shark_tracer_hook_pad_unlink_post);
  g_hash_table_insert (priv->myhooks, (gpointer) "element-change-state-pre",
      (void*)gst_shark_tracer_hook_element_change_state_pre);
  g_hash_table_insert (priv->myhooks, (gpointer) "element-change-state-post",
      (void*)gst_shark_tracer_hook_element_change_state_post);
  g_hash_table_insert (priv->myhooks, (gpointer) "mini-object-created",
      (void*)gst_shark_tracer_hook_mini_object_created);
  g_hash_table_insert (priv->myhooks, (gpointer) "mini-object-destroyed",
      (void*)gst_shark_tracer_hook_mini_object_destroyed);
  g_hash_table_insert (priv->myhooks, (gpointer) "object-created",
      (void*)gst_shark_tracer_hook_object_created);
  g_hash_table_insert (priv->myhooks, (gpointer) "object-destroyed",
      (void*)gst_shark_tracer_hook_object_destroyed);
  g_hash_table_insert (priv->myhooks, (gpointer) "mini-object-reffed",
      (void*)gst_shark_tracer_hook_mini_object_reffed);
  g_hash_table_insert (priv->myhooks, (gpointer) "mini-object-unreffed",
      (void*)gst_shark_tracer_hook_mini_object_unreffed);
  g_hash_table_insert (priv->myhooks, (gpointer) "object-reffed",
      (void*)gst_shark_tracer_hook_object_reffed);
  g_hash_table_insert (priv->myhooks, (gpointer) "object-unreffed",
      (void*)gst_shark_tracer_hook_object_unreffed);
}

static void
gst_shark_tracer_constructed (GObject * object)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);

  gst_shark_tracer_save_params (self);
}

static void
gst_shark_tracer_free_params (GstSharkTracerPrivate * priv)
{
  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init (&iter, priv->params);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    g_free (key);
    g_list_free_full ((GList*)value, g_free);
  }
}

static void
gst_shark_tracer_finalize (GObject * object)
{
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (object);

  gst_shark_tracer_free_params (priv);
  g_hash_table_unref (priv->params);
  g_hash_table_unref (priv->hooks);
  g_hash_table_unref (priv->myhooks);

  G_OBJECT_CLASS (gst_shark_tracer_parent_class)->finalize (object);
}

static void
gst_shark_tracer_save_params (GstSharkTracer * self)
{
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  gchar **tokens;
  gchar *params;
  gint i;

  g_object_get (self, "params", &params, NULL);

  if (NULL == params) {
    return;
  }

  tokens = g_strsplit_set (params, ",", 0);
  for (i = 0; tokens[i]; ++i) {
    GList *param;
    gchar **keyvalue;

    keyvalue = g_strsplit_set (tokens[i], "=", 2);
    if (NULL == keyvalue[1]) {
      GST_ERROR_OBJECT (self, "Malformed parameter %s", tokens[i]);
      goto cont;
    }

    GST_DEBUG_OBJECT (self, "Found param %s with value %s", keyvalue[0],
        keyvalue[1]);

    /* If there is no param with the given key yet, the lookup will
       return NULL. NULL is a valid value for prepend, in which case a
       new list will be started.
     */
    param = (GList*)g_hash_table_lookup (priv->params, keyvalue[0]);
    param = g_list_append (param, g_strdup (keyvalue[1]));
    g_hash_table_replace (priv->params, g_strdup (keyvalue[0]), param);

    GST_INFO_OBJECT (self, "Appended %s to %s", keyvalue[1], keyvalue[0]);

  cont:
    g_strfreev (keyvalue);
  }
  g_strfreev (tokens);
  g_free (params);

  if (i > 0) {
    gst_shark_tracer_dump_params (self);
  }
}

static void
gst_shark_tracer_dump_params (GstSharkTracer * self)
{
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GList *keys;
  GList *key;

  GST_INFO_OBJECT (self, "Dumping %d parameters:",
      g_hash_table_size (priv->params));

  keys = g_hash_table_get_keys (priv->params);

  for (key = keys; NULL != key; key = g_list_next (key)) {
    GList *values = (GList*)g_hash_table_lookup (priv->params, key->data);
    GList *value;

    GST_INFO_OBJECT (self, "\t- %s (%d)", (gchar *) key->data,
        g_list_length (values));
    for (value = values; NULL != value; value = g_list_next (value)) {
      GST_INFO_OBJECT (self, "\t\t- %s", (gchar *) value->data);
    }
  }

  g_list_free (keys);
}

gboolean
gst_shark_tracer_element_is_filtered (GstSharkTracer * self,
    const gchar * element)
{
  GstSharkTracerPrivate *priv;
  GList *filters;
  GList *filter;
  const gchar *filter_tag = "filter";
  gboolean is_filtered = FALSE;

  g_return_val_if_fail (self, FALSE);
  g_return_val_if_fail (element, FALSE);

  priv = GST_SHARK_TRACER_PRIVATE (self);

  GST_LOG_OBJECT (self, "Looking if user has filtered %s", element);

  filters = (GList*)g_hash_table_lookup (priv->params, filter_tag);
  if (NULL == filters) {
    GST_LOG_OBJECT (self, "There are no filters specified");
    return TRUE;
  }

  /* TODO: Cache this results to avoid compiling the regex every evaluation */
  for (filter = filters; NULL != filter; filter = g_list_next (filters)) {
    const gchar *pattern = (const gchar *) filter->data;

    is_filtered = g_regex_match_simple (pattern, element, (GRegexCompileFlags)0, (GRegexMatchFlags)0);
    if (is_filtered) {
      break;
    }
  }

  GST_LOG_OBJECT (self, "Element %s was filtered: %s", element,
      is_filtered ? "true" : "false");

  return is_filtered;
}

GList *
gst_shark_tracer_get_param (GstSharkTracer * self, const gchar * param)
{
  GstSharkTracerPrivate *priv = NULL;

  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (param, NULL);

  priv = GST_SHARK_TRACER_PRIVATE (self);

  return (GList*)g_hash_table_lookup (priv->params, param);
}

void
gst_shark_tracer_register_hook (GstSharkTracer * self, const gchar * detail,
    GCallback func)
{
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);

  if (!g_hash_table_contains (priv->hooks, detail)) {
    GCallback myhook = (GCallback)g_hash_table_lookup (priv->myhooks, detail);

    GST_INFO_OBJECT (self, "Registering new shark hook for %s", detail);

    /* Save child's hook */
    g_hash_table_insert (priv->hooks, g_strdup (detail), (void*)func);

    /* Register our hook */
    gst_tracing_register_hook (GST_TRACER (self), detail, myhook);
  }
}

/* My hooks */
static void
gst_shark_tracer_hook_pad_push_pre (GObject * object, GstClockTime ts,
    GstPad * pad, GstBuffer * buffer)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (GST_OBJECT_PARENT (pad));
  if (FALSE == gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "pad-push-pre");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime, GstPad *, GstBuffer *)) hook) (object, ts,
      pad, buffer);
}

static void
gst_shark_tracer_hook_pad_push_post (GObject * object, GstClockTime ts,
    GstPad * pad, GstFlowReturn res)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (GST_OBJECT_PARENT (pad));
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "pad-push-post");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime, GstPad *, GstFlowReturn)) hook) (object,
      ts, pad, res);
}

static void
gst_shark_tracer_hook_pad_push_list_pre (GObject * object, GstClockTime ts,
    GstPad * pad, GstBufferList * list)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (GST_OBJECT_PARENT (pad));
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "pad-push-pre");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime, GstPad *, GstBufferList *)) hook) (object,
      ts, pad, list);
}

static void
gst_shark_tracer_hook_pad_push_list_post (GObject * object, GstClockTime ts,
    GstPad * pad, GstFlowReturn res)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (GST_OBJECT_PARENT (pad));
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "pad-push-list-post");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime, GstPad *, GstFlowReturn)) hook) (object,
      ts, pad, res);
}

static void
gst_shark_tracer_hook_pad_pull_range_pre (GObject * object, GstClockTime ts,
    GstPad * pad, guint64 offset, guint size)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (GST_OBJECT_PARENT (pad));
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "pad-pull-range-pre");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime, GstPad *, guint64, guint)) hook) (object,
      ts, pad, offset, size);
}

static void
gst_shark_tracer_hook_pad_pull_range_post (GObject * object, GstClockTime ts,
    GstPad * pad, GstBuffer * buffer, GstFlowReturn res)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (GST_OBJECT_PARENT (pad));
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "pad-pull-range-post");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime, GstPad *, GstBuffer *,
              GstFlowReturn)) hook) (object, ts, pad, buffer, res);
}

static void
gst_shark_tracer_hook_pad_push_event_pre (GObject * object, GstClockTime ts,
    GstPad * pad, GstEvent * event)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (GST_OBJECT_PARENT (pad));
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "pad-push-event-pre");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime, GstPad *, GstEvent *)) hook) (object, ts,
      pad, event);
}

static void
gst_shark_tracer_hook_pad_push_event_post (GObject * object, GstClockTime ts,
    GstPad * pad, gboolean res)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (GST_OBJECT_PARENT (pad));
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "pad-push-event-post");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime, GstPad *, gboolean)) hook) (object, ts,
      pad, res);
}

static void
gst_shark_tracer_hook_pad_query_pre (GObject * object, GstClockTime ts,
    GstPad * pad, GstQuery * query)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (GST_OBJECT_PARENT (pad));
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "pad-query-pre");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime, GstPad *, GstQuery *)) hook) (object, ts,
      pad, query);
}

static void
gst_shark_tracer_hook_pad_query_post (GObject * object, GstClockTime ts,
    GstPad * pad, GstQuery * query, gboolean res)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (GST_OBJECT_PARENT (pad));
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "pad-query-post");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime, GstPad *, GstQuery *,
              gboolean)) hook) (object, ts, pad, query, res);
}

static void
gst_shark_tracer_hook_element_post_message_pre (GObject * object,
    GstClockTime ts, GstElement * element, GstMessage * message)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (element);
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "element-post-message-pre");
  g_return_if_fail (hook);

  ((void (*)(GObject *,
              GstClockTime, GstElement *, GstMessage *)) hook) (object, ts,
      element, message);
}

static void
gst_shark_tracer_hook_element_post_message_post (GObject * object,
    GstClockTime ts, GstElement * element, gboolean res)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (element);
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "element-post-message-post");
  g_return_if_fail (hook);

  ((void (*)(GObject *,
              GstClockTime, GstElement *, gboolean)) hook) (object, ts, element,
      res);
}

static void
gst_shark_tracer_hook_element_query_pre (GObject * object, GstClockTime ts,
    GstElement * element, GstQuery * query)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (element);
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "element-query-pre");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstElement *, GstQuery *)) hook) (object, ts, element, query);
}

static void
gst_shark_tracer_hook_element_query_post (GObject * object, GstClockTime ts,
    GstElement * element, GstQuery * query, gboolean res)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (element);
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "element-query-post");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstElement *, GstQuery *, gboolean)) hook) (object, ts, element,
      query, res);
}

static void
gst_shark_tracer_hook_element_new (GObject * object, GstClockTime ts,
    GstElement * element)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (element);
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "element-new");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstElement *)) hook) (object, ts, element);
}

static void
gst_shark_tracer_hook_element_add_pad (GObject * object, GstClockTime ts,
    GstElement * element, GstPad * pad)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (element);
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "element-add-pad");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstElement *, GstPad *)) hook) (object, ts, element, pad);
}

static void
gst_shark_tracer_hook_element_remove_pad (GObject * object, GstClockTime ts,
    GstElement * element, GstPad * pad)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (element);
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "element-remove-pad");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstElement *, GstPad *)) hook) (object, ts, element, pad);
}

static void
gst_shark_tracer_hook_element_change_state_pre (GObject * object,
    GstClockTime ts, GstElement * element, GstStateChange transition)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (element);
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "element-change-state-pre");
  g_return_if_fail (hook);

  ((void (*)(GObject *,
              GstClockTime, GstElement *, GstStateChange)) hook) (object, ts,
      element, transition);
}

static void
gst_shark_tracer_hook_element_change_state_post (GObject * object,
    GstClockTime ts, GstElement * element, GstStateChange transition,
    GstStateChangeReturn result)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (element);
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "element-change-state-post");
  g_return_if_fail (hook);

  ((void (*)(GObject *,
              GstClockTime, GstElement *, GstStateChange,
              GstStateChangeReturn)) hook) (object, ts, element, transition,
      result);
}

static void
gst_shark_tracer_hook_bin_add_pre (GObject * object, GstClockTime ts,
    GstBin * bin, GstElement * element)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (element);
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "bin-add-pre");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstBin *, GstElement *)) hook) (object, ts, bin, element);
}

static void
gst_shark_tracer_hook_bin_add_post (GObject * object, GstClockTime ts,
    GstBin * bin, GstElement * element, gboolean result)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (element);
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "bin-add-post");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstBin *, GstElement *, gboolean)) hook) (object, ts, bin,
      element, result);
}

static void
gst_shark_tracer_hook_bin_remove_pre (GObject * object, GstClockTime ts,
    GstBin * bin, GstElement * element)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (element);
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "bin-remove-pre");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstBin *, GstElement *)) hook) (object, ts, bin, element);
}

static void
gst_shark_tracer_hook_bin_remove_post (GObject * object, GstClockTime ts,
    GstBin * bin, gboolean result)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (bin);
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "bin-remove-post");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstBin *, gboolean)) hook) (object, ts, bin, result);
}

static void
gst_shark_tracer_hook_pad_link_pre (GObject * object, GstClockTime ts,
    GstPad * srcpad, GstPad * sinkpad)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (GST_OBJECT_PARENT (srcpad));
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "pad-link-pre");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstPad *, GstPad *)) hook) (object, ts, srcpad, sinkpad);
}

static void
gst_shark_tracer_hook_pad_link_post (GObject * object, GstClockTime ts,
    GstPad * srcpad, GstPad * sinkpad, GstPadLinkReturn result)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (GST_OBJECT_PARENT (srcpad));
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "pad-link-post");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstPad *, GstPad *, GstPadLinkReturn)) hook) (object, ts, srcpad,
      sinkpad, result);
}

static void
gst_shark_tracer_hook_pad_unlink_pre (GObject * object, GstClockTime ts,
    GstPad * srcpad, GstPad * sinkpad)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (GST_OBJECT_PARENT (srcpad));
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "pad-unlink-pre");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstPad *, GstPad *)) hook) (object, ts, srcpad, sinkpad);
}

static void
gst_shark_tracer_hook_pad_unlink_post (GObject * object, GstClockTime ts,
    GstPad * srcpad, GstPad * sinkpad, gboolean result)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;
  const gchar *elname;

  elname = GST_OBJECT_NAME (GST_OBJECT_PARENT (srcpad));
  if (!gst_shark_tracer_element_is_filtered (self, elname)) {
    return;
  }

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "pad-unlink-post");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstPad *, GstPad *, gboolean)) hook) (object, ts, srcpad, sinkpad,
      result);
}

static void
gst_shark_tracer_hook_mini_object_created (GObject * object, GstClockTime ts,
    GstMiniObject * mobject)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "mini-object-created");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstMiniObject *)) hook) (object, ts, mobject);
}

static void
gst_shark_tracer_hook_mini_object_destroyed (GObject * object, GstClockTime ts,
    GstMiniObject * mobject)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "mini-object-destroyed");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstMiniObject *)) hook) (object, ts, mobject);
}

static void
gst_shark_tracer_hook_object_unreffed (GObject * object, GstClockTime ts,
    GstObject * obj, gint new_refcount)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "object-unreffed");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstObject *, gint)) hook) (object, ts, obj, new_refcount);
}

static void
gst_shark_tracer_hook_object_reffed (GObject * object, GstClockTime ts,
    GstObject * obj, gint new_refcount)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "object-reffed");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstObject *, gint)) hook) (object, ts, obj, new_refcount);
}

static void
gst_shark_tracer_hook_mini_object_unreffed (GObject * object, GstClockTime ts,
    GstMiniObject * obj, gint new_refcount)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "mini-object-unreffed");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstMiniObject *, gint)) hook) (object, ts, obj, new_refcount);
}

static void
gst_shark_tracer_hook_mini_object_reffed (GObject * object, GstClockTime ts,
    GstMiniObject * obj, gint new_refcount)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "mini-object-reffed");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime,
              GstMiniObject *, gint)) hook) (object, ts, obj, new_refcount);
}

static void
gst_shark_tracer_hook_object_created (GObject * object, GstClockTime ts,
    GstObject * obj)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;

  hook = (GCallback)g_hash_table_lookup (priv->hooks, "object-created");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime, GstObject *)) hook) (object, ts, obj);
}

static void
gst_shark_tracer_hook_object_destroyed (GObject * object, GstClockTime ts,
    GstObject * obj)
{
  GstSharkTracer *self = GST_SHARK_TRACER (object);
  GstSharkTracerPrivate *priv = GST_SHARK_TRACER_PRIVATE (self);
  GCallback hook;

  hook =(GCallback) g_hash_table_lookup (priv->hooks, "object-destroyed");
  g_return_if_fail (hook);

  ((void (*)(GObject *, GstClockTime, GstObject *)) hook) (object, ts, obj);
}
