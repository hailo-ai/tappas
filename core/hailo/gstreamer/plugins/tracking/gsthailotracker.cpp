/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/

// General cpp includes
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include <iostream>
#include <vector>
#include <algorithm>

// Tappas includes
// General includes
#include "hailo_common.hpp"
#include "hailo_objects.hpp"
// Plugin includes
#include "gst_hailo_meta.hpp"
#include "gsthailotracker.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_hailo_tracker_debug_category);
#define GST_CAT_DEFAULT gst_hailo_tracker_debug_category
const gchar *OCR_EVENT_NAME = "OCR_LABEL";

//******************************************************************
// PROTOTYPES
//******************************************************************
static void gst_hailo_tracker_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_hailo_tracker_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gst_hailo_tracker_dispose(GObject *object);
static void gst_hailo_tracker_finalize(GObject *object);
static gboolean gst_hailo_tracker_start(GstBaseTransform *trans);
static gboolean gst_hailo_tracker_stop(GstBaseTransform *trans);
static gboolean gst_hailo_tracker_set_info(GstVideoFilter *filter, GstCaps *incaps, GstVideoInfo *in_info, GstCaps *outcaps, GstVideoInfo *out_info);
static GstFlowReturn gst_hailo_tracker_transform_frame_ip(GstVideoFilter *filter, GstVideoFrame *frame);
static gboolean gst_hailo_tracker_sink_event(GstBaseTransform *trans, GstEvent *event);
static gboolean gst_hailo_tracker_src_event(GstBaseTransform *trans, GstEvent *event);
static gboolean gst_hailo_tracker_ocr_event(GstHailoTracker *hailotracker, const GstStructure *event_structure);

