/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once

#include <gst/base/gstbasetransform.h>
#include "hailo_objects.hpp"
#include "export/encode_json.hpp"
#include <cstdio>
#include <zmq.hpp>

G_BEGIN_DECLS

#define GST_TYPE_HAILO_EXPORT_ZMQ (gst_hailoexportzmq_get_type())
#define GST_HAILO_EXPORT_ZMQ(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_EXPORT_ZMQ, GstHailoExportZMQ))
#define GST_HAILO_EXPORT_ZMQ_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_EXPORT_ZMQ, GstHailoExportZMQClass))
#define GST_IS_HAILO_EXPORT_ZMQ(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_EXPORT_ZMQ))
#define GST_IS_HAILO_EXPORT_ZMQ_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_EXPORT_ZMQ))

typedef struct _GstHailoExportZMQ GstHailoExportZMQ;
typedef struct _GstHailoExportZMQClass GstHailoExportZMQClass;

struct _GstHailoExportZMQ
{
    GstBaseTransform base_hailoexportzmq;
    gchar *address;
    uint buffer_offset;
    zmq::context_t *context;
    zmq::socket_t *socket;
};

struct _GstHailoExportZMQClass
{
    GstBaseTransformClass base_hailoexportzmq_class;
};

GType gst_hailoexportzmq_get_type(void);

G_END_DECLS