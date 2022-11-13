/* GstShark - A Front End for GstTracer
 * Copyright (C) 2018 RidgeRun Engineering <manuel.leiva@ridgerun.com>
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
 * SECTION:gstthreadmonitor
 * @short_description: log cpu usage stats
 *
 * A tracing module that take threadmonitor() snapshots and logs them.
 */

#include <glib/gstdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include "gstthreadmonitor.hpp"
#include "gstthreadmonitorcompute.hpp"
#include "gstctf.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_thread_monitor_debug);
#define GST_CAT_DEFAULT gst_thread_monitor_debug

struct _GstThreadMonitorTracer
{
  GstPeriodicTracer parent;
  GstThreadMonitor thread_monitor;
  int pid;
};

#define _do_init \
  GST_DEBUG_CATEGORY_INIT(gst_thread_monitor_debug, "threadmonitor", 0, "threadmonitor tracer");

G_DEFINE_TYPE_WITH_CODE(GstThreadMonitorTracer, gst_thread_monitor_tracer,
                        GST_TYPE_PERIODIC_TRACER, _do_init);

static GstTracerRecord *tr_threadmonitor;

static void threadmonitor_dummy_bin_add_post(GObject *obj, GstClockTime ts,
                                             GstBin *bin, GstElement *element, gboolean result);
static gboolean thread_monitor_thread_func(GstPeriodicTracer *tracer);

static void
threadmonitor_dummy_bin_add_post(GObject *obj, GstClockTime ts,
                                 GstBin *bin, GstElement *element, gboolean result)
{
  return;
}

static gboolean
thread_monitor_thread_func(GstPeriodicTracer *tracer)
{
  GstThreadMonitorTracer *self;
  GstThreadMonitor *thread_monitor;

  gchar *thread_name = NULL;

  gchar *thread_cpu_usage = NULL;
  gchar *thread_memory_usage = NULL;

  self = GST_THREAD_MONITOR_TRACER(tracer);
  thread_monitor = &self->thread_monitor;
  gst_thread_monitor_compute(tr_threadmonitor, thread_monitor, &thread_name, &thread_cpu_usage, &thread_memory_usage);


  return TRUE;
}

static void
gst_thread_monitor_tracer_class_init(GstThreadMonitorTracerClass *klass)
{
  GstPeriodicTracerClass *tracer_class;

  tracer_class = GST_PERIODIC_TRACER_CLASS(klass);

  tracer_class->timer_callback = GST_DEBUG_FUNCPTR(thread_monitor_thread_func);

  tr_threadmonitor = gst_tracer_record_new("threadmonitor.class",
                                           "name", GST_TYPE_STRUCTURE,
                                           gst_structure_new("value", "type", G_TYPE_GTYPE, G_TYPE_STRING,
                                                             "description", G_TYPE_STRING,
                                                             "thread name", "flags", GST_TYPE_TRACER_VALUE_FLAGS,
                                                             GST_TRACER_VALUE_FLAGS_AGGREGATED, NULL),
                                           "cpu_usage",
                                           GST_TYPE_STRUCTURE,
                                           gst_structure_new("value", "type", G_TYPE_GTYPE, G_TYPE_DOUBLE,
                                                             "description", G_TYPE_STRING, "CPU usage of thread percentage [%]", "flags",
                                                             GST_TYPE_TRACER_VALUE_FLAGS, GST_TRACER_VALUE_FLAGS_AGGREGATED, "min",
                                                             G_TYPE_DOUBLE, 0.0f, "max", G_TYPE_DOUBLE, 100.0f, NULL),

                                           "memory_usage",
                                           GST_TYPE_STRUCTURE,
                                           gst_structure_new("value", "type", G_TYPE_GTYPE, G_TYPE_DOUBLE,
                                                             "description", G_TYPE_STRING, "Memory usage of thread percentage [%]", "flags",
                                                             GST_TYPE_TRACER_VALUE_FLAGS, GST_TRACER_VALUE_FLAGS_AGGREGATED, "min",
                                                             G_TYPE_DOUBLE, 0.0f, "max", G_TYPE_DOUBLE, 100.0f, NULL),
                                           NULL);
}

static void
gst_thread_monitor_tracer_init(GstThreadMonitorTracer *self)
{

  GstThreadMonitor *thread_monitor;
  thread_monitor = &self->thread_monitor;
  gst_thread_monitor_init(thread_monitor);

  /* Register a dummy hook so that the tracer remains alive */
  gst_tracing_register_hook(GST_TRACER(self), "bin-add-post",
                            G_CALLBACK(threadmonitor_dummy_bin_add_post));
}
