/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/

// General cpp includes
#include <gst/gst.h>

// Tappas includes
#include "hailo_objects.hpp"
#include "gst_hailo_meta.hpp"
#include "gsthailogallery.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_hailo_gallery_debug_category);
#define GST_CAT_DEFAULT gst_hailo_gallery_debug_category

//******************************************************************
// PROTOTYPES
//******************************************************************
static void gst_hailo_gallery_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_hailo_gallery_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gst_hailo_gallery_dispose(GObject *object);
static GstFlowReturn gst_hailo_gallery_transform_ip(GstBaseTransform *trans, GstBuffer *buffer);

enum
{
    PROP_0,
    PROP_CLASS_ID,
    PROP_SIMILARITY_THR,
    PROP_GALLERY_QUEUE_SIZE,
};

//******************************************************************
// PAD TEMPLATES
//******************************************************************
/* Source Caps */
#define VIDEO_SRC_CAPS \
    gst_caps_new_any()

/* Sink Caps */
#define VIDEO_SINK_CAPS \
    gst_caps_new_any()

//******************************************************************
// CLASS INITIALIZATION
//******************************************************************
/* Define the element within the plugin */
G_DEFINE_TYPE_WITH_CODE(GstHailoGallery, gst_hailo_gallery, GST_TYPE_BASE_TRANSFORM,
                        GST_DEBUG_CATEGORY_INIT(gst_hailo_gallery_debug_category, "hailogallery", 0,
                                                "debug category for hailogallery element"));

/* Class initialization */
static void
gst_hailo_gallery_class_init(GstHailoGalleryClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);

    // Add the src pad
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, VIDEO_SRC_CAPS));
    // Add the sink pad
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, VIDEO_SINK_CAPS));

    // Set the element metadata
    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                          "Hailo gallery element",
                                          "Hailo/Filter/Metadata",
                                          "Represents a Gallery object for RE-ID purposes",
                                          "hailo.ai <contact@hailo.ai>");

    // Set the element properties
    gobject_class->set_property = gst_hailo_gallery_set_property;
    gobject_class->get_property = gst_hailo_gallery_get_property;
    g_object_class_install_property(gobject_class, PROP_CLASS_ID,
                                    g_param_spec_int("class-id", "class-id", "The class id of the class to update into the gallery. Default -1 crosses classes.", G_MININT, G_MAXINT, -1,
                                                     (GParamFlags)(GST_PARAM_MUTABLE_READY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_SIMILARITY_THR,
                                    g_param_spec_float("similarity-thr", "Gallery Similarity Threshold",
                                                       "Similarity threshold used in Gallery to find New ID's. Closer to 1.0 is less similar.",
                                                       0.0, 1.0, 0.8,
                                                       (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_GALLERY_QUEUE_SIZE,
                                    g_param_spec_int("gallery-queue-size", "Queue size",
                                                     "Number of Matrixes to save for each global ID",
                                                     0, G_MAXINT, 100,
                                                     (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    // Set virtual functions
    gobject_class->dispose = gst_hailo_gallery_dispose;
    base_transform_class->transform_ip = GST_DEBUG_FUNCPTR(gst_hailo_gallery_transform_ip);
}

/* Instance initialization */
static void
gst_hailo_gallery_init(GstHailoGallery *hailogallery)
{
    hailogallery->debug = false;
    hailogallery->class_id = -1;
    hailogallery->gallery = Gallery();
}

//******************************************************************
// PROPERTY HANDLING
//******************************************************************
/* Handle setting properties */
void gst_hailo_gallery_set_property(GObject *object, guint property_id,
                                    const GValue *value, GParamSpec *pspec)
{
    GstHailoGallery *hailogallery = GST_HAILO_GALLERY(object);

    GST_DEBUG_OBJECT(hailogallery, "set_property");

    switch (property_id)
    {
    case PROP_CLASS_ID:
        hailogallery->class_id = g_value_get_int(value);
        break;
    case PROP_SIMILARITY_THR:
        hailogallery->gallery.set_similarity_threshold(g_value_get_float(value));
        break;
    case PROP_GALLERY_QUEUE_SIZE:
        hailogallery->gallery.set_queue_size(g_value_get_int(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

/* Handle getting properties */
void gst_hailo_gallery_get_property(GObject *object, guint property_id,
                                    GValue *value, GParamSpec *pspec)
{
    GstHailoGallery *hailogallery = GST_HAILO_GALLERY(object);

    GST_DEBUG_OBJECT(hailogallery, "get_property");

    switch (property_id)
    {
    case PROP_CLASS_ID:
        g_value_set_int(value, hailogallery->class_id);
        break;
    case PROP_SIMILARITY_THR:
        g_value_set_float(value, hailogallery->gallery.get_similarity_threshold());
        break;
    case PROP_GALLERY_QUEUE_SIZE:
        g_value_set_int(value, hailogallery->gallery.get_queue_size());
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

//******************************************************************
// ELEMENT LIFECYCLE MANAGEMENT
//******************************************************************
/* Release any references to resources when the object first knows it will be destroyed.
   Can be called any number of times. */
void gst_hailo_gallery_dispose(GObject *object)
{
    GstHailoGallery *hailogallery = GST_HAILO_GALLERY(object);

    GST_DEBUG_OBJECT(hailogallery, "dispose");

    G_OBJECT_CLASS(gst_hailo_gallery_parent_class)->dispose(object);
}

//******************************************************************
// BUFFER TRANSFORMATION
//******************************************************************
/* Transform a buffer in place. This is where the actual gallery filter is applied. */
static GstFlowReturn
gst_hailo_gallery_transform_ip(GstBaseTransform *trans, GstBuffer *buffer)
{
    GstHailoGallery *hailogallery = GST_HAILO_GALLERY(trans);
    HailoROIPtr hailo_roi = get_hailo_main_roi(buffer, true);

    std::vector<HailoDetectionPtr> detections;
    for (auto obj : hailo_roi->get_objects_typed(HAILO_DETECTION))
    {
        HailoDetectionPtr detection = std::dynamic_pointer_cast<HailoDetection>(obj);
        if ((hailogallery->class_id == -1) || (detection->get_class_id() == hailogallery->class_id))
            detections.push_back(detection);
    }
    hailogallery->gallery.update(detections);

    GST_DEBUG_OBJECT(hailogallery, "transform_ip");
    return GST_FLOW_OK;
}