enum
{
    PROP_0,
    PROP_CLASS_ID,
    PROP_KALMAN_DIST_THR,
    PROP_IOU_THR,
    PROP_INIT_IOU_THR,
    PROP_KEEP_TRACKED_FRAMES,
    PROP_KEEP_NEW_FRAMES,
    PROP_KEEP_LOST_FRAMES,
    PROP_KEEP_PAST_METADATA,
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
G_DEFINE_TYPE_WITH_CODE(GstHailoTracker, gst_hailo_tracker, GST_TYPE_VIDEO_FILTER,
                        GST_DEBUG_CATEGORY_INIT(gst_hailo_tracker_debug_category, "hailotracker", 0,
                                                "debug category for hailotracker element"));

/* Class initialization */
static void
gst_hailo_tracker_class_init(GstHailoTrackerClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
    GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS(klass);

    // Add the src pad
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, VIDEO_SRC_CAPS));
    // Add the sink pad
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, VIDEO_SINK_CAPS));

    // Set the element metadata
    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                          "Hailo object tracking element",
                                          "Hailo/Filter/Metadata",
                                          "Applies Joint Detection and Embedding (JDE) model with Kalman filtering to track object instances.",
                                          "hailo.ai <contact@hailo.ai>");

    // Set the element properties
    gobject_class->set_property = gst_hailo_tracker_set_property;
    gobject_class->get_property = gst_hailo_tracker_get_property;
    g_object_class_install_property(gobject_class, PROP_CLASS_ID,
                                    g_param_spec_int("class-id", "class-id", "The class id of the class to track. Default -1 crosses classes.", G_MININT, G_MAXINT, -1,
                                                     (GParamFlags)(GST_PARAM_MUTABLE_READY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_KALMAN_DIST_THR,
                                    g_param_spec_float("kalman-dist-thr", "Kalman Distance Threshold",
                                                       "Threshold used in Kalman filter to compare Mahalanobis cost matrix. Closer to 1.0 is looser.",
                                                       0.0, 1.0, DEFAULT_KALMAN_DISTANCE,
                                                       (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_IOU_THR,
                                    g_param_spec_float("iou-thr", "IOU Distance Threshold",
                                                       "Threshold used in Kalman filter to compare IOU cost matrix. Closer to 1.0 is looser.",
                                                       0.0, 1.0, DEFAULT_IOU_THRESHOLD,
                                                       (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_INIT_IOU_THR,
                                    g_param_spec_float("init-iou-thr", "Initial IOU Distance Threshold",
                                                       "Threshold used in Kalman filter to compare IOU cost matrix of newly found instances. Closer to 1.0 is looser.",
                                                       0.0, 1.0, DEFAULT_INIT_IOU_THRESHOLD,
                                                       (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_KEEP_TRACKED_FRAMES,
                                    g_param_spec_int("keep-tracked-frames", "Keep tracked frames",
                                                     "Number of frames to keep without a successful match before a 'tracked' instance is considered 'lost'.",
                                                     0, G_MAXINT, DEFAULT_KEEP_FRAMES,
                                                     (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_KEEP_NEW_FRAMES,
                                    g_param_spec_int("keep-new-frames", "Keep new frames",
                                                     "Number of frames to keep without a successful match before a 'new' instance is removed from the tracking record.",
                                                     0, G_MAXINT, DEFAULT_KEEP_FRAMES,
                                                     (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_KEEP_LOST_FRAMES,
                                g_param_spec_int("keep-lost-frames", "Keep lost frames",
                                                    "Number of frames to keep without a successful match before a 'lost' instance is removed from the tracking record.",
                                                    0, G_MAXINT, DEFAULT_KEEP_FRAMES,
                                                    (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_KEEP_PAST_METADATA,
                                g_param_spec_boolean("keep-past-metadata", "Keep past metadata on tracked object",
                                                    "When set (default) past metadata is kept on tracked objects. When unset past metadata is removed from tracked objects.",
                                                    DEFAULT_KEEP_PAST_METADATA,
                                                    (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    // Set virtual functions
    gobject_class->dispose = gst_hailo_tracker_dispose;
    gobject_class->finalize = gst_hailo_tracker_finalize;
    base_transform_class->start = GST_DEBUG_FUNCPTR(gst_hailo_tracker_start);
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_hailo_tracker_stop);
    video_filter_class->set_info = GST_DEBUG_FUNCPTR(gst_hailo_tracker_set_info);
    video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR(gst_hailo_tracker_transform_frame_ip);
    base_transform_class->sink_event = GST_DEBUG_FUNCPTR(gst_hailo_tracker_sink_event);
    base_transform_class->src_event = GST_DEBUG_FUNCPTR(gst_hailo_tracker_src_event);
}

/* Instance initialization */
static void
gst_hailo_tracker_init(GstHailoTracker *hailotracker)
{
    hailotracker->class_id = -1;
    hailotracker->tracker_params.kalman_distance = DEFAULT_KALMAN_DISTANCE;
    hailotracker->tracker_params.iou_threshold = DEFAULT_IOU_THRESHOLD;
    hailotracker->tracker_params.init_iou_threshold = DEFAULT_INIT_IOU_THRESHOLD;
    hailotracker->tracker_params.keep_tracked_frames = DEFAULT_KEEP_FRAMES;
    hailotracker->tracker_params.keep_new_frames = DEFAULT_KEEP_FRAMES;
    hailotracker->tracker_params.keep_lost_frames = DEFAULT_KEEP_FRAMES;
    hailotracker->tracker_params.keep_past_metadata = DEFAULT_KEEP_PAST_METADATA;
}

//******************************************************************
// PROPERTY HANDLING
//******************************************************************

void update_active_trackers(GstHailoTracker *hailotracker, guint property_id)
{
    for (auto &stream_id : hailotracker->active_streams)
    {
        switch (property_id)
        {
        case PROP_KALMAN_DIST_THR:
            HailoTracker::GetInstance().set_kalman_distance(stream_id,
                                                            hailotracker->tracker_params.kalman_distance);
            break;
        case PROP_IOU_THR:
            HailoTracker::GetInstance().set_iou_threshold(stream_id,
                                                          hailotracker->tracker_params.iou_threshold);
            break;
        case PROP_INIT_IOU_THR:
            HailoTracker::GetInstance().set_init_iou_threshold(stream_id,
                                                               hailotracker->tracker_params.init_iou_threshold);
            break;
        case PROP_KEEP_TRACKED_FRAMES:
            HailoTracker::GetInstance().set_keep_tracked_frames(stream_id,
                                                                hailotracker->tracker_params.keep_tracked_frames);
            break;
        case PROP_KEEP_NEW_FRAMES:
            HailoTracker::GetInstance().set_keep_new_frames(stream_id,
                                                            hailotracker->tracker_params.keep_new_frames);
            break;
        case PROP_KEEP_LOST_FRAMES:
            HailoTracker::GetInstance().set_keep_lost_frames(stream_id,
                                                             hailotracker->tracker_params.keep_lost_frames);
            break;
        case PROP_KEEP_PAST_METADATA:
            HailoTracker::GetInstance().set_keep_past_metadata(stream_id,
                                                               hailotracker->tracker_params.keep_past_metadata);
            break;
        default:
            break;
        }
    }
}

/* Handle setting properties */
void gst_hailo_tracker_set_property(GObject *object, guint property_id,
                                    const GValue *value, GParamSpec *pspec)
{
    GstHailoTracker *hailotracker = GST_HAILO_TRACKER(object);

    GST_DEBUG_OBJECT(hailotracker, "set_property");

    switch (property_id)
    {
    case PROP_CLASS_ID:
        hailotracker->class_id = g_value_get_int(value);
        break;
    case PROP_KALMAN_DIST_THR:
        hailotracker->tracker_params.kalman_distance = g_value_get_float(value);
        break;
    case PROP_IOU_THR:
        hailotracker->tracker_params.iou_threshold = g_value_get_float(value);
        break;
    case PROP_INIT_IOU_THR:
        hailotracker->tracker_params.init_iou_threshold = g_value_get_float(value);
        break;
    case PROP_KEEP_TRACKED_FRAMES:
        hailotracker->tracker_params.keep_tracked_frames = g_value_get_int(value);
        break;
    case PROP_KEEP_NEW_FRAMES:
        hailotracker->tracker_params.keep_new_frames = g_value_get_int(value);
        break;
    case PROP_KEEP_LOST_FRAMES:
        hailotracker->tracker_params.keep_lost_frames = g_value_get_int(value);
        break;
    case PROP_KEEP_PAST_METADATA:
        hailotracker->tracker_params.keep_past_metadata = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
    update_active_trackers(hailotracker, property_id);
}

/* Handle getting properties */
void gst_hailo_tracker_get_property(GObject *object, guint property_id,
                                    GValue *value, GParamSpec *pspec)
{
    GstHailoTracker *hailotracker = GST_HAILO_TRACKER(object);

    GST_DEBUG_OBJECT(hailotracker, "get_property");

    switch (property_id)
    {
    case PROP_CLASS_ID:
        g_value_set_int(value, hailotracker->class_id);
        break;
    case PROP_KALMAN_DIST_THR:
        g_value_set_float(value, hailotracker->tracker_params.kalman_distance);
        break;
    case PROP_IOU_THR:
        g_value_set_float(value, hailotracker->tracker_params.iou_threshold);
        break;
    case PROP_INIT_IOU_THR:
        g_value_set_float(value, hailotracker->tracker_params.init_iou_threshold);
        break;
    case PROP_KEEP_TRACKED_FRAMES:
        g_value_set_int(value, hailotracker->tracker_params.keep_tracked_frames);
        break;
    case PROP_KEEP_NEW_FRAMES:
        g_value_set_int(value, hailotracker->tracker_params.keep_new_frames);
        break;
    case PROP_KEEP_LOST_FRAMES:
        g_value_set_int(value, hailotracker->tracker_params.keep_lost_frames);
        break;
    case PROP_KEEP_PAST_METADATA:
        g_value_set_boolean(value, hailotracker->tracker_params.keep_past_metadata);
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
void gst_hailo_tracker_dispose(GObject *object)
{
    GstHailoTracker *hailotracker = GST_HAILO_TRACKER(object);

    GST_DEBUG_OBJECT(hailotracker, "dispose");

    /* clean up as possible.  may be called multiple times */
    g_free(hailotracker->current_stream_id);

    G_OBJECT_CLASS(gst_hailo_tracker_parent_class)->dispose(object);
}

/* Finishes releasing the remaining resources just before the object itself will be freed from memory,
   and therefore it will only be called once. */
void gst_hailo_tracker_finalize(GObject *object)
{
    GstHailoTracker *hailotracker = GST_HAILO_TRACKER(object);

    GST_DEBUG_OBJECT(hailotracker, "finalize");

    /* clean up object here */
    G_OBJECT_CLASS(gst_hailo_tracker_parent_class)->finalize(object);
}

/* Called when the element starts processing. Allows opening external resources. */
static gboolean
gst_hailo_tracker_start(GstBaseTransform *trans)
{
    GstHailoTracker *hailotracker = GST_HAILO_TRACKER(trans);

    GST_DEBUG_OBJECT(hailotracker, "start");

    return TRUE;
}

/* Called when the element stops processing. Allows closing external resources. */
static gboolean
gst_hailo_tracker_stop(GstBaseTransform *trans)
{
    GstHailoTracker *hailotracker = GST_HAILO_TRACKER(trans);
    for (std::string stream_id : hailotracker->active_streams)
    {
        HailoTracker::GetInstance().remove_jde_tracker(stream_id);
    }

    GST_DEBUG_OBJECT(hailotracker, "stop");

    return TRUE;
}

/* Set negotiated caps and video info. */
static gboolean
gst_hailo_tracker_set_info(GstVideoFilter *filter, GstCaps *incaps,
                           GstVideoInfo *in_info, GstCaps *outcaps, GstVideoInfo *out_info)
{
    GstHailoTracker *hailotracker = GST_HAILO_TRACKER(filter);

    GST_DEBUG_OBJECT(hailotracker, "set_info");

    return TRUE;
}

//******************************************************************
// FRAME TRANSFORMATION
//******************************************************************
/* Transform a video frame in place. This is where the actual tracking filter is applied. */
static GstFlowReturn
gst_hailo_tracker_transform_frame_ip(GstVideoFilter *filter, GstVideoFrame *frame)
{
    GstHailoTracker *hailotracker = GST_HAILO_TRACKER(filter);
    GstBuffer *buffer = frame->buffer;
    HailoROIPtr hailo_roi = get_hailo_main_roi(buffer, true);

    std::vector<HailoDetectionPtr> detections;
    for (auto obj : hailo_roi->get_objects_typed(HAILO_DETECTION))
    {
        HailoDetectionPtr detection = std::dynamic_pointer_cast<HailoDetection>(obj);
        if ((hailotracker->class_id == -1) || (detection->get_class_id() == hailotracker->class_id))
        {
            detections.push_back(detection);
            hailo_roi->remove_object(detection);
        }
    }

    // Swap the detections in the roi with just the online tracked detections
    GST_OBJECT_LOCK(hailotracker);
    std::vector<HailoDetectionPtr> online_detection_ptrs = HailoTracker::GetInstance().update(std::string(hailotracker->current_stream_id), detections);
    hailo_common::add_detection_pointers(hailo_roi, online_detection_ptrs);
    GST_OBJECT_UNLOCK(hailotracker);

    GST_DEBUG_OBJECT(hailotracker, "transform_frame_ip");
    return GST_FLOW_OK;
}

//******************************************************************
// EVENT HANDLING
//******************************************************************
/* Handle sink events. */
static gboolean
gst_hailo_tracker_sink_event(GstBaseTransform *trans,
                             GstEvent *event)
{
    GstHailoTracker *hailotracker = GST_HAILO_TRACKER(trans);
    switch (GST_EVENT_TYPE(event))
    {
    case GST_EVENT_STREAM_START:
        const gchar *stream_id;
        gst_event_parse_stream_start(event, &stream_id);
        if (!stream_id)
        {
            GST_ERROR_OBJECT(hailotracker, "No stream ID");
            return FALSE;
        }
        else
        {
            GST_DEBUG_OBJECT(hailotracker, "filtering stream %s", stream_id);
            hailotracker->current_stream_id = strdup(stream_id);
            // If streamid is new create a new JDETracker
            if (std::find(hailotracker->active_streams.begin(),
                          hailotracker->active_streams.end(),
                          std::string(hailotracker->current_stream_id)) == hailotracker->active_streams.end())
            {
                HailoTracker::GetInstance().add_jde_tracker(std::string(hailotracker->current_stream_id), hailotracker->tracker_params);
                hailotracker->active_streams.emplace_back(std::string(hailotracker->current_stream_id));
            }
        }
    default:
        return GST_BASE_TRANSFORM_CLASS(gst_hailo_tracker_parent_class)->sink_event(trans, event);
    }
}

/* Handle src events. */
static gboolean
gst_hailo_tracker_src_event(GstBaseTransform *trans,
                            GstEvent *event)
{
    GstHailoTracker *hailotracker = GST_HAILO_TRACKER(trans);
    switch (GST_EVENT_TYPE(event))
    {
    case GST_EVENT_CUSTOM_UPSTREAM:
        const GstStructure *event_structure;
        event_structure = gst_event_get_structure(event);
        // OCR label event
        if (event_structure && gst_event_has_name(event, OCR_EVENT_NAME))
        {
            gboolean event_handled = gst_hailo_tracker_ocr_event(hailotracker, event_structure);
            return event_handled;
        }
        return true;
    default:
        return GST_BASE_TRANSFORM_CLASS(gst_hailo_tracker_parent_class)->src_event(trans, event);
    }
}

static gboolean
gst_hailo_tracker_ocr_event(GstHailoTracker *hailotracker,
                            const GstStructure *event_structure)
{
    GST_OBJECT_LOCK(hailotracker);
    gint track_id;
    gfloat confidence;
    gst_structure_get_int(event_structure, "track_id", &track_id);
    const gchar *ocr_label = gst_structure_get_string(event_structure, "label");
    gst_structure_get(event_structure, "confidence", G_TYPE_FLOAT, &confidence, NULL);

    HailoTracker::GetInstance().add_object_to_track(std::string(hailotracker->current_stream_id),
                                                    track_id,
                                                    std::make_shared<HailoClassification>("ocr", ocr_label, confidence));
    GST_OBJECT_UNLOCK(hailotracker);
    return true;
}
