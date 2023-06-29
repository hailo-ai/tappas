/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
/**
 * @brief
 * Crops and scales image according to command line parameters or metadata of detections.
 * Takes cropped images and pushes them to the source pad.
 *
 */

#include <gst/gst.h>
#include <gst/video/video.h>
#include <iomanip>
#include <iostream>
#include <vector>
#include <map>
#include <typeinfo>
#include "common/image.hpp"
#include "cropping/gsthailocropper.hpp"
#include "hailo_objects.hpp"
#include "hailo_common.hpp"
#include "gst_hailo_meta.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_hailocropper_debug);
#define GST_CAT_DEFAULT gst_hailocropper_debug

enum
{
    PROP_0,
    PROP_PROCESS_LIB,
    PROP_PROCESS_FUNC_NAME,
    PROP_RESIZE_METHOD,
    PROP_USE_LETTERBOX,
};

#define GST_TYPE_HAILOCROPPER_RESIZE_METHOD (gst_hailocropper_resize_method_get_type())
static GType
gst_hailocropper_resize_method_get_type(void)
{
    static GType hailocropper_resize_method_type = 0;
    static const GEnumValue hailocropper_resize_methods[] = {
        {cv::INTER_NEAREST, "Nearest neighbour interpolation", "nearest-neighbour"},
        {cv::INTER_LINEAR, "Bilinear interpolation", "bilinear"},
        {cv::INTER_CUBIC, "Bicubic interpolation", "bicubic"},
        {cv::INTER_AREA, "Resampling using pixel area relation", "inter-area"},
        {0, NULL, NULL},
    };
    if (!hailocropper_resize_method_type)
    {
        hailocropper_resize_method_type =
            g_enum_register_static("GstHailoCropperResizeMethod", hailocropper_resize_methods);
    }
    return hailocropper_resize_method_type;
}

#define _do_init \
    GST_DEBUG_CATEGORY_INIT(gst_hailocropper_debug, "hailocropper", 0, "hailocropper element");
#define gst_hailocropper_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstHailoCropper, gst_hailocropper, GST_TYPE_HAILO_BASE_CROPPER, _do_init);

