/* GstShark - A Front End for GstTracer
 * Copyright (C) 2018 RidgeRun Engineering <michael.gruner@ridgerun.com>
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
 * SECTION:gstgraphic
 * @short_description: display pipeline graphic
 *
 * A tracing module that uses the DOT libraries in order to show the pipeline executed graphically
 */

#include "gstgraphic.h"
#include "gstdot.h"

GST_DEBUG_CATEGORY_STATIC (gst_graphic_debug);
#define GST_CAT_DEFAULT gst_graphic_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_STATES);

struct _GstGraphicTracer
{
  GstSharkTracer parent;
};

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (gst_graphic_debug, "graphic", 0, "graphic tracer"); \
    GST_DEBUG_CATEGORY_GET (GST_CAT_STATES, "GST_STATES");

G_DEFINE_TYPE_WITH_CODE (GstGraphicTracer, gst_graphic_tracer,
    GST_SHARK_TYPE_TRACER, _do_init);

static void do_element_change_state_post (GstGraphicTracer * self, guint64 ts,
    GstElement * element, GstStateChange transition,
    GstStateChangeReturn result);
static void gst_graphic_tracer_finalize (GObject * obj);

static void
do_element_change_state_post (GstGraphicTracer * self, guint64 ts,
    GstElement * element, GstStateChange transition,
    GstStateChangeReturn result)
{
  gchar *dot_string;
  GstDotRender render;

  if (GST_IS_PIPELINE (element)
      && (transition == GST_STATE_CHANGE_PAUSED_TO_PLAYING)) {
    /* Logging the change of state in which the pipeline graphic is being done */
    GST_CAT_TRACE (GST_CAT_STATES,
        "%" GST_TIME_FORMAT ", element=%" GST_PTR_FORMAT ", change=%d, res=%d",
        GST_TIME_ARGS (ts), element, (gint) transition, (gint) result);

    /* Using a function pointer for passing the function that does
       the pipeline graphic as a parameter */
    render = gst_dot_x11_render;

    /* Getting the DOT string information */
    dot_string = gst_dot_pipeline_to_string (GST_PIPELINE (element));
    gst_dot_do_render (dot_string, render, NULL);
    g_free (dot_string);
  }
}

/* tracer class */

static void
gst_graphic_tracer_finalize (GObject * obj)
{
  G_OBJECT_CLASS (gst_graphic_tracer_parent_class)->finalize (obj);
}

static void
gst_graphic_tracer_class_init (GstGraphicTracerClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = gst_graphic_tracer_finalize;
}

static void
gst_graphic_tracer_init (GstGraphicTracer * self)
{
  GstTracer *tracer = GST_TRACER (self);
  gst_tracing_register_hook (tracer, "element-change-state-post",
      G_CALLBACK (do_element_change_state_post));
}
