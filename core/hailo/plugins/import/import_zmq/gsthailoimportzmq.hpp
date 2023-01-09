/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once

#include <gst/base/gstbasetransform.h>
#include "hailo_objects.hpp"
#include "import/decode_json.hpp"
#include <cstdio>
#include <zmq.hpp>

G_BEGIN_DECLS

#define GST_TYPE_HAILO_IMPORT_ZMQ (gst_hailoimportzmq_get_type())
#define GST_HAILO_IMPORT_ZMQ(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_IMPORT_ZMQ, GstHailoImportZMQ))
#define GST_HAILO_IMPORT_ZMQ_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_IMPORT_ZMQ, GstHailoImportZMQClass))
#define GST_IS_HAILO_IMPORT_ZMQ(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_IMPORT_ZMQ))
#define GST_IS_HAILO_IMPORT_ZMQ_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_IMPORT_ZMQ))

typedef struct _GstHailoImportZMQ GstHailoImportZMQ;
typedef struct _GstHailoImportZMQClass GstHailoImportZMQClass;

struct _GstHailoImportZMQ
{
    GstBaseTransform base_hailoimportzmq;
    gchar *address;
    zmq::context_t *context;
    zmq::socket_t *socket;
};

struct _GstHailoImportZMQClass
{
    GstBaseTransformClass base_hailoimportzmq_class;
};

GType gst_hailoimportzmq_get_type(void);

G_END_DECLS