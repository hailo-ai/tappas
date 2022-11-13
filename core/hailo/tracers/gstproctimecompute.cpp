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

#include "gstproctimecompute.hpp"

typedef struct _GstProcTimeElement GstProcTimeElement;
struct _GstProcTimeElement
{
  GstPad *src_pad;
  GstPad *sink_pad;
  GstClockTime start_time;
};

struct _GstProcTime
{
  GList *elements;
};

static void free_element (gpointer data);
static void gst_proctime_add_in_list (GstProcTime * proc_time,
    GstPad * sink_pad, GstPad * src_pad);

static void
free_element (gpointer data)
{
  GstProcTimeElement *element;

  element = (GstProcTimeElement *) data;

  gst_object_unref (element->src_pad);
  element->src_pad = NULL;

  gst_object_unref (element->sink_pad);
  element->sink_pad = NULL;

  g_free (element);
}

GstProcTime *
gst_proctime_new (void)
{
  GstProcTime *self;

  self = (GstProcTime*)g_malloc (sizeof (GstProcTime));

  g_return_val_if_fail (self, NULL);

  self->elements = NULL;

  return self;
}

void
gst_proctime_free (GstProcTime * self)
{
  g_return_if_fail (self);

  g_list_free_full (self->elements, free_element);
  g_free (self);
}

/* Add a new element in the list.
 * The element added must be an element with only one src pad and one 
 * sink pad.
 */
static void
gst_proctime_add_in_list (GstProcTime * proc_time, GstPad * sink_pad,
    GstPad * src_pad)
{
  GstProcTimeElement *new_element;

  g_return_if_fail (proc_time);
  g_return_if_fail (sink_pad);
  g_return_if_fail (src_pad);

  new_element = (GstProcTimeElement*)g_malloc0 (sizeof (GstProcTimeElement));
  new_element->start_time = GST_CLOCK_TIME_NONE;

  new_element->sink_pad = (GstPad*)gst_object_ref (sink_pad);
  new_element->src_pad = (GstPad*)gst_object_ref (src_pad);

  proc_time->elements = g_list_append (proc_time->elements, new_element);
}

void
gst_proctime_add_new_element (GstProcTime * proc_time, GstElement * element)
{
  GstIterator *src_iterator = NULL;
  GstIterator *sink_iterator = NULL;
  GValue vpad = G_VALUE_INIT;
  gint num_src_pads = 0;
  gint num_sink_pads = 0;
  GstPad *src_pad = NULL;
  GstPad *sink_pad = NULL;

  g_return_if_fail (proc_time);
  g_return_if_fail (element);

  src_iterator = gst_element_iterate_src_pads (element);
  while (gst_iterator_next (src_iterator, &vpad) == GST_ITERATOR_OK) {
    src_pad = GST_PAD (g_value_get_object (&vpad));
    g_value_reset (&vpad);
    num_src_pads++;

    if (num_src_pads > 1) {
      goto out;
    }
  }

  sink_iterator = gst_element_iterate_sink_pads (element);
  while (gst_iterator_next (sink_iterator, &vpad) == GST_ITERATOR_OK) {
    sink_pad = GST_PAD (g_value_get_object (&vpad));
    g_value_reset (&vpad);
    num_sink_pads++;

    if (num_sink_pads > 1) {
      goto out;
    }
  }

  /* We are only interested in elements with one sink and src pad */
  if (num_src_pads == 1 && num_sink_pads == 1) {
    gst_proctime_add_in_list (proc_time, sink_pad, src_pad);
  }

out:
  {
    g_value_unset (&vpad);

    if (NULL != src_iterator) {
      gst_iterator_free (src_iterator);
    }

    if (NULL != sink_iterator) {
      gst_iterator_free (sink_iterator);
    }
  }
}

gboolean
gst_proctime_proc_time (GstProcTime * proc_time, GstClockTime * time,
    GstPad * peer_pad, GstPad * src_pad, GstClockTime ts,
    gboolean do_calculation)
{
  GstProcTimeElement *element;
  GstClockTime stop_time;
  guint elem_num;
  guint elem_idx;
  gboolean found = FALSE;

  g_return_val_if_fail (proc_time, FALSE);
  g_return_val_if_fail (time, FALSE);
  g_return_val_if_fail (src_pad, FALSE);
  g_return_val_if_fail (peer_pad, FALSE);

  elem_num = g_list_length (proc_time->elements);
  /* Search the peer pad in the list 
   * The peer pad is used to identify which is the element where the 
   * buffer is received.
   */
  for (elem_idx = 0; elem_idx < elem_num; ++elem_idx) {
    element = (GstProcTimeElement*)g_list_nth_data (proc_time->elements, elem_idx);
    if (element->sink_pad == peer_pad) {
      element->start_time = ts;
    }
  }

  if (!do_calculation)
    goto exit;

  /* Search the src pad in the list 
   * The peer pad is used to identify which is the element where the 
   * buffer was processed.
   * If the src pad is not in the list, then it is a src element and the
   * precessing time is not computed
   */
  for (elem_idx = 0; elem_idx < elem_num; ++elem_idx) {
    element = (GstProcTimeElement*)g_list_nth_data (proc_time->elements, elem_idx);
    if (element->src_pad == src_pad) {
      stop_time = ts;
      if (stop_time > element->start_time) {
        *time = stop_time - element->start_time;
      } else {
        /* FIXME: For elements storing buffers (e.g queues) there are
           timestamps mismatches sometimes, because more than 1 buffer
           is pushed before getting 1 at the output */
        GST_WARNING_OBJECT (element->src_pad,
            "Timestamps mismatch, this should not happen");
        found = FALSE;
        goto exit;
      }
      found = TRUE;
    }
  }

exit:
  return found;
}
