/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#pragma once

#include "gstperiodictracer.hpp"

G_BEGIN_DECLS

#define GST_TYPE_THREAD_MONITOR_TRACER (gst_thread_monitor_tracer_get_type())
G_DECLARE_FINAL_TYPE (GstThreadMonitorTracer, gst_thread_monitor_tracer, GST, THREAD_MONITOR_TRACER, GstPeriodicTracer)

G_END_DECLS