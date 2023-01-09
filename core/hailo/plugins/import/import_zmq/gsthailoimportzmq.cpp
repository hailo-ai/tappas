/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "gsthailoimportzmq.hpp"
#pragma GCC diagnostic pop
#include "gst_hailo_meta.hpp"
#include <chrono>
#include <cstdio>
#include <ctime>
#include <gst/video/video.h>
#include <gst/gst.h>
#include <unistd.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"

GST_DEBUG_CATEGORY_STATIC(gst_hailoimportzmq_debug_category);
#define GST_CAT_DEFAULT gst_hailoimportzmq_debug_category

/* prototypes */

static void gst_hailoimportzmq_set_property(GObject *object,
                                            guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_hailoimportzmq_get_property(GObject *object,
                                            guint property_id, GValue *value, GParamSpec *pspec);
static void gst_hailoimportzmq_dispose(GObject *object);
static void gst_hailoimportzmq_finalize(GObject *object);

static gboolean gst_hailoimportzmq_start(GstBaseTransform *trans);
static gboolean gst_hailoimportzmq_stop(GstBaseTransform *trans);
static GstFlowReturn gst_hailoimportzmq_transform_ip(GstBaseTransform *trans,
                                                     GstBuffer *buffer);

/* class initialization */

G_DEFINE_TYPE_WITH_CODE(GstHailoImportZMQ, gst_hailoimportzmq, GST_TYPE_BASE_TRANSFORM,
                        GST_DEBUG_CATEGORY_INIT(gst_hailoimportzmq_debug_category, "hailoimportzmq", 0,
                                                "debug category for hailoimportzmq element"));

enum
{
    PROP_0,
    PROP_ADDRESS,
};

// Default import node
const gchar *DEFAULT_ADDRESS = "tcp://localhost:5555";

static void
gst_hailoimportzmq_class_init(GstHailoImportZMQClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);

    const char *description = "Imports HailoObjects in JSON format from a ZMQ socket."
                              "\n\t\t\t   "
                              "Decodes classes contained by JSON to HailoROI objects.";
    /* Setting up pads and setting metadata should be moved to
       base_class_init if you intend to subclass this class. */
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                                            gst_caps_new_any()));
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                                            gst_caps_new_any()));

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                          "hailoimportzmq - import element",
                                          "Hailo/Tools",
                                          description,
                                          "hailo.ai <contact@hailo.ai>");

    gobject_class->set_property = gst_hailoimportzmq_set_property;
    gobject_class->get_property = gst_hailoimportzmq_get_property;
    g_object_class_install_property(gobject_class, PROP_ADDRESS,
                                    g_param_spec_string("address", "Endpoint address.",
                                                        "Address to bind the socket to.", "tcp://localhost:5555",
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));

    gobject_class->dispose = gst_hailoimportzmq_dispose;
    gobject_class->finalize = gst_hailoimportzmq_finalize;
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_hailoimportzmq_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_hailoimportzmq_stop);
    base_transform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_hailoimportzmq_transform_ip);
}

static void
gst_hailoimportzmq_init(GstHailoImportZMQ *hailoimportzmq)
{
    hailoimportzmq->address = g_strdup(DEFAULT_ADDRESS);
}