static void gst_hailocropper_set_property(GObject *object,
                                          guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_hailocropper_get_property(GObject *object,
                                          guint prop_id, GValue *value, GParamSpec *pspec);

static std::vector<HailoROIPtr> gst_hailocropper_prepare_crops(GstHailoBaseCropper *hailocropper,
                                                               GstBuffer *buf);
static GstStateChangeReturn gst_hailocropper_change_state(GstElement *element, GstStateChange transition);
void gst_hailocropper_resize_by_method(GstHailoBaseCropper *basecropper, cv::Mat &cropped_image, cv::Mat &resized_image, HailoROIPtr roi, GstVideoFormat image_format);

static void
gst_hailocropper_class_init(GstHailoCropperClass *klass)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;
    GstHailoBaseCropperClass *basecropper_class;
    gobject_class = (GObjectClass *)klass;
    gstelement_class = (GstElementClass *)klass;
    basecropper_class = (GstHailoBaseCropperClass *)klass;

    gobject_class->set_property = gst_hailocropper_set_property;
    gobject_class->get_property = gst_hailocropper_get_property;

    gst_element_class_set_details_simple(gstelement_class,
                                         "hailocropper",
                                         "Hailo/Tools",
                                         "Create a sub pipeline with cropped and scaled images determined by an so function.", "hailo.ai <contact@hailo.ai>");
    g_object_class_install_property(gobject_class, PROP_PROCESS_LIB,
                                    g_param_spec_string("so-path", "process so Path Location - Mandatory",
                                                        "Location of the so file to load", NULL,
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
    g_object_class_install_property(gobject_class, PROP_PROCESS_FUNC_NAME,
                                    g_param_spec_string("function-name", "Name of function in the so file - Mandatory",
                                                        "function-name", "",
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
    g_object_class_install_property(gobject_class, PROP_RESIZE_METHOD,
                                    g_param_spec_enum("resize-method", "resize method", "Resize method for each crop",
                                                      GST_TYPE_HAILOCROPPER_RESIZE_METHOD, (gint)cv::INTER_LINEAR,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_USE_LETTERBOX,
                                    g_param_spec_boolean("use-letterbox", "Use letterbox",
                                                         "If true, then this element will resize with  aspect ratio preserving. Default false.", false,
                                                         (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    gstelement_class->change_state = GST_DEBUG_FUNCPTR(gst_hailocropper_change_state);
    basecropper_class->prepare_crops = gst_hailocropper_prepare_crops;
    basecropper_class->resize = gst_hailocropper_resize_by_method;
}

static void
gst_hailocropper_init(GstHailoCropper *hailocropper)
{
    GST_DEBUG_OBJECT(hailocropper, "init");
    hailocropper->method = cv::INTER_LINEAR;
    hailocropper->use_letterbox = false;
}

static void
gst_hailocropper_set_property(GObject *object, guint prop_id,
                              const GValue *value, GParamSpec *pspec)
{
    GstHailoCropper *hailocropper = GST_HAILO_CROPPER(object);

    switch (prop_id)
    {
    case PROP_PROCESS_LIB:
        hailocropper->lib_path = g_strdup(g_value_get_string(value));
        break;
    case PROP_PROCESS_FUNC_NAME:
        hailocropper->function_name = g_strdup(g_value_get_string(value));
        break;
    case PROP_RESIZE_METHOD:
        GST_OBJECT_LOCK(hailocropper);
        hailocropper->method = (cv::InterpolationFlags)g_value_get_enum(value);
        GST_OBJECT_UNLOCK(hailocropper);
        break;
    case PROP_USE_LETTERBOX:
        hailocropper->use_letterbox = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_hailocropper_get_property(GObject *object, guint prop_id,
                              GValue *value, GParamSpec *pspec)
{
    GstHailoCropper *hailocropper = GST_HAILO_CROPPER(object);

    switch (prop_id)
    {
    case PROP_PROCESS_LIB:
        g_value_set_string(value, hailocropper->lib_path);
        break;
    case PROP_PROCESS_FUNC_NAME:
        g_value_set_string(value, hailocropper->function_name);
        break;
    case PROP_RESIZE_METHOD:
        GST_OBJECT_LOCK(hailocropper);
        g_value_set_enum(value, (gint)hailocropper->method);
        GST_OBJECT_UNLOCK(hailocropper);
        break;
    case PROP_USE_LETTERBOX:
        g_value_set_boolean(value, hailocropper->use_letterbox);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

void gst_hailocropper_resize_by_method(GstHailoBaseCropper *basecropper, cv::Mat &cropped_image, cv::Mat &resized_image, HailoROIPtr roi, GstVideoFormat image_format)
{
    GstHailoCropper *hailocropper = GST_HAILO_CROPPER(basecropper);
    if (hailocropper->use_letterbox)
    {
        resize_letterbox(hailocropper->method, cropped_image, resized_image, roi, image_format);
    }
    else
    {
        resize_normal(hailocropper->method, cropped_image, resized_image, image_format);
    }
}

/**
 * @brief Calls the so function to retrieve the ROI's to crop.
 *
 * @param basecropper basecropper element.
 * @param buf a GstBuffer.
 * @return std::vector<HailoROIPtr> vector of ROI's to crop and resize.
 */
static std::vector<HailoROIPtr> gst_hailocropper_prepare_crops(GstHailoBaseCropper *basecropper, GstBuffer *buf)
{
    GstHailoCropper *hailocropper = GST_HAILO_CROPPER(basecropper);
    // Get main HailoROI, in case this is the first hailo element in the pipeline, create one.
    HailoROIPtr hailo_roi = get_hailo_main_roi(buf, true);
    GstCaps *caps = gst_pad_get_current_caps(basecropper->sinkpad);

    GstVideoInfo *info = gst_video_info_new();
    gst_video_info_from_caps(info, caps);

    std::shared_ptr<HailoMat> image = get_mat_by_format(buf, info);
    gst_video_info_free(info);

    std::vector<HailoROIPtr> crop_rois = hailocropper->handler(image, hailo_roi);
    gst_caps_unref(caps);
    return crop_rois;
}

static gboolean
gst_hailocropper_load_symbol(GstHailoCropper *hailocropper)
{
    // Load the give SO using dlopen.
    hailocropper->loaded_lib = dlopen(hailocropper->lib_path, RTLD_LAZY);
    if (!hailocropper->loaded_lib)
    {
        std::cerr << "Could not load lib " << dlerror() << std::endl;
        return FALSE;
    }
    // reset errors
    dlerror();

    hailocropper->handler = (std::vector<HailoROIPtr>(*)(std::shared_ptr<HailoMat>, HailoROIPtr))dlsym(hailocropper->loaded_lib, hailocropper->function_name);
    // If there was an error loading one of the symbols, close the dl and break.
    const char *dlsym_error = dlerror();
    if (dlsym_error || !hailocropper->handler)
    {
        std::cerr << "Cannot load symbol: " << dlsym_error << std::endl;
        dlclose(hailocropper->loaded_lib);
        return FALSE;
    }

    return TRUE;
}

static gboolean
gst_hailocropper_free_symbol(GstHailoCropper *hailocropper)
{
    if (hailocropper->loaded_lib)
    {
        dlclose(hailocropper->loaded_lib);
    }
    return TRUE;
}

static GstStateChangeReturn
gst_hailocropper_change_state(GstElement *element, GstStateChange transition)
{
    GstStateChangeReturn ret;

    ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    if (ret == GST_STATE_CHANGE_FAILURE)
        return ret;

    switch (transition)
    {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        if (!gst_hailocropper_load_symbol(GST_HAILO_CROPPER(element)))
            ret = GST_STATE_CHANGE_FAILURE;
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
    {
        if (!gst_hailocropper_free_symbol(GST_HAILO_CROPPER(element)))
            ret = GST_STATE_CHANGE_FAILURE;
        break;
    }
    default:
        break;
    }

    return ret;
}
