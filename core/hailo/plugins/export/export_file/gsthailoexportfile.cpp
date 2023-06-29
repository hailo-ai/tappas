/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include "gsthailoexportfile.hpp"
#include "gst_hailo_meta.hpp"
#include "hailo/hailort.h"
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

GST_DEBUG_CATEGORY_STATIC(gst_hailoexportfile_debug_category);
#define GST_CAT_DEFAULT gst_hailoexportfile_debug_category

/* prototypes */

static void gst_hailoexportfile_set_property(GObject *object,
                                          guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_hailoexportfile_get_property(GObject *object,
                                          guint property_id, GValue *value, GParamSpec *pspec);
static void gst_hailoexportfile_dispose(GObject *object);
static void gst_hailoexportfile_finalize(GObject *object);

static gboolean gst_hailoexportfile_start(GstBaseTransform *trans);
static gboolean gst_hailoexportfile_stop(GstBaseTransform *trans);
static GstFlowReturn gst_hailoexportfile_transform_ip(GstBaseTransform *trans,
                                                   GstBuffer *buffer);

/* class initialization */

G_DEFINE_TYPE_WITH_CODE(GstHailoExportFile, gst_hailoexportfile, GST_TYPE_BASE_TRANSFORM,
                        GST_DEBUG_CATEGORY_INIT(gst_hailoexportfile_debug_category, "hailoexportfile", 0,
                                                "debug category for hailoexportfile element"));

enum
{
    PROP_0,
    PROP_FIlE_PATH,
};

static void
gst_hailoexportfile_class_init(GstHailoExportFileClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);

    const char *description = "Exports HailoObjects in JSON format to a file."
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
                                          "hailoexportfile - export element",
                                          "Hailo/Tools",
                                          description,
                                          "hailo.ai <contact@hailo.ai>");

    gobject_class->set_property = gst_hailoexportfile_set_property;
    gobject_class->get_property = gst_hailoexportfile_get_property;
    g_object_class_install_property(gobject_class, PROP_FIlE_PATH,
                                    g_param_spec_string("location", "Path to export file.",
                                                        "Location of the JSON file to save", "hailo_meta.json",
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));

    gobject_class->dispose = gst_hailoexportfile_dispose;
    gobject_class->finalize = gst_hailoexportfile_finalize;
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_hailoexportfile_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_hailoexportfile_stop);
    base_transform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_hailoexportfile_transform_ip);
}

static void
gst_hailoexportfile_init(GstHailoExportFile *hailoexportfile)
{
    hailoexportfile->file_path = g_strdup("hailo_meta.json");
    hailoexportfile->buffer_offset = 0;
}

void gst_hailoexportfile_set_property(GObject *object, guint property_id,
                                   const GValue *value, GParamSpec *pspec)
{
    GstHailoExportFile *hailoexportfile = GST_HAILO_EXPORT_FILE(object);

    GST_DEBUG_OBJECT(hailoexportfile, "set_property");

    switch (property_id)
    {
    case PROP_FIlE_PATH:
        hailoexportfile->file_path = g_strdup(g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailoexportfile_get_property(GObject *object, guint property_id,
                                   GValue *value, GParamSpec *pspec)
{
    GstHailoExportFile *hailoexportfile = GST_HAILO_EXPORT_FILE(object);

    GST_DEBUG_OBJECT(hailoexportfile, "get_property");

    switch (property_id)
    {
    case PROP_FIlE_PATH:
        g_value_set_string(value, hailoexportfile->file_path);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailoexportfile_dispose(GObject *object)
{
    GstHailoExportFile *hailoexportfile = GST_HAILO_EXPORT_FILE(object);
    GST_DEBUG_OBJECT(hailoexportfile, "dispose");

    /* clean up as possible.  may be called multiple times */

    G_OBJECT_CLASS(gst_hailoexportfile_parent_class)->dispose(object);
}

void gst_hailoexportfile_finalize(GObject *object)
{
    GstHailoExportFile *hailoexportfile = GST_HAILO_EXPORT_FILE(object);
    GST_DEBUG_OBJECT(hailoexportfile, "finalize");

    /* clean up object here */

    G_OBJECT_CLASS(gst_hailoexportfile_parent_class)->finalize(object);
}

static gboolean
gst_hailoexportfile_start(GstBaseTransform *trans)
{
    GstHailoExportFile *hailoexportfile = GST_HAILO_EXPORT_FILE(trans);
    GST_DEBUG_OBJECT(hailoexportfile, "start");

    hailoexportfile->json_file = fopen(hailoexportfile->file_path, "w");
    fputs("[]", hailoexportfile->json_file);
    fclose(hailoexportfile->json_file);

    return TRUE;
}

static gboolean
gst_hailoexportfile_stop(GstBaseTransform *trans)
{
    GstHailoExportFile *hailoexportfile = GST_HAILO_EXPORT_FILE(trans);
    GST_DEBUG_OBJECT(hailoexportfile, "stop");

    return TRUE;
}

static GstFlowReturn
gst_hailoexportfile_transform_ip(GstBaseTransform *trans,
                                 GstBuffer *buffer)
{
    GstHailoExportFile *hailoexportfile = GST_HAILO_EXPORT_FILE(trans);

    // Get the roi from the current buffer and encode it to a JSON entry
    HailoROIPtr hailo_roi = get_hailo_main_roi(buffer, true);
    rapidjson::Document encoded_roi = encode_json::encode_hailo_roi(hailo_roi);

    // Add a timestamp
    auto timenow = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    encoded_roi.AddMember("timestamp (ms)", rapidjson::Value(timenow), encoded_roi.GetAllocator());
    encoded_roi.AddMember("buffer_offset", rapidjson::Value(hailoexportfile->buffer_offset), encoded_roi.GetAllocator());

    // Add the stream-id
    std::string stream_id = hailo_roi->get_stream_id();

    if (stream_id.length() == 0)
    {
        gchar * id = gst_pad_get_stream_id(trans->srcpad);
        stream_id = std::string(reinterpret_cast<char *>(id));
        g_free(id);
    }

    encoded_roi.AddMember("stream_id", stream_id, encoded_roi.GetAllocator() );

    // Open the file 
    hailoexportfile->json_file = fopen(hailoexportfile->file_path, "rb+");

    // Replace the "]" terminator with "," (if not empty)
    if (std::getc(hailoexportfile->json_file) == '[' && std::getc(hailoexportfile->json_file) != ']') {
        std::fseek(hailoexportfile->json_file, -1, SEEK_END);
        std::fputc(',', hailoexportfile->json_file);
    } else {
        std::fseek(hailoexportfile->json_file, -1, SEEK_END);
    }

    // Append the new entry to the document
    char writeBuffer[65536];
    rapidjson::FileWriteStream write_stream(hailoexportfile->json_file, writeBuffer, sizeof(writeBuffer));
    rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer(write_stream);
    encoded_roi.Accept(writer);

    // Close the array
    std::fputc(']', hailoexportfile->json_file);
    fclose(hailoexportfile->json_file);

    hailoexportfile->buffer_offset++;
    GST_DEBUG_OBJECT(hailoexportfile, "transform_ip");
    return GST_FLOW_OK;
}
