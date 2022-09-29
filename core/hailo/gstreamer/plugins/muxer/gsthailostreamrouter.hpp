/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/*
 * GStreamer STREAM_ROUTER element
 *
 *
 * gsthailostreamrouter.hpp: Simple STREAM_ROUTER
 */

#pragma once

#include <gst/gst.h>
#include <vector>

G_BEGIN_DECLS


// Define HailoStreamRouter type
#define GST_TYPE_HAILO_STREAM_ROUTER \
  (gst_hailo_stream_router_get_type())
#define GST_HAILO_STREAM_ROUTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_STREAM_ROUTER, GstHailoStreamRouter))
#define GST_HAILO_STREAM_ROUTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_STREAM_ROUTER, GstHailoStreamRouterClass))
#define GST_IS_HAILO_STREAM_ROUTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_STREAM_ROUTER))
#define GST_IS_HAILO_STREAM_ROUTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_STREAM_ROUTER))
#define GST_HAILO_STREAM_ROUTER_CAST(obj) ((GstHailoStreamRouter *)(obj))

typedef struct _GstHailoStreamRouter GstHailoStreamRouter;
typedef struct _GstHailoStreamRouterClass GstHailoStreamRouterClass;

/**
 * GstHailoStreamRouter:
 *
 * #GstHailoStreamRouter.
 */

struct _GstHailoStreamRouter
{
  GstElement element;

  GstPad *sinkpad;
  GMutex lock;
  GHashTable *targets_table;
};

struct _GstHailoStreamRouterClass
{
  GstElementClass parent_class;
};

G_GNUC_INTERNAL GType gst_hailo_stream_router_get_type(void);

G_END_DECLS