void gst_hailoimportzmq_set_property(GObject *object, guint property_id,
                                     const GValue *value, GParamSpec *pspec)
{
    GstHailoImportZMQ *hailoimportzmq = GST_HAILO_IMPORT_ZMQ(object);

    GST_DEBUG_OBJECT(hailoimportzmq, "set_property");

    switch (property_id)
    {
    case PROP_ADDRESS:
        hailoimportzmq->address = g_strdup(g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailoimportzmq_get_property(GObject *object, guint property_id,
                                     GValue *value, GParamSpec *pspec)
{
    GstHailoImportZMQ *hailoimportzmq = GST_HAILO_IMPORT_ZMQ(object);

    GST_DEBUG_OBJECT(hailoimportzmq, "get_property");

    switch (property_id)
    {
    case PROP_ADDRESS:
        g_value_set_string(value, hailoimportzmq->address);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailoimportzmq_dispose(GObject *object)
{
    GstHailoImportZMQ *hailoimportzmq = GST_HAILO_IMPORT_ZMQ(object);
    GST_DEBUG_OBJECT(hailoimportzmq, "dispose");

    /* clean up as possible.  may be called multiple times */

    G_OBJECT_CLASS(gst_hailoimportzmq_parent_class)->dispose(object);
}

void gst_hailoimportzmq_finalize(GObject *object)
{
    GstHailoImportZMQ *hailoimportzmq = GST_HAILO_IMPORT_ZMQ(object);
    GST_DEBUG_OBJECT(hailoimportzmq, "finalize");

    /* clean up object here */

    G_OBJECT_CLASS(gst_hailoimportzmq_parent_class)->finalize(object);
}

static gboolean
gst_hailoimportzmq_start(GstBaseTransform *trans)
{
    GstHailoImportZMQ *hailoimportzmq = GST_HAILO_IMPORT_ZMQ(trans);
    GST_DEBUG_OBJECT(hailoimportzmq, "start");
    // Prepare the context and socket
    hailoimportzmq->context = new zmq::context_t(1);
    hailoimportzmq->socket = new zmq::socket_t(*(hailoimportzmq->context), ZMQ_SUB);

    // Bind the socket to the requested address
    hailoimportzmq->socket->setsockopt(ZMQ_SUBSCRIBE, "", 0);
    try {
        hailoimportzmq->socket->connect(hailoimportzmq->address);
    }
    catch (zmq::error_t const& err)
    {
        GST_ERROR("hailoimportzmq failed to connect to socket at address %s! Error: %s",
                  hailoimportzmq->address,
                  err.what());
        return FALSE;
    }

    return TRUE;
}

static gboolean
gst_hailoimportzmq_stop(GstBaseTransform *trans)
{
    GstHailoImportZMQ *hailoimportzmq = GST_HAILO_IMPORT_ZMQ(trans);
    GST_DEBUG_OBJECT(hailoimportzmq, "stop");

    // Unbind the socket and close the context
    hailoimportzmq->socket->close();
    hailoimportzmq->context->close();

    return TRUE;
}

static GstFlowReturn
gst_hailoimportzmq_transform_ip(GstBaseTransform *trans,
                                GstBuffer *buffer)
{
    GstHailoImportZMQ *hailoimportzmq = GST_HAILO_IMPORT_ZMQ(trans);

    // Get the roi from the current buffer and decode it to a JSON entry
    HailoROIPtr hailo_roi = get_hailo_main_roi(buffer, true);

    // Recv the message
    zmq::message_t recv_message;
#if (CPPZMQ_VERSION_MAJOR >= 4 && CPPZMQ_VERSION_MINOR >= 6 && CPPZMQ_VERSION_PATCH >= 0)
    zmq::recv_result_t recv_succeeded = 0;
#else
    zmq::detail::recv_result_t recv_succeeded = 0;
#endif
    while (recv_message.size() == 0)
    {
        recv_succeeded = hailoimportzmq->socket->recv(recv_message, zmq::recv_flags(ZMQ_DONTWAIT));
        sleep(0);  // yield the scheduler to prevent the thread from spinning the CPU core
    }
    if (recv_succeeded <= 0)
        GST_WARNING("hailoimportzmq failed to send buffer!");

    // Decode the recvd JSON
    std::string rx_str;
    rx_str.assign(static_cast<char *>(recv_message.data()), recv_message.size());
    rapidjson::Document decoded_stream;
    if (decoded_stream.Parse(rx_str).HasParseError()) {
        GST_ERROR("hailoimportzmq failed to parse message to json!");
    }
    decode_json::decode_hailo_roi(decoded_stream, hailo_roi);

    GST_DEBUG_OBJECT(hailoimportzmq, "transform_ip");
    return GST_FLOW_OK;
}
