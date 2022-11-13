/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gsthailopython.hpp"
#include "gst_hailo_meta.hpp"
#include "tensor_meta.hpp"
#include "hailopython_infra.hpp"
#include <gst/gst.h>
#include <gst/video/gstvideofilter.h>
#include <gst/video/video.h>

#if __GNUC__ > 8
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#define DEFAULT_MODULE "processor.py"
#define DEFAULT_FUNCTION "run"
#define DEFAULT_FINALIZE_FUNCTION "none"

GST_DEBUG_CATEGORY_STATIC(gst_hailopython_debug_category);
#define GST_CAT_DEFAULT gst_hailopython_debug_category

/* prototypes */

static void gst_hailopython_set_property(GObject *object, guint property_id, const GValue *value,
                                         GParamSpec *pspec);
static void gst_hailopython_get_property(GObject *object, guint property_id, GValue *value,
                                         GParamSpec *pspec);
static void gst_hailopython_dispose(GObject *object);
static void gst_hailopython_finalize(GObject *object);

static gboolean gst_hailopython_set_caps(GstBaseTransform *trans, GstCaps *incaps, GstCaps *outcaps);
static gboolean gst_hailopython_start(GstBaseTransform *trans);
static gboolean gst_hailopython_stop(GstBaseTransform *trans);
static GstFlowReturn gst_hailopython_transform_frame_ip(GstVideoFilter *filter,
                                                        GstVideoFrame *frame);

enum
{
    PROP_0,
    PROP_MODULE,
    PROP_FUNCTION,
    PROP_FINALIZE_FUNCTION
};

/* pad templates */

#define VIDEO_SRC_CAPS GST_VIDEO_CAPS_MAKE("{ RGB, YUY2, I420, RGBA, NV12 }")

#define VIDEO_SINK_CAPS GST_VIDEO_CAPS_MAKE("{ RGB, YUY2, I420, RGBA, NV12 }")

/* class initialization */

G_DEFINE_TYPE_WITH_CODE(GstHailoPython, gst_hailopython, GST_TYPE_VIDEO_FILTER,
                        GST_DEBUG_CATEGORY_INIT(gst_hailopython_debug_category, "hailopython", 0,
                                                "debug category for hailopython element"));

