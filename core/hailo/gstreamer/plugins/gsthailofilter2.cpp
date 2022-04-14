/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include "gsthailofilter2.hpp"
#include "tensor_meta.hpp"
#include "metadata/gst_hailo_meta.hpp"
#include "hailo/hailort.h"
#include <gst/gst.h>
#include <dlfcn.h>
#include <map>
#include <iostream>

GST_DEBUG_CATEGORY_STATIC(gst_hailofilter2_debug_category);
#define GST_CAT_DEFAULT gst_hailofilter2_debug_category
#define DEFAULT_FUNCTION_NAME "filter"

static void gst_hailofilter2_set_property(GObject *object,
                                          guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_hailofilter2_get_property(GObject *object,
                                          guint property_id, GValue *value, GParamSpec *pspec);
static void gst_hailofilter2_dispose(GObject *object);
static void gst_hailofilter2_finalize(GObject *object);

static gboolean gst_hailofilter2_start(GstBaseTransform *trans);
static gboolean gst_hailofilter2_stop(GstBaseTransform *trans);
static GstFlowReturn gst_hailofilter2_transform_ip(GstBaseTransform *trans,
                                                   GstBuffer *buffer);

enum
{
    PROP_0,
    PROP_PROCESS_LIB,
    PROP_PROCESS_FUNC_NAME,
};

G_DEFINE_TYPE_WITH_CODE(GstHailoFilter2, gst_hailofilter2, GST_TYPE_BASE_TRANSFORM,
                        GST_DEBUG_CATEGORY_INIT(gst_hailofilter2_debug_category, "hailofilter2", 0,
                                                "debug category for hailofilter2 element"));

static void
gst_hailofilter2_class_init(GstHailoFilter2Class *klass)
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
                                          "hailofilter2 - postprocessing element", "Hailo/Tools", "Allowes to user access Hailonet's output using an so file.",
                                          "hailo.ai <contact@hailo.ai>");

    gobject_class->set_property = gst_hailofilter2_set_property;
    gobject_class->get_property = gst_hailofilter2_get_property;
    g_object_class_install_property(gobject_class, PROP_PROCESS_LIB,
                                    g_param_spec_string("so-path", "process so Path Location",
                                                        "Location of the so file to load", NULL,
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
    g_object_class_install_property(gobject_class, PROP_PROCESS_FUNC_NAME,
                                    g_param_spec_string("function-name", "Name of function in the so file",
                                                        "function-name", "filter",
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
    gobject_class->dispose = gst_hailofilter2_dispose;
    gobject_class->finalize = gst_hailofilter2_finalize;
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_hailofilter2_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_hailofilter2_stop);
    base_transform_class->transform_ip =
        GST_DEBUG_FUNCPTR(gst_hailofilter2_transform_ip);
}

static void
gst_hailofilter2_init(GstHailoFilter2 *hailofilter2)
{
    hailofilter2->default_function_name = g_strdup(DEFAULT_FUNCTION_NAME);
}

void gst_hailofilter2_set_property(GObject *object, guint property_id,
                                   const GValue *value, GParamSpec *pspec)
{
    GstHailoFilter2 *hailofilter2 = GST_HAILO_FILTER2(object);

    GST_DEBUG_OBJECT(hailofilter2, "set_property");

    switch (property_id)
    {
    case PROP_PROCESS_LIB:
        hailofilter2->lib_path = g_strdup(g_value_get_string(value));
        break;
    case PROP_PROCESS_FUNC_NAME:
        hailofilter2->function_name.push_back(g_strdup(g_value_get_string(value)));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailofilter2_get_property(GObject *object, guint property_id,
                                   GValue *value, GParamSpec *pspec)
{
    GstHailoFilter2 *hailofilter2 = GST_HAILO_FILTER2(object);

    GST_DEBUG_OBJECT(hailofilter2, "get_property");

    switch (property_id)
    {
    case PROP_PROCESS_LIB:
        g_value_set_string(value, hailofilter2->lib_path);
        break;
    case PROP_PROCESS_FUNC_NAME:
        g_value_set_string(value, DEFAULT_FUNCTION_NAME);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailofilter2_dispose(GObject *object)
{
    GstHailoFilter2 *hailofilter2 = GST_HAILO_FILTER2(object);
    if (hailofilter2->loaded_lib != nullptr)
    {
        dlclose(hailofilter2->loaded_lib);
    }

    GST_DEBUG_OBJECT(hailofilter2, "dispose");

    /* clean up as possible.  may be called multiple times */

    G_OBJECT_CLASS(gst_hailofilter2_parent_class)->dispose(object);
}

void gst_hailofilter2_finalize(GObject *object)
{
    GstHailoFilter2 *hailofilter2 = GST_HAILO_FILTER2(object);

    GST_DEBUG_OBJECT(hailofilter2, "finalize");

    /* clean up object here */

    G_OBJECT_CLASS(gst_hailofilter2_parent_class)->finalize(object);
}

static gboolean
gst_hailofilter2_start(GstBaseTransform *trans)
{
    GstHailoFilter2 *hailofilter2 = GST_HAILO_FILTER2(trans);
    // Load the give SO using dlopen.
    hailofilter2->loaded_lib = dlopen(hailofilter2->lib_path, RTLD_LAZY);
    if (!hailofilter2->loaded_lib)
    {
        std::cerr << "Could not load lib " << dlerror() << std::endl;
    }
    // reset errors
    dlerror();
    // Use the default function name if no name was provided.
    if (hailofilter2->function_name.size() == 0)
    {
        hailofilter2->function_name.push_back(hailofilter2->default_function_name);
    }
    // Create handler for all function-names on the list.
    for (uint i = 0; i < hailofilter2->function_name.size(); i++)
    {
        auto f = hailofilter2->function_name[i];
        hailofilter2->handler.push_back((void (*)(HailoROIPtr))dlsym(hailofilter2->loaded_lib, f));
        // If there was an error loading one of the symbols, close the dl and break.
        const char *dlsym_error = dlerror();
        if (dlsym_error)
        {
            std::cerr << "Cannot load symbol: " << dlsym_error << std::endl;
            dlclose(hailofilter2->loaded_lib);
            break;
        }
    }
    GST_DEBUG_OBJECT(hailofilter2, "start");

    return TRUE;
}

static gboolean
gst_hailofilter2_stop(GstBaseTransform *trans)
{
    GstHailoFilter2 *hailofilter2 = GST_HAILO_FILTER2(trans);

    GST_DEBUG_OBJECT(hailofilter2, "stop");

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
        const hailo_vstream_info_t vstream_info = reinterpret_cast<GstHailoTensorMeta *>(gst_buffer_get_meta(pmeta->buffer, g_type_from_name(TENSOR_META_API_NAME)))->info;
        roi->add_tensor(std::make_shared<NewHailoTensor>(reinterpret_cast<uint8_t *>(info.data), vstream_info));
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

static GstFlowReturn
gst_hailofilter2_transform_ip(GstBaseTransform *trans,
                              GstBuffer *buffer)
{
    GstHailoFilter2 *hailofilter2 = GST_HAILO_FILTER2(trans);

    HailoROIPtr hailo_roi = get_hailo_main_roi(buffer, true);
    get_tensors_from_meta(buffer, hailo_roi);
    // Call all functions.
    for (uint i = 0; i < hailofilter2->handler.size(); i++)
    {
        auto handler = hailofilter2->handler[i];
        handler(hailo_roi);
    }
    remove_tensors(buffer, hailo_roi);
    GST_DEBUG_OBJECT(hailofilter2, "transform_ip");
    return GST_FLOW_OK;
}
