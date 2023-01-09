/* GstShark - A Front End for GstTracer
 * Copyright (C) 2016-2018 RidgeRun Engineering <michael.gruner@ridgerun.com>
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
#pragma once

#include <gst/gst.h>
#include <gst/gsttracer.h>

G_BEGIN_DECLS

#define GST_SHARK_TYPE_TRACER (gst_shark_tracer_get_type())
G_DECLARE_DERIVABLE_TYPE (GstSharkTracer, gst_shark_tracer, GST_SHARK, TRACER, GstTracer)

struct _GstSharkTracerClass
{
  GstTracerClass parent_class;
};

gboolean gst_shark_tracer_element_is_filtered (GstSharkTracer *self, const gchar *regex);
GList * gst_shark_tracer_get_param (GstSharkTracer *self, const gchar *param);

void gst_shark_tracer_register_hook (GstSharkTracer *self, const gchar *detail,
    GCallback func);

G_END_DECLS