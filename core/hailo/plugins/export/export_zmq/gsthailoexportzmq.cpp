/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include "gsthailoexportzmq.hpp"
#include "gst_hailo_meta.hpp"
#include <chrono>
#include <cstdio>
#include <ctime>
#include <gst/video/video.h>
#include <gst/gst.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/error/en.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"

GST_DEBUG_CATEGORY_STATIC(gst_hailoexportzmq_debug_category);
#define GST_CAT_DEFAULT gst_hailoexportzmq_debug_category

/* prototypes */

static void gst_hailoexportzmq_set_property(GObject *object,
                                          guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_hailoexportzmq_get_property(GObject *object,
                                          guint property_id, GValue *value, GParamSpec *pspec);
static void gst_hailoexportzmq_dispose(GObject *object);
static void gst_hailoexportzmq_finalize(GObject *object);

static gboolean gst_hailoexportzmq_start(GstBaseTransform *trans);
static gboolean gst_hailoexportzmq_stop(GstBaseTransform *trans);
static GstFlowReturn gst_hailoexportzmq_transform_ip(GstBaseTransform *trans,
                                                   GstBuffer *buffer);

/* class initialization */

G_DEFINE_TYPE_WITH_CODE(GstHailoExportZMQ, gst_hailoexportzmq, GST_TYPE_BASE_TRANSFORM,
                        GST_DEBUG_CATEGORY_INIT(gst_hailoexportzmq_debug_category, "hailoexportzmq", 0,
                                                "debug category for hailoexportzmq element"));

enum
{
    PROP_0,
    PROP_ADDRESS,
};

static void
gst_hailoexportzmq_class_init(GstHailoExportZMQClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);

    const char *description = "Exports HailoObjects in JSON format to a ZMQ socket."
                              "\n\t\t\t   "
                              "Encodes classes contained by HailoROI objects to JSON.";
    /* Setting up pads and setting metadata should be moved to
       base_class_init if you intend to subclass this class. */
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                                            gst_caps_new_any()));
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                                            gst_caps_new_any()));

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                          "hailoexportzmq - export element",
                                          "Hailo/Tools",
                                          description,
                                          "hailo.ai <contact@hailo.ai>");

    gobject_class->set_property = gst_hailoexportzmq_set_property;
    gobject_class->get_property = gst_hailoexportzmq_get_property;
    g_object_class_install_property(gobject_class, PROP_ADDRESS,
                                    g_param_spec_string("address", "Endpoint address.",
                                                        "Address to bind the socket to.", "tcp://*:5555",
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));

    gobject_class->dispose = gst_hailoexportzmq_dispose;
    gobject_class->finalize = gst_hailoexportzmq_finalize;
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_hailoexportzmq_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_hailoexportzmq_stop);
    base_transform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_hailoexportzmq_transform_ip);
}

static void
gst_hailoexportzmq_init(GstHailoExportZMQ *hailoexportzmq)
{
    hailoexportzmq->address = g_strdup("tcp://*:5555");
    hailoexportzmq->buffer_offset = 0;
}

