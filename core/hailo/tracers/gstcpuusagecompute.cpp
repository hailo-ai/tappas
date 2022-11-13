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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/gst.h>
#include <glib/gstdio.h>
#include "gstcpuusage.hpp"
#include "gstcpuusagecompute.hpp"

#include <unistd.h>
#include <string.h>

#define CPU_NAME_MAX_SIZE 8
#define S(arg) XS(arg)
#define XS(arg) #arg

void
gst_cpu_usage_init (GstCPUUsage * cpu_usage)
{
  gint cpu_num = 0;

  g_return_if_fail (cpu_usage);

  memset (cpu_usage, 0, sizeof (GstCPUUsage));
  cpu_usage->cpu_array_sel = FALSE;

  if ((cpu_num = sysconf (_SC_NPROCESSORS_CONF)) == -1) {
    GST_WARNING ("failed to get number of cpus");
    cpu_num = 1;
  }

  cpu_usage->cpu_num = cpu_num;
}

void
gst_cpu_usage_compute (GstCPUUsage * cpu_usage)
{
  gfloat *cpu_load;
  gint cpu_num;
  gint cpu_id;
  FILE *fd;

  gint *user;
  gint *user_aux;
  gint *nice;
  gint *nice_aux;
  gint *system;
  gint *system_aux;
  gint *idle;
  gint *idle_aux;

  gchar cpu_name[CPU_NAME_MAX_SIZE];
  gint iowait;                  /* Time waiting for I/O to complete */
  gint irq;                     /* Time servicing interrupts        */
  gint softirq;                 /* Time servicing softirqs          */
  gint steal;                   /* Time spent in other OSes when in virtualized env */
  gint quest;                   /* Time spent running a virtual CPU for guest OS    */
  gint quest_nice;              /* Time spent running niced guest */
  gfloat num_value;
  gfloat den_value;
  gboolean cpu_array_sel;
  gint ret;

  g_return_if_fail (cpu_usage);

  user = cpu_usage->user;
  user_aux = cpu_usage->user_aux;
  nice = cpu_usage->nice;
  nice_aux = cpu_usage->nice_aux;
  system = cpu_usage->system;
  system_aux = cpu_usage->system_aux;
  idle = cpu_usage->idle;
  idle_aux = cpu_usage->idle_aux;

  cpu_array_sel = cpu_usage->cpu_array_sel;
  cpu_load = cpu_usage->cpu_load;
  cpu_num = cpu_usage->cpu_num;



  /* Compute the load for each core */
  fd = g_fopen ("/proc/stat", "r");
  if (cpu_array_sel == 0) {
    ret =
        fscanf (fd, "%" S (CPU_NAME_MAX_SIZE) "s %d %d %d %d %d %d %d %d %d %d",
        cpu_name, &user[0], &nice[0], &system[0], &idle[0], &iowait, &irq,
        &softirq, &steal, &quest, &quest_nice);
    for (cpu_id = 0; cpu_id < cpu_num; ++cpu_id) {
      ret =
          fscanf (fd,
          "%" S (CPU_NAME_MAX_SIZE) "s %d %d %d %d %d %d %d %d %d %d", cpu_name,
          &user[cpu_id], &nice[cpu_id], &system[cpu_id], &idle[cpu_id], &iowait,
          &irq, &softirq, &steal, &quest, &quest_nice);
    }
    /* Compute the utilization for each core */
    for (cpu_id = 0; cpu_id < cpu_num; ++cpu_id) {
      num_value =
          ((user[cpu_id] + nice[cpu_id] + system[cpu_id]) - (user_aux[cpu_id] +
              nice_aux[cpu_id] + system_aux[cpu_id]));
      den_value =
          ((user[cpu_id] + nice[cpu_id] + system[cpu_id] + idle[cpu_id]) -
          (user_aux[cpu_id] + nice_aux[cpu_id] + system_aux[cpu_id] +
              idle_aux[cpu_id]));
      cpu_load[cpu_id] = 100 * (num_value / den_value);
    }
    cpu_array_sel = 1;
  } else {
    ret =
        fscanf (fd, "%" S (CPU_NAME_MAX_SIZE) "s %d %d %d %d %d %d %d %d %d %d",
        cpu_name, &user_aux[0], &nice_aux[0], &system_aux[0], &idle_aux[0],
        &iowait, &irq, &softirq, &steal, &quest, &quest_nice);
    for (cpu_id = 0; cpu_id < cpu_num; ++cpu_id) {
      ret =
          fscanf (fd,
          "%" S (CPU_NAME_MAX_SIZE) "s %d %d %d %d %d %d %d %d %d %d", cpu_name,
          &user_aux[cpu_id], &nice_aux[cpu_id], &system_aux[cpu_id],
          &idle_aux[cpu_id], &iowait, &irq, &softirq, &steal, &quest,
          &quest_nice);
    }
    /* Compute the utilization for each core */
    for (cpu_id = 0; cpu_id < cpu_num; ++cpu_id) {
      num_value =
          ((user_aux[cpu_id] + nice_aux[cpu_id] + system_aux[cpu_id]) -
          (user[cpu_id] + nice[cpu_id] + system[cpu_id]));
      den_value =
          ((user_aux[cpu_id] + nice_aux[cpu_id] + system_aux[cpu_id] +
              idle_aux[cpu_id]) - (user[cpu_id] + nice[cpu_id] +
              system[cpu_id] + idle[cpu_id]));
      cpu_load[cpu_id] = 100 * (num_value / den_value);
    }
    cpu_array_sel = 0;
  }

  (void) ret;

  cpu_usage->cpu_array_sel = cpu_array_sel;
  fclose (fd);
}
