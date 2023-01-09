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
#define CPU_NUM_MAX  8
/* Returns a reference to the array the contains the cpu usage computed */
#define CPU_USAGE_ARRAY(cpuusage_struct)  (cpuusage_struct->cpu_load)
/* Returns how many element contains the cpu_usage array
 * This value also represents the number of cpus in the system */
#define CPU_USAGE_ARRAY_LENGTH(cpuusage_struct)  (cpuusage_struct->cpu_num)
    typedef struct
{
  /* CPU core number */
  gint cpu_num;
  gfloat cpu_load[CPU_NUM_MAX];

  gint user[CPU_NUM_MAX];       /* Time spent in user mode */
  gint user_aux[CPU_NUM_MAX];   /* Time spent in user mode */
  gint nice[CPU_NUM_MAX];       /* Time spent in user mode with low priority */
  gint nice_aux[CPU_NUM_MAX];   /* Time spent in user mode with low priority */
  gint system[CPU_NUM_MAX];     /* Time spent in user mode with low priority */
  gint system_aux[CPU_NUM_MAX]; /* Time spent in user mode with low priority */
  gint idle[CPU_NUM_MAX];       /* Time spent in system mode */
  gint idle_aux[CPU_NUM_MAX];   /* Time spent in system mode */
  gboolean cpu_array_sel;
} GstCPUUsage;

void gst_cpu_usage_init (GstCPUUsage * cpu_usage);

void gst_cpu_usage_compute (GstCPUUsage * cpu_usage);

G_END_DECLS
