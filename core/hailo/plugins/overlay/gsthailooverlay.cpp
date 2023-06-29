/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include <gst/gst.h>
#include <gst/video/video.h>
#include <opencv2/opencv.hpp>
#include "overlay/gsthailooverlay.hpp"
#include "common/image.hpp"
#include "overlay/overlay.hpp"
#include "gst_hailo_meta.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_hailooverlay_debug_category);
#define GST_CAT_DEFAULT gst_hailooverlay_debug_category

/* prototypes */

static void gst_hailooverlay_set_property(GObject *object,
                                          guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_hailooverlay_get_property(GObject *object,
                                          guint property_id, GValue *value, GParamSpec *pspec);
static void gst_hailooverlay_dispose(GObject *object);
static void gst_hailooverlay_finalize(GObject *object);

static gboolean gst_hailooverlay_start(GstBaseTransform *trans);
static gboolean gst_hailooverlay_stop(GstBaseTransform *trans);
static GstFlowReturn gst_hailooverlay_transform_ip(GstBaseTransform *trans,
                                                   GstBuffer *buffer);

/* class initialization */

G_DEFINE_TYPE_WITH_CODE(GstHailoOverlay, gst_hailooverlay, GST_TYPE_BASE_TRANSFORM,
                        GST_DEBUG_CATEGORY_INIT(gst_hailooverlay_debug_category, "hailooverlay", 0,
                                                "debug category for hailooverlay element"));

enum
{
    PROP_0,
    PROP_LINE_THICKNESS,
    PROP_FONT_THICKNESS,
    PROP_LANDMARK_POINT_RADIUS,
    PROP_FACE_BLUR,
    PROP_SHOW_CONF,
    PROP_MASK_OVERLAY_N_THREADS,
    PROP_LOCAL_GALLERY,
};

static void
gst_hailooverlay_class_init(GstHailoOverlayClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *base_transform_class =
        GST_BASE_TRANSFORM_CLASS(klass);

    const char *description = "Draws post-processing results for networks inferred by hailonet elements."
                              "\n\t\t\t   "
                              "Draws classes contained by HailoROI objects attached to incoming frames.";
    /* Setting up pads and setting metadata should be moved to
       base_class_init if you intend to subclass this class. */
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                                            gst_caps_from_string(GST_VIDEO_CAPS_MAKE("{ RGB, YUY2, RGBA, NV12 }"))));
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                                            gst_caps_from_string(GST_VIDEO_CAPS_MAKE("{ RGB, YUY2, RGBA, NV12 }"))));

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                          "hailooverlay - overlay element",
                                          "Hailo/Tools",
                                          description,
                                          "hailo.ai <contact@hailo.ai>");

    gobject_class->set_property = gst_hailooverlay_set_property;
    gobject_class->get_property = gst_hailooverlay_get_property;
    g_object_class_install_property(gobject_class, PROP_LINE_THICKNESS,
                                    g_param_spec_int("line-thickness", "line-thickness", "The thickness when drawing lines. Default 1.", 0, G_MAXINT, 1,
                                                     (GParamFlags)(GST_PARAM_MUTABLE_READY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_FONT_THICKNESS,
                                    g_param_spec_int("font-thickness", "font-thickness", "The thickness when drawing text. Default 1.", 0, G_MAXINT, 1,
                                                     (GParamFlags)(GST_PARAM_MUTABLE_READY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_FACE_BLUR,
                                    g_param_spec_boolean("face-blur", "face-blur", "Whether to blur faces", false,
                                                         (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_SHOW_CONF,
                                    g_param_spec_boolean("show-confidence", "show-confidence", "Whether to display confidence on detections, classifications etc...", true,
                                                         (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    // install property mask-overlay-n-threads uint default value 0
    g_object_class_install_property(gobject_class, PROP_MASK_OVERLAY_N_THREADS,
                                    g_param_spec_uint("mask-overlay-n-threads", "mask-overlay-n-threads", "Number of threads to use for parallel mask drawing. Default 0 (Will use the default value OpenCV initializes - effected by the system capabilities).", 0, G_MAXUINT, 0,
                                                      (GParamFlags)(GST_PARAM_MUTABLE_READY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_LOCAL_GALLERY,
                                    g_param_spec_boolean("local-gallery", "local-gallery", "Whether to display Identified and UnIdentified ROI's taken from the local gallery, as well as the Global ID they receive.", false,
                                                         (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_LANDMARK_POINT_RADIUS,
                                    g_param_spec_float("landmark-point-radius", "landmark-point-radius", "The radius of the points when drawing landmarks. Default 3.", 0, G_MAXFLOAT, 3,
                                                       (GParamFlags)(GST_PARAM_MUTABLE_READY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gobject_class->dispose = gst_hailooverlay_dispose;
    gobject_class->finalize = gst_hailooverlay_finalize;
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_hailooverlay_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_hailooverlay_stop);
    base_transform_class->transform_ip =
        GST_DEBUG_FUNCPTR(gst_hailooverlay_transform_ip);
}

static void
gst_hailooverlay_init(GstHailoOverlay *hailooverlay)
{
    hailooverlay->line_thickness = 1;
    hailooverlay->font_thickness = 1;
    hailooverlay->face_blur = false;
    hailooverlay->show_confidence = true;
    hailooverlay->local_gallery = false;
    hailooverlay->landmark_point_radius = 3;
    hailooverlay->mask_overlay_n_threads = 0;
}

void gst_hailooverlay_set_property(GObject *object, guint property_id,
                                   const GValue *value, GParamSpec *pspec)
{
    GstHailoOverlay *hailooverlay = GST_HAILO_OVERLAY(object);

    GST_DEBUG_OBJECT(hailooverlay, "set_property");

    switch (property_id)
    {
    case PROP_LINE_THICKNESS:
        hailooverlay->line_thickness = g_value_get_int(value);
        break;
    case PROP_FONT_THICKNESS:
        hailooverlay->font_thickness = g_value_get_int(value);
        break;
    case PROP_LANDMARK_POINT_RADIUS:
        hailooverlay->landmark_point_radius = g_value_get_float(value);
        break;
    case PROP_FACE_BLUR:
        hailooverlay->face_blur = g_value_get_boolean(value);
        break;
    case PROP_SHOW_CONF:
        hailooverlay->show_confidence = g_value_get_boolean(value);
        break;
    case PROP_MASK_OVERLAY_N_THREADS:
        hailooverlay->mask_overlay_n_threads = g_value_get_uint(value);
        break;
    case PROP_LOCAL_GALLERY:
        hailooverlay->local_gallery = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailooverlay_get_property(GObject *object, guint property_id,
                                   GValue *value, GParamSpec *pspec)
{
    GstHailoOverlay *hailooverlay = GST_HAILO_OVERLAY(object);

    GST_DEBUG_OBJECT(hailooverlay, "get_property");

    switch (property_id)
    {
    case PROP_LINE_THICKNESS:
        g_value_set_int(value, hailooverlay->line_thickness);
        break;
    case PROP_FONT_THICKNESS:
        g_value_set_int(value, hailooverlay->font_thickness);
        break;
    case PROP_LANDMARK_POINT_RADIUS:
        g_value_set_float(value, hailooverlay->landmark_point_radius);
        break;
    case PROP_FACE_BLUR:
        g_value_set_boolean(value, hailooverlay->face_blur);
        break;
    case PROP_SHOW_CONF:
        g_value_set_boolean(value, hailooverlay->show_confidence);
        break;
    case PROP_LOCAL_GALLERY:
        g_value_set_boolean(value, hailooverlay->local_gallery);
        break;
    case PROP_MASK_OVERLAY_N_THREADS:
        g_value_set_uint(value, hailooverlay->mask_overlay_n_threads);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

void gst_hailooverlay_dispose(GObject *object)
{
    GstHailoOverlay *hailooverlay = GST_HAILO_OVERLAY(object);
    GST_DEBUG_OBJECT(hailooverlay, "dispose");

    /* clean up as possible.  may be called multiple times */

    G_OBJECT_CLASS(gst_hailooverlay_parent_class)->dispose(object);
}

void gst_hailooverlay_finalize(GObject *object)
{
    GstHailoOverlay *hailooverlay = GST_HAILO_OVERLAY(object);
    GST_DEBUG_OBJECT(hailooverlay, "finalize");

    /* clean up object here */

    G_OBJECT_CLASS(gst_hailooverlay_parent_class)->finalize(object);
}

static gboolean
gst_hailooverlay_start(GstBaseTransform *trans)
{
    GstHailoOverlay *hailooverlay = GST_HAILO_OVERLAY(trans);
    GST_DEBUG_OBJECT(hailooverlay, "start");

    return TRUE;
}

static gboolean
gst_hailooverlay_stop(GstBaseTransform *trans)
{
    GstHailoOverlay *hailooverlay = GST_HAILO_OVERLAY(trans);
    GST_DEBUG_OBJECT(hailooverlay, "stop");

    return TRUE;
}

static GstFlowReturn
gst_hailooverlay_transform_ip(GstBaseTransform *trans,
                              GstBuffer *buffer)
{
    overlay_status_t ret = OVERLAY_STATUS_UNINITIALIZED;
    GstFlowReturn status = GST_FLOW_ERROR;
    GstHailoOverlay *hailooverlay = GST_HAILO_OVERLAY(trans);
    GstCaps *caps;
    cv::Mat mat;
    HailoROIPtr hailo_roi;
    GST_DEBUG_OBJECT(hailooverlay, "transform_ip");

    caps = gst_pad_get_current_caps(trans->sinkpad);

    GstVideoInfo *info = gst_video_info_new();
    gst_video_info_from_caps(info, caps);

    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READWRITE);

    std::shared_ptr<HailoMat> hmat = get_mat_by_format(buffer, info, hailooverlay->line_thickness, hailooverlay->font_thickness);
    gst_video_info_free(info);

    hailo_roi = get_hailo_main_roi(buffer, true);

    if (hmat)
    {
        // Blur faces if face-blur is activated.
        if (hailooverlay->face_blur)
        {
            face_blur(*hmat.get(), hailo_roi);
        }
        // Draw all results of the given roi on mat.
        ret = draw_all(*hmat.get(), hailo_roi, hailooverlay->landmark_point_radius, hailooverlay->show_confidence, hailooverlay->local_gallery, hailooverlay->mask_overlay_n_threads);
    }
    if (ret != OVERLAY_STATUS_OK)
    {
        status = GST_FLOW_ERROR;
        goto cleanup;
    }
    status = GST_FLOW_OK;
cleanup:
    gst_caps_unref(caps);
    gst_buffer_unmap(buffer, &map);
    return status;
}
