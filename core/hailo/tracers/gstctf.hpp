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
#pragma once

#include <gst/gst.h>
G_BEGIN_DECLS typedef struct _GstCtfDescriptor GstCtfDescriptor;

typedef enum
{
  INIT_EVENT_ID,
  CPUUSAGE_EVENT_ID,
  PROCTIME_EVENT_ID,
  INTERLATENCY_EVENT_ID,
  FPS_EVENT_ID,
  SCHED_TIME_EVENT_ID,
  QUEUE_LEVEL_EVENT_ID,
  BITRATE_EVENT_ID,
  BUFFER_EVENT_ID,
} event_id;

gchar *get_ctf_path_name (void);
gboolean gst_ctf_init (void);
void gst_ctf_close (void);
void add_metadata_event_struct (const gchar * metadata_event);
void do_print_cpuusage_event (event_id id, guint32 cpunum, gfloat * cpuload);
void do_print_proctime_event (event_id id, gchar * elementname, guint64 time);
void do_print_framerate_event (event_id id, gchar * elementname, guint64 fps);
void do_print_interlatency_event (event_id id,
    char *originpad, gchar * destinationpad, guint64 time);
void do_print_scheduling_event (event_id id, gchar * elementname, guint64 time);
void do_print_queue_level_event (event_id id, const gchar * elementname, guint32 bytes,
    guint32 max_bytes, guint32 buffers, guint32 max_buffers, guint64 time, guint64 max_time);
void do_print_bitrate_event (event_id id, gchar * elementname, guint64 bps);
void do_print_buffer_event (event_id id, const gchar * pad, GstClockTime pts,
    GstClockTime dts, GstClockTime duration, guint64 offset,
    guint64 offset_end, guint64 size, GstBufferFlags flags,
    guint32 refcount);
void do_print_ctf_init (event_id id);
G_END_DECLS