static void gst_hailopython_class_init(GstHailoPythonClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
    GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS(klass);

    /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
    gst_element_class_add_pad_template(
        GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, gst_caps_from_string(VIDEO_SRC_CAPS)));
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                                            gst_caps_from_string(VIDEO_SINK_CAPS)));

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass), "HailoPython Element", "Generic",
                                          "HailoPython Element", "hailo.ai <contact@hailo.ai>");

    gobject_class->set_property = gst_hailopython_set_property;
    gobject_class->get_property = gst_hailopython_get_property;
    gobject_class->dispose = gst_hailopython_dispose;
    gobject_class->finalize = gst_hailopython_finalize;

    base_transform_class->set_caps = GST_DEBUG_FUNCPTR(gst_hailopython_set_caps);
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_hailopython_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_hailopython_stop);
    video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR(gst_hailopython_transform_frame_ip);

    g_object_class_install_property(
        gobject_class, PROP_MODULE,
        g_param_spec_string("module", "Python module name", "Python module name", DEFAULT_MODULE,
                            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(
        gobject_class, PROP_FUNCTION,
        g_param_spec_string("function", "Python function name", "Python function name",
                            DEFAULT_FUNCTION,
                            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(
        gobject_class, PROP_FINALIZE_FUNCTION,
        g_param_spec_string("finalize-function", "Python finalize function name", "Python finalize function name",
                            DEFAULT_FINALIZE_FUNCTION,
                            (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
}

static void gst_hailopython_init(GstHailoPython *hailopython)
{
    auto curr_path = fs::current_path();

    curr_path /= DEFAULT_MODULE;

    hailopython->module_name = g_strdup(curr_path.c_str());
    hailopython->function_name = g_strdup(DEFAULT_FUNCTION);
    hailopython->finalize_function_name = g_strdup(DEFAULT_FINALIZE_FUNCTION);
    hailopython->python_callback = nullptr;
    hailopython->python_finalize_callback = nullptr;
}

void gst_hailopython_set_property(GObject *object, guint property_id, const GValue *value,
                                  GParamSpec *pspec)
{
    GstHailoPython *hailopython = GST_HAILO_PYTHON(object);

    GST_DEBUG_OBJECT(hailopython, "set_property");

    switch (property_id)
    {
    case PROP_MODULE:
        g_free(hailopython->module_name);
        hailopython->module_name = g_value_dup_string(value);
        break;
    case PROP_FUNCTION:
        g_free(hailopython->function_name);
        hailopython->function_name = g_value_dup_string(value);
        break;
    case PROP_FINALIZE_FUNCTION:
        g_free(hailopython->finalize_function_name);
        hailopython->finalize_function_name = g_value_dup_string(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailopython_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    GstHailoPython *hailopython = GST_HAILO_PYTHON(object);

    GST_DEBUG_OBJECT(hailopython, "get_property");

    switch (property_id)
    {
    case PROP_MODULE:
        g_value_set_string(value, hailopython->module_name);
        break;
    case PROP_FUNCTION:
        g_value_set_string(value, hailopython->function_name);
        break;
    case PROP_FINALIZE_FUNCTION:
        g_value_set_string(value, hailopython->finalize_function_name);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailopython_dispose(GObject *object)
{
    GstHailoPython *hailopython = GST_HAILO_PYTHON(object);

    GST_DEBUG_OBJECT(hailopython, "dispose");

    /* clean up as possible.  may be called multiple times */
    G_OBJECT_CLASS(gst_hailopython_parent_class)->dispose(object);
}

void gst_hailopython_finalize(GObject *object)
{
    GstHailoPython *hailopython = GST_HAILO_PYTHON(object);

    GST_DEBUG_OBJECT(hailopython, "finalize");

    if (hailopython->python_finalize_callback != nullptr) 
    {
        char *error_msg;
        GstFlowReturn result = invoke_python_callback(hailopython->python_finalize_callback, &error_msg);
        
        if (result != GST_FLOW_OK)
        {
            GST_ELEMENT_ERROR(hailopython, LIBRARY, FAILED, ("%s", error_msg), (NULL));
        }
    }

    delete hailopython->python_callback;
    hailopython->python_callback = nullptr;

    if (hailopython->python_finalize_callback != nullptr) 
    {
        delete hailopython->python_finalize_callback;
        hailopython->python_finalize_callback = nullptr;
    }
    
    g_free(hailopython->module_name);
    hailopython->module_name = nullptr;

    g_free(hailopython->function_name);
    hailopython->function_name = nullptr;

    g_free(hailopython->finalize_function_name);
    hailopython->finalize_function_name = nullptr;

    G_OBJECT_CLASS(gst_hailopython_parent_class)->finalize(object);
}

static gboolean gst_hailopython_set_caps(GstBaseTransform *trans, GstCaps *incaps, GstCaps *outcaps) 
{    
    // Mark as un-used
    (void)(outcaps);

    GstHailoPython *hailopython = GST_HAILO_PYTHON(trans);
    GST_DEBUG_OBJECT(hailopython, "set_caps");

    char *error_msg;
     GstFlowReturn result = set_python_callback_caps(hailopython->python_callback, incaps, &error_msg);

    if (result != GST_FLOW_OK)
    {
        GST_ELEMENT_ERROR(hailopython, LIBRARY, FAILED, ("%s", error_msg), (NULL));
    }
    
    return GST_BASE_TRANSFORM_CLASS(gst_hailopython_parent_class)->set_caps(trans, incaps, outcaps);
}

static gboolean gst_hailopython_start(GstBaseTransform *trans)
{
    GstHailoPython *hailopython = GST_HAILO_PYTHON(trans);

    GST_DEBUG_OBJECT(hailopython, "start");

    if (hailopython->python_callback)
    {
        GST_DEBUG("start called with initialized python callback, deleting python callback");
        delete hailopython->python_callback;
        hailopython->python_callback = nullptr;
    }

    if (hailopython->python_finalize_callback)
    {
        GST_DEBUG("start called with initialized python finalize callback, deleting python callback");
        delete hailopython->python_finalize_callback;
        hailopython->python_finalize_callback = nullptr;
    }

    if (!hailopython->module_name)
    {
        GST_ERROR_OBJECT(hailopython, "Parameter 'module' not set");
        GST_ELEMENT_ERROR(hailopython, LIBRARY, INIT, ("Error creating Python callback"),
                          ("Invalid module"));
        return FALSE;
    }

    auto module_path = fs::absolute(hailopython->module_name);
    if (!fs::exists(module_path) || !fs::is_regular_file(module_path))
    {
        GST_ERROR_OBJECT(hailopython, "Parameter 'module_name' points to a non-existant file");
        GST_ELEMENT_ERROR(hailopython, LIBRARY, INIT, ("Error creating Python callback."),
                          ("Invalid module name file path (%s).", module_path.c_str()));
        return FALSE;
    }

    if (!hailopython->function_name)
    {
        GST_ERROR_OBJECT(hailopython, "Parameter 'function-name' is null");
        GST_ELEMENT_ERROR(hailopython, LIBRARY, INIT, ("Error creating Python callback."),
                          ("Invalid function name."));
        return FALSE;
    }
    char *error_msg;
    hailopython->python_callback = create_python_callback(module_path.c_str(),
                                                          hailopython->function_name, "[]", "{}", &error_msg);

    if (!hailopython->python_callback)
    {
        GST_ELEMENT_ERROR(trans, LIBRARY, INIT, ("Error creating Python callback"),
                          ("Module: %s\n Function: %s\n Error: %s\n",
                          hailopython->module_name, hailopython->function_name, error_msg));
    }

    if (g_strcmp0(hailopython->finalize_function_name, g_strdup(DEFAULT_FINALIZE_FUNCTION)) != 0)
    {
        hailopython->python_finalize_callback = create_python_callback(module_path.c_str(),
                                                                    hailopython->finalize_function_name,
                                                                    "[]", "{}", &error_msg);

        if (!hailopython->python_finalize_callback)
        {
            GST_ELEMENT_ERROR(trans, LIBRARY, INIT, ("Error creating Python Finalize callback"),
                            ("Module: %s\n Function: %s\n Error: %s\n",
                            hailopython->module_name, hailopython->finalize_function_name, error_msg));
        }
    }

    return TRUE;
}

static gboolean gst_hailopython_stop(GstBaseTransform *trans)
{
    GstHailoPython *hailopython = GST_HAILO_PYTHON(trans);

    GST_DEBUG_OBJECT(hailopython, "stop");

    return TRUE;
}

/**
 * @brief Get the tensors from meta object
 *
 * @param buffer The buffer to extract the tensor_meta from.
 * @param roi ROI to add the tensors to.
 * @note This function implementation should be changed according to HRT-5150 on the next release.
 */
static void get_tensors_from_meta(GstBuffer *buffer, HailoROIPtr roi)
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
        roi->add_tensor(std::make_shared<HailoTensor>(reinterpret_cast<uint8_t *>(info.data), vstream_info));
        gst_buffer_unmap(pmeta->buffer, &info);
    }
}

static GstFlowReturn gst_hailopython_transform_frame_ip(GstVideoFilter *filter, GstVideoFrame *frame)
{
    GstHailoPython *hailopython = GST_HAILO_PYTHON(filter);
    GstFlowReturn result = GST_FLOW_ERROR;
    GST_DEBUG_OBJECT(hailopython, "transform_frame_ip");
    char *error_msg;
    auto roi = get_hailo_main_roi(frame->buffer, true);
    get_tensors_from_meta(frame->buffer, roi);

    result = invoke_python_callback(hailopython->python_callback, frame->buffer, (py_descriptor_t)roi.get(), &error_msg);

    if (result != GST_FLOW_OK)
    {
        GST_ELEMENT_ERROR(hailopython, LIBRARY, FAILED, ("%s", error_msg), (NULL));
    }

    return result;
}

static gboolean plugin_init(GstPlugin *plugin)
{
    return gst_element_register(plugin, "hailopython", GST_RANK_PRIMARY, GST_TYPE_HAILO_PYTHON);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, hailopython, "Hailo Python Plugin", plugin_init,
                  VERSION, "unknown", "hailo_python", "https://hailo.ai/")