void gst_hailoexportzmq_set_property(GObject *object, guint property_id,
                                   const GValue *value, GParamSpec *pspec)
{
    GstHailoExportZMQ *hailoexportzmq = GST_HAILO_EXPORT_ZMQ(object);

    GST_DEBUG_OBJECT(hailoexportzmq, "set_property");

    switch (property_id)
    {
    case PROP_ADDRESS:
        hailoexportzmq->address = g_strdup(g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailoexportzmq_get_property(GObject *object, guint property_id,
                                   GValue *value, GParamSpec *pspec)
{
    GstHailoExportZMQ *hailoexportzmq = GST_HAILO_EXPORT_ZMQ(object);

    GST_DEBUG_OBJECT(hailoexportzmq, "get_property");

    switch (property_id)
    {
    case PROP_ADDRESS:
        g_value_set_string(value, hailoexportzmq->address);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailoexportzmq_dispose(GObject *object)
{
    GstHailoExportZMQ *hailoexportzmq = GST_HAILO_EXPORT_ZMQ(object);
    GST_DEBUG_OBJECT(hailoexportzmq, "dispose");

    /* clean up as possible.  may be called multiple times */

    G_OBJECT_CLASS(gst_hailoexportzmq_parent_class)->dispose(object);
}

void gst_hailoexportzmq_finalize(GObject *object)
{
    GstHailoExportZMQ *hailoexportzmq = GST_HAILO_EXPORT_ZMQ(object);
    GST_DEBUG_OBJECT(hailoexportzmq, "finalize");

    /* clean up object here */

    G_OBJECT_CLASS(gst_hailoexportzmq_parent_class)->finalize(object);
}

static gboolean
gst_hailoexportzmq_start(GstBaseTransform *trans)
{
    GstHailoExportZMQ *hailoexportzmq = GST_HAILO_EXPORT_ZMQ(trans);
    GST_DEBUG_OBJECT(hailoexportzmq, "start");

    // Prepare the context and socket
    hailoexportzmq->context = new zmq::context_t(1);
    hailoexportzmq->socket = new zmq::socket_t(*(hailoexportzmq->context), ZMQ_PUB);

    // Bind the socket to the requested address
    hailoexportzmq->socket->bind(hailoexportzmq->address);

    return TRUE;
}

static gboolean
gst_hailoexportzmq_stop(GstBaseTransform *trans)
{
    GstHailoExportZMQ *hailoexportzmq = GST_HAILO_EXPORT_ZMQ(trans);
    GST_DEBUG_OBJECT(hailoexportzmq, "stop");

    // Unbind the socket and close the context
    hailoexportzmq->socket->close();
    hailoexportzmq->context->close();

    return TRUE;
}

static GstFlowReturn
gst_hailoexportzmq_transform_ip(GstBaseTransform *trans,
                                 GstBuffer *buffer)
{
    GstHailoExportZMQ *hailoexportzmq = GST_HAILO_EXPORT_ZMQ(trans);

    // Get the roi from the current buffer and encode it to a JSON entry
    HailoROIPtr hailo_roi = get_hailo_main_roi(buffer, true);
    rapidjson::Document encoded_roi = encode_json::encode_hailo_roi(hailo_roi);

    // Add a timestamp
    auto timenow = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    encoded_roi.AddMember("timestamp (ms)", rapidjson::Value(timenow), encoded_roi.GetAllocator());
    encoded_roi.AddMember("buffer_offset", rapidjson::Value(hailoexportzmq->buffer_offset), encoded_roi.GetAllocator());

    // Get the buffer of the json
    rapidjson::StringBuffer json_buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(json_buffer);
    encoded_roi.Accept(writer);

    // Send the message
    zmq::message_t json_message(json_buffer.GetSize());
    // Copy is required since zmq::message_t would only wrap the data, so if the buffer is freed/overwritten
    // while the message is sending you will get garbage data or a segfault.
    std::memcpy(json_message.data(), json_buffer.GetString(), json_buffer.GetSize()); 
#if (CPPZMQ_VERSION_MAJOR >= 4 && CPPZMQ_VERSION_MINOR >= 6 && CPPZMQ_VERSION_PATCH >= 0)
    zmq::send_result_t result = hailoexportzmq->socket->send(json_message, zmq::send_flags(ZMQ_DONTWAIT));
#else
    zmq::detail::send_result_t result = hailoexportzmq->socket->send(json_message, zmq::send_flags(ZMQ_DONTWAIT));
#endif
    if (result != json_message.size())
        GST_WARNING("hailoexportzmq failed to send buffer!");

    hailoexportzmq->buffer_offset++;
    GST_DEBUG_OBJECT(hailoexportzmq, "transform_ip");
    return GST_FLOW_OK;
}
