/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
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