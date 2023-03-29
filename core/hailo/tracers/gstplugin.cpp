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

/* GstShark license */
#define GST_SHARK_LICENSE "LGPL"
#define PACKAGE "gst-shark"
#define PACKAGE_NAME "GstShark"
#define PACKAGE_URL "https://github.com/RidgeRun/gst-shark"
#define VERSION "0.7.2.1"

#include <gst/gst.h>
#include <glib/gstdio.h>
#include <unistd.h>
#include "gstgraphic.hpp"
#include "gstcpuusage.hpp"
#include "gstthreadmonitor.hpp"
#include "gstnumerator.hpp"
#include "gstdetections.hpp"
#include "gstproctime.hpp"
#include "gstinterlatency.hpp"
#include "gstscheduletime.hpp"
#include "gstframerate.hpp"
#include "gstqueuelevel.hpp"
#include "gstbitrate.hpp"
#include "gstbuffer.hpp"
#include "gstctf.hpp"

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_tracer_register (plugin, "cpuusage",
          gst_cpu_usage_tracer_get_type ())) {
    return FALSE;
  }
  if (!gst_tracer_register (plugin, "threadmonitor",
          gst_thread_monitor_tracer_get_type ())) {
    return FALSE;
  }
  if (!gst_tracer_register (plugin, "graphic", gst_graphic_tracer_get_type ())) {
    return FALSE;
  }
  if (!gst_tracer_register (plugin, "proctime",
          gst_proc_time_tracer_get_type ())) {
    return FALSE;
  }
  if (!gst_tracer_register (plugin, "interlatency",
          gst_interlatency_tracer_get_type ())) {
    return FALSE;
  }
  if (!gst_tracer_register (plugin, "scheduletime",
          gst_scheduletime_tracer_get_type ())) {
    return FALSE;
  }
  if (!gst_tracer_register (plugin, "framerate",
          gst_framerate_tracer_get_type ())) {
    return FALSE;
  }
  if (!gst_tracer_register (plugin, "queuelevel",
          gst_queue_level_tracer_get_type ())) {
    return FALSE;
  }
  if (!gst_tracer_register (plugin, "bitrate", gst_bitrate_tracer_get_type ())) {
    return FALSE;
  }
  if (!gst_tracer_register (plugin, "buffer", gst_buffer_tracer_get_type ())) {
    return FALSE;
  }
  if (!gst_tracer_register (plugin, "numerator", gst_numerator_tracer_get_type ())) {
    return FALSE;
  }
  if (!gst_tracer_register (plugin, "detections", gst_detections_tracer_get_type ())) {
    return FALSE;
  }
  if (!gst_ctf_init ()) {
    return FALSE;
  }

  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR,
    hailotracers, "GstShark tracers", plugin_init, VERSION,
    GST_SHARK_LICENSE, PACKAGE_NAME, PACKAGE_URL);
