/* GstShark - A Front End for GstTracer
 * Copyright (C) 2016 RidgeRun Engineering <michael.gruner@ridgerun.com>
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

G_BEGIN_DECLS

typedef struct _GstProcTime GstProcTime;

GstProcTime *gst_proctime_new (void);

void gst_proctime_add_new_element (GstProcTime * proc_time,
    GstElement * element);

gboolean gst_proctime_proc_time (GstProcTime * proc_time,
    GstClockTime * time, GstPad * peer_pad, GstPad * src_pad,
    GstClockTime ts, gboolean do_calculation);

void gst_proctime_free (GstProcTime * proc_time);

G_END_DECLS