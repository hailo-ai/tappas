/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include "gsthailofilter.hpp"
#include "tensor_meta.hpp"
#include "gst_hailo_meta.hpp"
#include "hailo/hailort.h"
#include <gst/video/video.h>
#include <gst/gst.h>
#include <dlfcn.h>
#include <map>
#include <iostream>

GST_DEBUG_CATEGORY_STATIC(gst_hailofilter_debug_category);
#define GST_CAT_DEFAULT gst_hailofilter_debug_category
#define DEFAULT_FUNCTION_NAME "filter"
#define INIT_FUNC_NAME "init"
#define FREE_FUNC_NAME "free_resources"

static void gst_hailofilter_set_property(GObject *object,
                                         guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_hailofilter_get_property(GObject *object,
                                         guint property_id, GValue *value, GParamSpec *pspec);
static void gst_hailofilter_dispose(GObject *object);
static void gst_hailofilter_finalize(GObject *object);

static gboolean gst_hailofilter_start(GstBaseTransform *trans);
static gboolean gst_hailofilter_stop(GstBaseTransform *trans);
static GstFlowReturn gst_hailofilter_transform_ip(GstBaseTransform *trans,
                                                  GstBuffer *buffer);

enum
{
    PROP_0,
    PROP_PROCESS_LIB,
    PROP_PROCESS_FUNC_NAME,
    PROP_USE_GST_BUFFER,
    PROP_CONFIG_FILE_PATH,
    PROP_REMOVE_TENSORS,
};

G_DEFINE_TYPE_WITH_CODE(GstHailofilter, gst_hailofilter, GST_TYPE_BASE_TRANSFORM,
                        GST_DEBUG_CATEGORY_INIT(gst_hailofilter_debug_category, "hailofilter", 0,
                                                "debug category for hailofilter element"));

static void
gst_hailofilter_class_init(GstHailofilterClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);

    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                                            GST_CAPS_ANY));
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                                            GST_CAPS_ANY));

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                          "hailofilter - postprocessing element", "Hailo/Tools", "Allowes to user access Hailonet's output using an so file.",
                                          "hailo.ai <contact@hailo.ai>");

    gobject_class->set_property = gst_hailofilter_set_property;
    gobject_class->get_property = gst_hailofilter_get_property;
    g_object_class_install_property(gobject_class, PROP_PROCESS_LIB,
                                    g_param_spec_string("so-path", "process so Path Location",
                                                        "Location of the so file to load", NULL,
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
    g_object_class_install_property(gobject_class, PROP_PROCESS_FUNC_NAME,
                                    g_param_spec_string("function-name", "Name of function in the so file",
                                                        "function-name", "filter",
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
    g_object_class_install_property(gobject_class, PROP_USE_GST_BUFFER,
                                    g_param_spec_boolean("use-gst-buffer", "use-gst-buffer", "use function with access to the Gst Buffer", false,
                                                         (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_CONFIG_FILE_PATH,
                                    g_param_spec_string("config-path", "config-path",
                                                        "json config file path", NULL,
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
    g_object_class_install_property(gobject_class, PROP_REMOVE_TENSORS,
                                    g_param_spec_boolean("remove-tensors", "remove-tensors", "whether hailofilter should delete tensors at the end", true,
                                                         (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gobject_class->dispose = gst_hailofilter_dispose;
    gobject_class->finalize = gst_hailofilter_finalize;
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_hailofilter_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_hailofilter_stop);
    base_transform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_hailofilter_transform_ip);
}

static void
gst_hailofilter_init(GstHailofilter *hailofilter)
{
    hailofilter->use_config = true;
    hailofilter->remove_tensors = true;
    hailofilter->params = nullptr;
    hailofilter->config_path = g_strdup("NULL");
}

void gst_hailofilter_set_property(GObject *object, guint property_id,
                                  const GValue *value, GParamSpec *pspec)
{
    GstHailofilter *hailofilter = GST_HAILO_FILTER(object);

    GST_DEBUG_OBJECT(hailofilter, "set_property");

    switch (property_id)
    {
    case PROP_PROCESS_LIB:
        hailofilter->lib_path = g_strdup(g_value_get_string(value));
        break;
    case PROP_PROCESS_FUNC_NAME:
        hailofilter->function_name = g_strdup(g_value_get_string(value));
        break;
    case PROP_USE_GST_BUFFER:
        hailofilter->use_gst_buffer = g_value_get_boolean(value);
        break;
    case PROP_CONFIG_FILE_PATH:
        hailofilter->config_path = g_strdup(g_value_get_string(value));
        break;
    case PROP_REMOVE_TENSORS:
        hailofilter->remove_tensors = g_value_get_boolean(value);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailofilter_get_property(GObject *object, guint property_id,
                                  GValue *value, GParamSpec *pspec)
{
    GstHailofilter *hailofilter = GST_HAILO_FILTER(object);

    GST_DEBUG_OBJECT(hailofilter, "get_property");

    switch (property_id)
    {
    case PROP_PROCESS_LIB:
        g_value_set_string(value, hailofilter->lib_path);
        break;
    case PROP_PROCESS_FUNC_NAME:
        g_value_set_string(value, hailofilter->function_name);
        break;
    case PROP_USE_GST_BUFFER:
        g_value_set_boolean(value, hailofilter->use_gst_buffer);
        break;
    case PROP_CONFIG_FILE_PATH:
        g_value_set_string(value, hailofilter->config_path);
        break;
    case PROP_REMOVE_TENSORS:
        g_value_set_boolean(value, hailofilter->remove_tensors);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}
void gst_hailofilter_dispose(GObject *object)
{
    GstHailofilter *hailofilter = GST_HAILO_FILTER(object);
    if (hailofilter->params != nullptr)
    {
        auto delete_func = (void (*)(void *))dlsym(hailofilter->loaded_lib, FREE_FUNC_NAME);
        delete_func(hailofilter->params);
    }
    if (hailofilter->loaded_lib != nullptr)
    {
        dlclose(hailofilter->loaded_lib);
    }

    GST_DEBUG_OBJECT(hailofilter, "dispose");

    /* clean up as possible.  may be called multiple times */

    G_OBJECT_CLASS(gst_hailofilter_parent_class)->dispose(object);
}

void gst_hailofilter_finalize(GObject *object)
{
    GstHailofilter *hailofilter = GST_HAILO_FILTER(object);

    GST_DEBUG_OBJECT(hailofilter, "finalize");

    /* clean up object here */

    G_OBJECT_CLASS(gst_hailofilter_parent_class)->finalize(object);
}

static gboolean gst_hailofilter_start(GstBaseTransform *trans)
{
    GstHailofilter *hailofilter = GST_HAILO_FILTER(trans);
    // Load the give SO using dlopen.
    hailofilter->loaded_lib = dlopen(hailofilter->lib_path, RTLD_LAZY);
    if (!hailofilter->loaded_lib)
    {
        std::cerr << "Could not load lib " << dlerror() << std::endl;
    }
    // reset errors
    dlerror();

    // Use the default function name if no name was provided.
    if (hailofilter->function_name == nullptr)
    {
        hailofilter->function_name = g_strdup(DEFAULT_FUNCTION_NAME);
    }

    // Create handler for all function-names on the list.

    auto init_func = (void *(*)(std::string, std::string))dlsym(hailofilter->loaded_lib, INIT_FUNC_NAME);

    if (init_func == nullptr)
    {
        hailofilter->use_config = false;

        if (hailofilter->use_gst_buffer)
        {
            /*
            if use_gst_buffer, the function should have three arguments (HailoROIPtr, GstVideoFrame*, and gchar*),
            and therefore will be able to change the buffer data.
            */
            hailofilter->handler_gst_no_config = (void (*)(HailoROIPtr, GstVideoFrame *))dlsym(hailofilter->loaded_lib, hailofilter->function_name);
        }
        else
        {
            /*
            in this case the funtion doesn't get GstVideoFrame* as argument therefore it can't modify the buffer data.
            */
            hailofilter->handler_no_config = (void (*)(HailoROIPtr))dlsym(hailofilter->loaded_lib, hailofilter->function_name);
        }
    }
    else // found init function
    {
        hailofilter->params = init_func(hailofilter->config_path, hailofilter->function_name);
        if (hailofilter->use_gst_buffer)
        {
            /*
            if use_gst_buffer, the function should have four arguments (HailoROIPtr, GstVideoFrame*, and gchar*,void *),
            and therefore will be able to change the buffer data.
            */
            hailofilter->handler_gst = (void (*)(HailoROIPtr, GstVideoFrame *, void *))dlsym(hailofilter->loaded_lib, hailofilter->function_name);
        }
        else
        {
            /*
                       in this case the funtion doesn't get GstVideoFrame* as argument therefore it can't modify the buffer data.
            */
            hailofilter->handler = (void (*)(HailoROIPtr, void *))dlsym(hailofilter->loaded_lib, hailofilter->function_name);
        }
    }
    // If there was an error loading one of the symbols, close the dl and break.
    const char *dlsym_error = dlerror();
    if (dlsym_error)
    {
        std::cerr << "Cannot load symbol: " << dlsym_error << std::endl;
        dlclose(hailofilter->loaded_lib);
    }

    GST_DEBUG_OBJECT(hailofilter, "start");

    return TRUE;
}

static gboolean
gst_hailofilter_stop(GstBaseTransform *trans)
{
    GstHailofilter *hailofilter = GST_HAILO_FILTER(trans);

    GST_DEBUG_OBJECT(hailofilter, "stop");

    return TRUE;
}

/**
 * @brief Get the tensors from meta object
 *
 * @param buffer The buffer to extract the tensor_meta from.
 * @param roi ROI to add the tensors to.
 * @note This function implementation should be changed according to HRT-5150 on the next release.
 */
void get_tensors_from_meta(GstBuffer *buffer, HailoROIPtr roi)
{
    gpointer state = NULL;
    GstMeta *meta;
    GstParentBufferMeta *pmeta;
    GstMapInfo info;
    while ((meta = gst_buffer_iterate_meta_filtered(buffer, &state, GST_PARENT_BUFFER_META_API_TYPE)))
    {
        pmeta = reinterpret_cast<GstParentBufferMeta *>(meta);
        (void)gst_buffer_map(pmeta->buffer, &info, GST_MAP_READWRITE);
        // check if the buffer has tensor metadata
        if (!gst_buffer_get_meta(pmeta->buffer, g_type_from_name(TENSOR_META_API_NAME)))
        {
            gst_buffer_unmap(pmeta->buffer, &info);
            continue;
        }
        const hailo_vstream_info_t vstream_info = reinterpret_cast<GstHailoTensorMeta *>(gst_buffer_get_meta(pmeta->buffer, g_type_from_name(TENSOR_META_API_NAME)))->info;
        roi->add_tensor(std::make_shared<HailoTensor>(reinterpret_cast<uint8_t *>(info.data), vstream_info));
        gst_buffer_unmap(pmeta->buffer, &info);
    }
}

/**
 * @brief Remove all tensors from the buffer and the ROI.
 *
 * @param buffer The buffer to remove tensors from.
 * @param roi The roi to remove tensors from.
 * @return gboolean true if all removals were successful, false otherwise.
 */
gboolean remove_tensors(GstBuffer *buffer, HailoROIPtr roi)
{
    gpointer state = NULL;
    GstMeta *meta;
    std::vector<GstMeta *> meta_vector;
    gboolean ret = false;
    roi->clear_tensors();
    while ((meta = gst_buffer_iterate_meta_filtered(buffer, &state, GST_PARENT_BUFFER_META_API_TYPE)))
    {
        meta_vector.emplace_back(std::move(meta));
    }
    for (auto meta : meta_vector)
    {
        ret = gst_buffer_remove_meta(buffer, meta);
        if (ret == false)
            return ret;
    }
    return true;
}

static GstFlowReturn gst_hailofilter_transform_ip(GstBaseTransform *trans,
                                                  GstBuffer *buffer)
{
    GstHailofilter *hailofilter = GST_HAILO_FILTER(trans);

    HailoROIPtr hailo_roi = get_hailo_main_roi(buffer, true);
    get_tensors_from_meta(buffer, hailo_roi);
    GstPad *srcpad = trans->srcpad;

    if (hailo_roi->get_stream_id().length() == 0)
    {
        gchar *id = gst_pad_get_stream_id(srcpad);
        std::string stream_id = std::string(reinterpret_cast<char *>(id));
        g_free(id);
        hailo_roi->set_stream_id(stream_id);
    }

    // Call all functions.
    if (hailofilter->use_gst_buffer)
    {
        GstCaps *caps = gst_pad_get_current_caps(srcpad);
        GstVideoFrame frame;
        GstVideoInfo info;
        gst_video_info_from_caps(&info, caps);
        gst_caps_unref(caps);
        if (!gst_video_frame_map(&frame, &info, buffer, GstMapFlags(GST_MAP_READ | GST_MAP_WRITE)))
        {
            std::cerr << "Cannot map buffer to frame" << std::endl;
        }
        if (hailofilter->use_config)
        {
            auto handler = hailofilter->handler_gst;
            handler(hailo_roi, &frame, hailofilter->params);
        }
        else
        {
            auto handler = hailofilter->handler_gst_no_config;
            handler(hailo_roi, &frame);
        }
        gst_video_frame_unmap(&frame);
    }
    else
    {
        if (hailofilter->use_config)
        {
            auto handler = hailofilter->handler;
            handler(hailo_roi, hailofilter->params);
        }
        else
        {
            auto handler = hailofilter->handler_no_config;
            handler(hailo_roi);
        }
    }

    if (hailofilter->remove_tensors)
    {
        remove_tensors(buffer, hailo_roi);
    }

    GST_DEBUG_OBJECT(hailofilter, "transform_ip");
    return GST_FLOW_OK;
}
