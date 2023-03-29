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
#include "config.h"
#endif

#include <gst/gst.h>
#include <glib/gstdio.h>
#include "gstthreadmonitor.hpp"
#include "gstthreadmonitorcompute.hpp"
#include "gstctf.hpp"

#include <unistd.h>
#include <string.h>
#include <string>
#include <vector>

std::vector<std::string> blacklist_thread_names = {"gst-launch-1.0", "qtdemux0:sink", "gmain", "typefind:sink", "pool-gst-launch"};

void gst_thread_monitor_init(GstThreadMonitor *thread_monitor)
{
  char *strip_line = NULL;
  gint num_columns = 0;
  char *columns = NULL;
  gchar *token;
  gchar *colums_command;
  FILE *fp;
  char **tokens;
  gint thread_name_loc;
  gint thread_cpu_usage_loc;
  gint thread_memory_usage_loc;
  size_t len = 0;
  char *line = NULL;

  g_return_if_fail(thread_monitor);
  memset(thread_monitor, 0, sizeof(GstThreadMonitor));

  g_return_if_fail(thread_monitor);

  colums_command = g_strdup_printf("top -b -H-p %d -n 1 | grep PID | tr -s ' ' | sed -e 's/\x1b\[[0-9;]*m//g'", getpid());

  fp = popen(colums_command, "r");

  if (fp == NULL)
  {
    GST_WARNING("Failed to run command");
    return;
  }
  if (getline(&line, &len, fp) != -1)
  {
    strip_line = g_strstrip(line);
    columns = (char *)malloc(strlen(strip_line) + 1);
    strcpy(columns, strip_line);
    token = strtok(strip_line, " ");
    while (token != NULL)
    {
      num_columns++;
      token = strtok(NULL, " ");
    }
  }
  tokens = g_strsplit(columns, " ", num_columns);

  thread_name_loc = -1;
  thread_cpu_usage_loc = -1;
  thread_memory_usage_loc = -1;
  for (int i = 0; i < num_columns; i++)
  {
    if (strcmp(tokens[i], "COMMAND") == 0)
    {
      thread_name_loc = i;
    }
    else if (strcmp(tokens[i], "%CPU") == 0)
    {
      thread_cpu_usage_loc = i;
    }
    else if (strcmp(tokens[i], "%MEM") == 0)
    {
      thread_memory_usage_loc = i;
    }
  }
  if (thread_name_loc == -1 || thread_cpu_usage_loc == -1 || thread_memory_usage_loc == -1)
  {
    GST_WARNING("Failed to find columns");
    return;
  }
  // save columns locations and add 1 to each location because it it will be in use in awk, and awk starts at 1
  thread_monitor->thread_name_loc = thread_name_loc + 1;
  thread_monitor->thread_cpu_usage_loc = thread_cpu_usage_loc + 1;
  thread_monitor->thread_memory_usage_loc = thread_memory_usage_loc + 1;
  free(columns);
  pclose(fp);
  g_free(colums_command);
}

void gst_thread_monitor_compute(GstTracerRecord *tr_threadmonitor, GstThreadMonitor *thread_monitor, gchar **thread_name, gchar **thread_cpu_usage,
                                gchar **thread_memory_usage)
{
  FILE *fp;
  gchar *command;
  char **tokens;

  size_t len = 0;
  char *line = NULL;
  ssize_t read;

  command = g_strdup_printf("top -H -b -p %d -n 1 | sed -n '/PID/,/^$/p' | tail -n +2 | tr -s ' ' | sed -e 's/\x1b\[[0-9;]*m//g' | awk '{print $%d,$%d,$%d}'", getpid(), thread_monitor->thread_name_loc, thread_monitor->thread_cpu_usage_loc, thread_monitor->thread_memory_usage_loc);

  fp = popen(command, "r");
  if (fp == NULL)
  {
    GST_WARNING("Failed to run command");
    return;
  }

  while ((read = getline(&line, &len, fp)) != -1)
  {
    tokens = g_strsplit(line, " ", 3);
    *thread_name = tokens[0];
    *thread_cpu_usage = tokens[1];
    *thread_memory_usage = tokens[2];

    bool blacklist = false;

    for (std::string name : blacklist_thread_names)
    {
      if (strcmp(*thread_name, name.c_str()) == 0)
      {
        blacklist = true;
        break;
      }
    }

    if (!blacklist)
    {
      gst_tracer_record_log(tr_threadmonitor, *thread_name, atof(*thread_cpu_usage), atof(*thread_memory_usage));
    }
    line = NULL;
    len = 0;
  }

  pclose(fp);
  g_free(command);
}
