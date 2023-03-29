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
#include "gst_hailo_stream_meta.hpp"
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
static gboolean gst_hailo_tracker_stop(GstBaseTransform *trans);
static GstFlowReturn gst_hailo_tracker_transform_frame_ip(GstVideoFilter *filter, GstVideoFrame *frame);
static gboolean gst_hailo_tracker_sink_event(GstBaseTransform *trans, GstEvent *event);

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
    PROP_STD_WEIGHT_POSITION,
    PROP_STD_WEIGHT_POSITION_BOX,
    PROP_STD_WEIGHT_VELOCITY,
    PROP_STD_WEIGHT_VELOCITY_BOX,
    PROP_DEBUG,
    PROP_HAILO_OBJECTS_BLACKLIST,
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
    g_object_class_install_property(gobject_class, PROP_DEBUG,
                                    g_param_spec_boolean("debug", "debug",
                                                         "Enable output of new and lost tracked objects.\n\
                                                          When set - new and lost tracked objects are outputted. \n\
                                                          These detections will have a classification type 'tracking' with its state. \n\
                                                          This is useful for debugging and testing tracker parameters.",
                                                         DEFAULT_DEBUG,
                                                         (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
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
                                                         "Past metadata are the sub objects on the current tracked object. \n\
                                    When set (default) - past metadata is kept on tracked objects. \n\
                                    When unset - past metadata is removed from tracked objects. \n\
                                    There are some objects that cannot be consistently scaled to match the new location of the tracked object (Like landmarks, mask , matrix). \n\
                                    For example, if a face is rotated 90 degrees, the landmarks will not be in the correct location \n\
                                    So even when this property is set to True, these metadata types will not be kept.",
                                                         DEFAULT_KEEP_PAST_METADATA,
                                                         (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_STD_WEIGHT_POSITION,
                                    g_param_spec_float("std-weight-position", "std weight position",
                                                       "Set standard deviation weight for position prediction covariance matrix. \n\
                                    This value is used to weight the kalman filter prediction step for detection box position i.e. the box x, y. \n\
                                    Smaller value give more weight to the prediction over the actual measurmet. \n\
                                    For more snappy tracker set higher value for more stabilized tracker set lower value. \n\
                                    To see difference you will have to change scale i.e. 0.1, 0.01, 0.001 etc.",
                                                       0.0, 1.0, DEFAULT_STD_WEIGHT_POSITION,
                                                       (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_STD_WEIGHT_POSITION_BOX,
                                    g_param_spec_float("std-weight-position-box", "std weight position box",
                                                       "Set standard deviation weight for box width and height prediction covariance matrix. \n\
                                    This value is used to weight the kalman filter prediction step for detection box width and height i.e. the box h, w. \n\
                                    Smaller value give more weight to the prediction over the actual measurmet. \n\
                                    For more snappy tracker set higher value for more stabilized tracker set lower value. \n\
                                    To see difference you will have to change scale i.e. 0.1, 0.01, 0.001 etc.",
                                                       0.0, 1.0, DEFAULT_STD_WEIGHT_POSITION_BOX,
                                                       (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_STD_WEIGHT_VELOCITY,
                                    g_param_spec_float("std-weight-velocity", "std weight velocity",
                                                       "Set standard deviation weight for velocity prediction covariance matrix. \n\
                                    This value is used to weight the kalman filter prediction step for detection box velocity i.e. the box vx, vy. \n\
                                    Smaller value give more weight to the prediction over the actual measurmet. \n\
                                    For more snappy tracker set higher value for more stabilized tracker set lower value. \n\
                                    To see difference you will have to change scale i.e. 0.1, 0.01, 0.001 etc.",
                                                       0.0, 1.0, DEFAULT_STD_WEIGHT_VELOCITY,
                                                       (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_STD_WEIGHT_VELOCITY_BOX,
                                    g_param_spec_float("std-weight-velocity-box", "std weight velocity box",
                                                       "Set standard deviation weight for box width and height change velocity prediction covariance matrix. \n\
                                    This value is used to weight the kalman filter prediction step for detection box width and height changes i.e. the box vh, vw. \n\
                                    Smaller value give more weight to the prediction over the actual measurmet. \n\
                                    For more snappy tracker set higher value for more stabilized tracker set lower value. \n\
                                    To see difference you will have to change scale i.e. 0.1, 0.01, 0.001 etc.",
                                                       0.0, 1.0, DEFAULT_STD_WEIGHT_VELOCITY_BOX,
                                                       (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_HAILO_OBJECTS_BLACKLIST,
                                    g_param_spec_string("hailo-objects-blacklist", "Hailo objects blacklist",
                                                        "list of hailo objects types that the tracker should not keep, comma separated", "hailo_landmarks,hailo_depth_mask,hailo_class_mask",
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    // Set virtual functions
    gobject_class->dispose = gst_hailo_tracker_dispose;
    base_transform_class->stop = GST_DEBUG_FUNCPTR(gst_hailo_tracker_stop);
    video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR(gst_hailo_tracker_transform_frame_ip);
    base_transform_class->sink_event = GST_DEBUG_FUNCPTR(gst_hailo_tracker_sink_event);
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
    hailotracker->tracker_params.std_weight_position = DEFAULT_STD_WEIGHT_POSITION;
    hailotracker->tracker_params.std_weight_position_box = DEFAULT_STD_WEIGHT_POSITION_BOX;
    hailotracker->tracker_params.std_weight_velocity = DEFAULT_STD_WEIGHT_VELOCITY;
    hailotracker->tracker_params.std_weight_velocity_box = DEFAULT_STD_WEIGHT_VELOCITY_BOX;
    hailotracker->tracker_params.debug = DEFAULT_DEBUG;
    hailotracker->tracker_params.hailo_objects_blacklist = DEFAULT_HAILO_OBJECTS_BLACKLIST;
}

//******************************************************************
// PROPERTY HANDLING
//******************************************************************

static std::string get_tracker_name(GstHailoTracker *hailotracker, std::string stream_id)
{
    std::string element_name = GST_ELEMENT_NAME(GST_ELEMENT_CAST(hailotracker));
    return std::string(element_name + "_" + stream_id);
}

void update_active_trackers(GstHailoTracker *hailotracker, guint property_id)
{
    for (auto &stream_id : hailotracker->active_streams)
    {
        std::string tracker_name = get_tracker_name(hailotracker, stream_id);
        switch (property_id)
        {
        case PROP_KALMAN_DIST_THR:
            HailoTracker::GetInstance().set_kalman_distance(tracker_name,
                                                            hailotracker->tracker_params.kalman_distance);
            break;
        case PROP_IOU_THR:
            HailoTracker::GetInstance().set_iou_threshold(tracker_name,
                                                          hailotracker->tracker_params.iou_threshold);
            break;
        case PROP_INIT_IOU_THR:
            HailoTracker::GetInstance().set_init_iou_threshold(tracker_name,
                                                               hailotracker->tracker_params.init_iou_threshold);
            break;
        case PROP_KEEP_TRACKED_FRAMES:
            HailoTracker::GetInstance().set_keep_tracked_frames(tracker_name,
                                                                hailotracker->tracker_params.keep_tracked_frames);
            break;
        case PROP_KEEP_NEW_FRAMES:
            HailoTracker::GetInstance().set_keep_new_frames(tracker_name,
                                                            hailotracker->tracker_params.keep_new_frames);
            break;
        case PROP_KEEP_LOST_FRAMES:
            HailoTracker::GetInstance().set_keep_lost_frames(tracker_name,
                                                             hailotracker->tracker_params.keep_lost_frames);
            break;
        case PROP_KEEP_PAST_METADATA:
            HailoTracker::GetInstance().set_keep_past_metadata(tracker_name,
                                                               hailotracker->tracker_params.keep_past_metadata);
            break;
        case PROP_STD_WEIGHT_POSITION:
            HailoTracker::GetInstance().set_std_weight_position(tracker_name,
                                                                hailotracker->tracker_params.std_weight_position);
            break;
        case PROP_STD_WEIGHT_POSITION_BOX:
            HailoTracker::GetInstance().set_std_weight_position_box(tracker_name,
                                                                    hailotracker->tracker_params.std_weight_position_box);
            break;
        case PROP_STD_WEIGHT_VELOCITY:
            HailoTracker::GetInstance().set_std_weight_velocity(tracker_name,
                                                                hailotracker->tracker_params.std_weight_velocity);
            break;
        case PROP_STD_WEIGHT_VELOCITY_BOX:
            HailoTracker::GetInstance().set_std_weight_velocity_box(tracker_name,
                                                                    hailotracker->tracker_params.std_weight_velocity_box);
            break;
        case PROP_HAILO_OBJECTS_BLACKLIST:
            HailoTracker::GetInstance().set_hailo_objects_blacklist(tracker_name,
                                                                    hailotracker->tracker_params.hailo_objects_blacklist);
            break;
        case PROP_DEBUG:
            HailoTracker::GetInstance().set_debug(tracker_name,
                                                  hailotracker->tracker_params.debug);
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
    case PROP_STD_WEIGHT_POSITION:
        hailotracker->tracker_params.std_weight_position = g_value_get_float(value);
        break;
    case PROP_STD_WEIGHT_POSITION_BOX:
        hailotracker->tracker_params.std_weight_position_box = g_value_get_float(value);
        break;
    case PROP_STD_WEIGHT_VELOCITY:
        hailotracker->tracker_params.std_weight_velocity = g_value_get_float(value);
        break;
    case PROP_STD_WEIGHT_VELOCITY_BOX:
        hailotracker->tracker_params.std_weight_velocity_box = g_value_get_float(value);
        break;
    case PROP_DEBUG:
        hailotracker->tracker_params.debug = g_value_get_boolean(value);
        break;
    case PROP_HAILO_OBJECTS_BLACKLIST:
    {
        std::string blacklist = g_value_get_string(value);
        std::vector<hailo_object_t> hailo_objects_blacklist_vec = {};
        char delimiter = ',';
        // check if the last char of the string is the delimiter if not add the delimiter
        if (blacklist[blacklist.size() - 1] != delimiter)
        {
            blacklist += delimiter;
        }
        // remove spaces if there are any
        blacklist.erase(std::remove(blacklist.begin(), blacklist.end(), ' '), blacklist.end());

        // fill the vector with the hailo objects types
        size_t pos = 0;
        std::string token;
        while ((pos = blacklist.find(delimiter)) != std::string::npos)
        {
            token = blacklist.substr(0, pos);
            hailo_objects_blacklist_vec.push_back(hailo_object_type_from_string(token));
            blacklist.erase(0, pos + 1);
        }
        hailotracker->tracker_params.hailo_objects_blacklist = std::move(hailo_objects_blacklist_vec);
        break;
    }

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
    case PROP_STD_WEIGHT_POSITION:
        g_value_set_float(value, hailotracker->tracker_params.std_weight_position);
        break;
    case PROP_STD_WEIGHT_POSITION_BOX:
        g_value_set_float(value, hailotracker->tracker_params.std_weight_position_box);
        break;
    case PROP_STD_WEIGHT_VELOCITY:
        g_value_set_float(value, hailotracker->tracker_params.std_weight_velocity);
        break;
    case PROP_STD_WEIGHT_VELOCITY_BOX:
        g_value_set_float(value, hailotracker->tracker_params.std_weight_velocity_box);
        break;
    case PROP_DEBUG:
        g_value_set_boolean(value, hailotracker->tracker_params.debug);
        break;
    case PROP_HAILO_OBJECTS_BLACKLIST:
    {
        // create string of the blacklist from the vector
        std::string blacklist = "";
        for (auto &hailo_object : hailotracker->tracker_params.hailo_objects_blacklist)
        {
            blacklist += hailo_object_type_to_string(hailo_object);
            blacklist += ",";
        }
        g_value_set_string(value, blacklist.c_str());
        break;
    }
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

/* Called when the element stops processing. Allows closing external resources. */
static gboolean
gst_hailo_tracker_stop(GstBaseTransform *trans)
{
    GstHailoTracker *hailotracker = GST_HAILO_TRACKER(trans);
    for (std::string stream_id : hailotracker->active_streams)
    {
        std::string tracker_name = get_tracker_name(hailotracker, stream_id);
        HailoTracker::GetInstance().remove_jde_tracker(tracker_name);
    }

    GST_DEBUG_OBJECT(hailotracker, "stop");

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

    gchar *stream_id = hailotracker->current_stream_id;
    GstHailoStreamMeta *stream_meta = gst_buffer_get_hailo_stream_meta(buffer);
    if (stream_meta)
    {
        // Get the input stream name from the stream metadata on the buffer (If there is one)
        stream_id = gst_buffer_get_hailo_stream_meta(buffer)->stream_id;
    }

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
    std::string tracker_name = get_tracker_name(hailotracker, std::string(stream_id));
    std::vector<HailoDetectionPtr> online_detection_ptrs = HailoTracker::GetInstance().update(tracker_name, detections);

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
                std::string tracker_name = get_tracker_name(hailotracker, std::string(hailotracker->current_stream_id));
                HailoTracker::GetInstance().add_jde_tracker(tracker_name, hailotracker->tracker_params);
                hailotracker->active_streams.emplace_back(std::string(hailotracker->current_stream_id));
            }
        }
    default:
        return GST_BASE_TRANSFORM_CLASS(gst_hailo_tracker_parent_class)->sink_event(trans, event);
    }
}
