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
// #define CPU_NUM_MAX  8
/* Returns a reference to the array the contains the cpu usage computed */
// #define THREAD_MONITOR_ARRAY(cpuusage_struct)  (cpuusage_struct->cpu_load)
/* Returns how many element contains the thread_monitor array
 * This value also represents the number of cpus in the system */
// #define THREAD_MONITOR_ARRAY_LENGTH(cpuusage_struct)  (cpuusage_struct->cpu_num)
typedef struct
{
  guint thread_name_loc;
  guint thread_cpu_usage_loc;
  guint thread_memory_usage_loc;
} GstThreadMonitor;

void gst_thread_monitor_init(GstThreadMonitor *cpu_usage);

void gst_thread_monitor_compute(GstTracerRecord *tr_threadmonitor, GstThreadMonitor *thread_monitor, gchar **thread_name, gchar **thread_cpu_usage,
                                gchar **thread_memory_usage);

G_END_DECLS