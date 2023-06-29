/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include <gst/gst.h>
#include <gst/video/video.h>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <vector>
#include <map>
#include <typeinfo>
#include "common/image.hpp"
#include "cropping/gsthailobasecropper.hpp"
#include "gst_hailo_cropping_meta.hpp"
#include "gst_hailo_stream_meta.hpp"
#include "hailo_objects.hpp"
#include "hailo_common.hpp"
#include "gst_hailo_meta.hpp"
#ifdef HAILO15_TARGET
    #include "dsp/gsthailodsp.h"
    #include "dsp/gsthailodspbufferpoolutils.hpp"
#endif

GST_DEBUG_CATEGORY_STATIC(gst_hailo_basecropper_debug);
#define GST_CAT_DEFAULT gst_hailo_basecropper_debug

enum
{
    PROP_0,
    PROP_USE_INTERNAL_OFFSET,
    PROP_DROP_UNCROPPED_BUFFERS,
    PROP_CROPPING_PERIOD,
    PROP_FILTER_STREAMS,
    #ifdef HAILO15_TARGET
    PROP_USE_DSP,
    PROP_POOL_SIZE,
    #endif
};

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
                                                                   GST_PAD_SINK,
                                                                   GST_PAD_ALWAYS,
                                                                   gst_caps_from_string(HAILO_BASE_CROPPER_VIDEO_CAPS));

// We define two source pad templates, one for the main stream and one for the cropped stream.
// Altough they are the same, we need to define them separately to support a proper caps negotiation in some platforms.
static GstStaticPadTemplate main_src_factory = GST_STATIC_PAD_TEMPLATE("src_0",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_ALWAYS,
                                                                  gst_caps_from_string(HAILO_BASE_CROPPER_VIDEO_CAPS));

static GstStaticPadTemplate crop_src_factory = GST_STATIC_PAD_TEMPLATE("src_1",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_ALWAYS,
                                                                  gst_caps_from_string(HAILO_BASE_CROPPER_VIDEO_CAPS));
#define _debug_init \
    GST_DEBUG_CATEGORY_INIT(gst_hailo_basecropper_debug, "hailobasecropper", 0, "hailobasecropper element");
#define gst_hailo_basecropper_parent_class parent_class
G_DEFINE_ABSTRACT_TYPE_WITH_CODE(GstHailoBaseCropper, gst_hailo_basecropper, GST_TYPE_ELEMENT, _debug_init);

static void gst_hailo_basecropper_set_property(GObject *object,
                                               guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_hailo_basecropper_get_property(GObject *object,
                                               guint prop_id, GValue *value, GParamSpec *pspec);

static gboolean gst_hailo_basecropper_sink_event(GstPad *pad,
                                                 GstObject *parent, GstEvent *event);
static gboolean gst_hailo_basecropper_sink_query(GstPad *pad,
                                                 GstObject *parent, GstQuery *query);
static gboolean gst_hailo_basecropper_src_query(GstPad *pad,
                                                 GstObject *parent, GstQuery *query);
static GstFlowReturn gst_hailo_basecropper_chain(GstPad *pad,
                                                 GstObject *parent, GstBuffer *buf);

static void gst_hailo_basecropper_dispose(GObject *object);

static gboolean gst_hailo_basecropper_decide_allocation(GstHailoBaseCropper *hailo_basecropper, GstQuery *query);

static GstBuffer* gst_hailo_basecropper_allocate_new_buffer(GstHailoBaseCropper *hailo_basecropper, size_t buffer_size);

#ifdef HAILO15_TARGET
static gboolean dsp_crop_and_resize(GstHailoBaseCropper *hailo_basecropper, cv::Rect crop_rect, std::shared_ptr<HailoMat> resized_image, \
                                    GstBuffer *input_buffer, GstVideoInfo *input_video_info, GstBuffer *output_buffer,  GstVideoInfo *output_video_info);
static gboolean gst_hailo_basecropper_propose_allocation(GstHailoBaseCropper *hailo_basecropper, GstPad *pad, GstQuery *query);
#endif

static void
gst_hailo_basecropper_class_init(GstHailoBaseCropperClass *klass)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;

    gobject_class = (GObjectClass *)klass;
    gstelement_class = (GstElementClass *)klass;

    gobject_class->set_property = gst_hailo_basecropper_set_property;
    gobject_class->get_property = gst_hailo_basecropper_get_property;
    gobject_class->dispose = GST_DEBUG_FUNCPTR(gst_hailo_basecropper_dispose);

    g_object_class_install_property(gobject_class, PROP_USE_INTERNAL_OFFSET,
                                    g_param_spec_boolean("internal-offset", "Internal Offset",
                                                         "Whether to use Gstreamer offset of internal offset. \nNOTE: If using file sources, Gstreamer does not generate offsets for buffers, so this property should be set to true in such cases.", false,
                                                         (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_DROP_UNCROPPED_BUFFERS,
                                    g_param_spec_boolean("drop-uncropped-buffers", "Drop Uncropped Buffers",
                                                         "If true, then this element will drop buffers that have no croppable ROIs. Default false.", false,
                                                         (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_CROPPING_PERIOD,
                                    g_param_spec_uint("cropping-period", "Cropping Period",
                                                      "Period of of how often to crop buffers. Can be thought of as 'cropping every x buffers'. Default 1 (every buffer)",
                                                      1, G_MAXINT, 1,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
    g_object_class_install_property(gobject_class, PROP_FILTER_STREAMS,
                                    gst_param_spec_array("filter-streams", "Filter Streams",
                                                         "Filter specific streams to crop. Streams are filtered by GstHailoStreamMeta (see hailoroundrobin element). Default behavior crops all streams. \nExample usage: filter-streams=\'<sink_1, sink_3>\'",
                                                         g_param_spec_string("filter-stream-name", "Filter stream name",
                                                                             "Filter stream", "",
                                                                             (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)),
                                                         (GParamFlags)(G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS)));

    # ifdef HAILO15_TARGET
    g_object_class_install_property(gobject_class, PROP_USE_DSP,
                                    g_param_spec_boolean("use-dsp", "Use DSP",
                                                         "Whether to use DSP for cropping. Default true.", true,
                                                         (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(gobject_class, PROP_POOL_SIZE,
                                    g_param_spec_uint("pool-size", "Pool Size",
                                                      "Size of the pool of buffers to use for cropping. Default 10",
                                                      1, G_MAXINT, 10,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
    # endif

    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&main_src_factory));
    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&crop_src_factory));
    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&sink_factory));

    klass->prepare_crops = nullptr;
    klass->resize = nullptr;
}

static void
gst_hailo_basecropper_init(GstHailoBaseCropper *hailo_basecropper)
{
    hailo_basecropper->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");
    gst_pad_set_event_function(hailo_basecropper->sinkpad, GST_DEBUG_FUNCPTR(gst_hailo_basecropper_sink_event));
    gst_pad_set_chain_function(hailo_basecropper->sinkpad, GST_DEBUG_FUNCPTR(gst_hailo_basecropper_chain));
    gst_pad_set_query_function(hailo_basecropper->sinkpad, GST_DEBUG_FUNCPTR(gst_hailo_basecropper_sink_query));
    GST_PAD_SET_PROXY_CAPS(hailo_basecropper->sinkpad);
    gst_element_add_pad(GST_ELEMENT(hailo_basecropper), hailo_basecropper->sinkpad);

    hailo_basecropper->srcpad_main = gst_pad_new_from_static_template(&main_src_factory, "src_0");
    GST_PAD_SET_PROXY_CAPS(hailo_basecropper->srcpad_main);
    gst_element_add_pad(GST_ELEMENT(hailo_basecropper), hailo_basecropper->srcpad_main);

    hailo_basecropper->srcpad_crop = gst_pad_new_from_static_template(&crop_src_factory, "src_1");
    GST_PAD_SET_PROXY_CAPS(hailo_basecropper->srcpad_crop);
    gst_element_add_pad(GST_ELEMENT(hailo_basecropper), hailo_basecropper->srcpad_crop);
    gst_pad_set_query_function(hailo_basecropper->srcpad_crop, GST_DEBUG_FUNCPTR(gst_hailo_basecropper_src_query));

    // Set default values.
    #ifdef HAILO15_TARGET
    hailo_basecropper->use_dsp = true;
    hailo_basecropper->bufferpool_max_size = 10;
    hailo_basecropper->bufferpool_min_size = 1;
    #endif
    hailo_basecropper->use_internal_offset = false;
    hailo_basecropper->internal_offset = 0;
    hailo_basecropper->cropping_period = 1;
    hailo_basecropper->num_streams_to_filter = 0;
    hailo_basecropper->drop_uncropped_buffers = false;
    hailo_basecropper->buffer_pool = NULL;
    hailo_basecropper->stream_ids_buff_offset.clear();
    for (uint i = 0; i < GST_HAILO_CROPPER_MAX_FILTER_STREAMS; i++)
        hailo_basecropper->filter_streams[i] = "";
}

#ifdef HAILO15_TARGET
static gboolean
gst_hailo_basecropper_propose_allocation(GstHailoBaseCropper *hailo_basecropper, GstPad *pad, GstQuery *query)
{
    gboolean ret = gst_pad_peer_query(hailo_basecropper->srcpad_main, query);
    if (!ret)
        GST_DEBUG_OBJECT(hailo_basecropper, "Peer query allocation failed");
    gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, NULL);

    return true;
}
#endif

static void gst_hailo_basecropper_dispose(GObject *object)
{
    GstHailoBaseCropper *hailo_basecropper = GST_HAILO_BASE_CROPPER(object);
    GST_INFO_OBJECT(hailo_basecropper, "Performing Dispose");

    if (hailo_basecropper->buffer_pool)
    {
        GST_DEBUG_OBJECT(hailo_basecropper, "Unreffing buffer pool");
        gst_object_unref(hailo_basecropper->buffer_pool);
        hailo_basecropper->buffer_pool = NULL;
    }

    G_OBJECT_CLASS(gst_hailo_basecropper_parent_class)->dispose(object);
}

static gboolean
gst_hailo_basecropper_decide_allocation(GstHailoBaseCropper *hailo_basecropper, GstQuery *query)
{
    gboolean ret = TRUE;

    #ifdef HAILO15_TARGET
    if (!hailo_basecropper->use_dsp)
        return ret;

    GST_DEBUG_OBJECT(hailo_basecropper, "Performing decide allocation");

    GstElement *element = GST_ELEMENT_CAST(hailo_basecropper);
    hailo_basecropper->buffer_pool = gst_create_hailo_dsp_bufferpool_from_allocation_query(element, query, hailo_basecropper->bufferpool_min_size, hailo_basecropper->bufferpool_max_size, 0);
    if (hailo_basecropper->buffer_pool == NULL)
    {
        GST_ERROR_OBJECT(hailo_basecropper, "Decide Allocation - Failed to create buffer pool");
        return FALSE;
    }

    GST_INFO_OBJECT(hailo_basecropper, "Decide allocation - hailo buffer pool created");

    #endif
    return ret;
}

static void
set_filter_streams(GstHailoBaseCropper *hailo_basecropper, const GValue *value)
{
    // Insert the elements of Gvalue represents filter streams into the char array 'filter_streams'
    if (value == NULL)
    {
        GST_ERROR("initialization of element property filter_streams: value is NULL");
        return;
    }
    guint len = gst_value_array_get_size(value);
    if (len > 0 && len < GST_HAILO_CROPPER_MAX_FILTER_STREAMS)
    {
        for (uint i = 0; i < len; i++)
        {
            const GValue *val = gst_value_array_get_value(value, i);
            hailo_basecropper->filter_streams[i] = g_strdup(g_value_get_string(val));
        }
        hailo_basecropper->num_streams_to_filter = len;
    }
    else
    {
        std::cerr << "initialization of element property filter_streams: value is " << len << " and must be between 0 to " << GST_HAILO_CROPPER_MAX_FILTER_STREAMS << std::endl;
    }
    return;
}
static void
get_filter_streams(GstHailoBaseCropper *hailo_basecropper, GValue *value)
{
    // Insert the elements of Gvalue represents input streams into the char array 'input_streams'
    if (value == NULL)
    {
        GST_ERROR("initialization of element property filter_streams: value is NULL");
        return;
    }
    guint len = gst_value_array_get_size(value);
    if (len > 0)
    {
        GST_ERROR("initialization of element property filter_streams: value an not empty array");
        return;
    }
    for (uint i = 0; i < hailo_basecropper->num_streams_to_filter; i++)
    {
        GValue val = G_VALUE_INIT;
        g_value_init(&val, G_TYPE_STRING);
        g_value_set_string(&val, hailo_basecropper->filter_streams[i]);
        gst_value_array_append_and_take_value(value, &val);
    }
}

static void
gst_hailo_basecropper_set_property(GObject *object, guint prop_id,
                                   const GValue *value, GParamSpec *pspec)
{
    GstHailoBaseCropper *hailo_basecropper = GST_HAILO_BASE_CROPPER(object);

    switch (prop_id)
    {
    case PROP_USE_INTERNAL_OFFSET:
        hailo_basecropper->use_internal_offset = g_value_get_boolean(value);
        break;
    case PROP_DROP_UNCROPPED_BUFFERS:
        hailo_basecropper->drop_uncropped_buffers = g_value_get_boolean(value);
        break;
    case PROP_CROPPING_PERIOD:
        hailo_basecropper->cropping_period = g_value_get_uint(value);
        break;
    case PROP_FILTER_STREAMS:
        set_filter_streams(hailo_basecropper, value);
        break;
    #ifdef HAILO15_TARGET
    case PROP_USE_DSP:
        hailo_basecropper->use_dsp = g_value_get_boolean(value);
        break;
    case PROP_POOL_SIZE:
        hailo_basecropper->bufferpool_max_size = g_value_get_uint(value);
        break;
    #endif
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_hailo_basecropper_get_property(GObject *object, guint prop_id,
                                   GValue *value, GParamSpec *pspec)
{
    GstHailoBaseCropper *hailo_basecropper = GST_HAILO_BASE_CROPPER(object);

    switch (prop_id)
    {
    case PROP_USE_INTERNAL_OFFSET:
        g_value_set_boolean(value, hailo_basecropper->use_internal_offset);
        break;
    case PROP_DROP_UNCROPPED_BUFFERS:
        g_value_set_boolean(value, hailo_basecropper->drop_uncropped_buffers);
        break;
    case PROP_CROPPING_PERIOD:
        g_value_set_uint(value, hailo_basecropper->cropping_period);
        break;
    case PROP_FILTER_STREAMS:
        get_filter_streams(hailo_basecropper, value);
        break;
    #ifdef HAILO15_TARGET
    case PROP_USE_DSP:
        g_value_set_boolean(value, hailo_basecropper->use_dsp);
        break;
    case PROP_POOL_SIZE:
        g_value_set_uint(value, hailo_basecropper->bufferpool_max_size);
        break;
    #endif
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/**
 * Retrieves wanted caps from downstream elements and sets them.
 *
 * @param[in] hailo_basecropper      Pointer to the element, used to send caps query on the crop pad.
 * @return Upon success, returns true. Otherwise, returns false.
 */
static gboolean
gst_crop_scale_setcaps(GstHailoBaseCropper *hailo_basecropper)
{
    GstCaps *caps_result, *outcaps = NULL;
    GstQuery *query = NULL;

    // Create new caps Query;
    query = gst_query_new_caps(GST_CAPS_ANY);

    // Query the peer pad of cropped pad to obtain wanted resolution.
    gst_pad_peer_query(hailo_basecropper->srcpad_crop, query);
    gst_query_parse_caps_result(query, &caps_result);

    // Fixate the caps
    // gst_caps_fixate takes ownership of caps_result, no need to unref it.
    outcaps = gst_caps_fixate(caps_result);

    // Set The caps, unref all objects and return.
    gboolean ret = gst_pad_set_caps(hailo_basecropper->srcpad_crop, outcaps);
    gst_query_unref(query);

    return ret;
}

static gboolean
gst_hailo_handle_caps_query(GstPad *pad, GstQuery *query)
{
    GstCaps *caps_result, *allowed_caps, *qcaps;
    /* we should report the supported caps here which are all */
    allowed_caps = gst_pad_get_pad_template_caps(pad);
    gst_query_parse_caps(query, &qcaps);

    if (qcaps)
    {
        // caps query - intersect template caps (allowed caps) with incomming caps query
        caps_result = gst_caps_intersect(allowed_caps, qcaps);
    }
    else
    {
        // no caps query - return template caps
        caps_result = allowed_caps;
    }

    // set the caps result
    gst_query_set_caps_result(query, caps_result);
    gst_caps_unref(caps_result);

    return TRUE;
}

static gboolean
gst_hailo_basecropper_src_query(GstPad *pad,
                                 GstObject *parent, GstQuery *query)
{
    gboolean ret;

    switch (GST_QUERY_TYPE(query))
    {
    case GST_QUERY_CAPS:
    {
        ret = gst_hailo_handle_caps_query(pad, query);
        break;
    }
    default:
    {
        /* just call the default handler */
        ret = gst_pad_query_default(pad, parent, query);
        break;
    }
    }
    return ret;
}

static gboolean
gst_hailo_basecropper_sink_query(GstPad *pad,
                                 GstObject *parent, GstQuery *query)
{
    gboolean ret;
    GstHailoBaseCropper *hailo_basecropper = GST_HAILO_BASE_CROPPER(parent);

    switch (GST_QUERY_TYPE(query))
    {
    case GST_QUERY_CAPS:
    {
        ret = gst_hailo_handle_caps_query(pad, query);
        break;
    }
    case GST_QUERY_ALLOCATION:
    {
        GST_DEBUG_OBJECT(hailo_basecropper, "Received allocation query from sinkpad in hailo_basecropper");
    #ifdef HAILO15_TARGET
        ret = gst_hailo_basecropper_propose_allocation(hailo_basecropper, pad, query);
        if (!ret)
            GST_DEBUG_OBJECT(hailo_basecropper, "Failed to query peer srcpad_main");
    #else
        ret = gst_pad_query_default(pad, parent, query);
    #endif
        break;
    }
    case GST_QUERY_ACCEPT_CAPS:
    {
        GstCaps *caps;
        gst_query_parse_accept_caps(query, &caps);
        gst_query_set_accept_caps_result(query, true);
        ret = TRUE;
        break;
    }
    default:
    {
        /* just call the default handler */
        ret = gst_pad_query_default(pad, parent, query);
        break;
    }
    }
    return ret;
}

static gboolean
gst_hailo_basecropper_sink_event(GstPad *pad, GstObject *parent,
                                 GstEvent *event)
{
    GstHailoBaseCropper *hailo_basecropper = GST_HAILO_BASE_CROPPER(parent);
    gboolean ret;

    GST_LOG_OBJECT(hailo_basecropper, "Received %s event: %" GST_PTR_FORMAT,
                   GST_EVENT_TYPE_NAME(event), event);

    switch (GST_EVENT_TYPE(event))
    {

    case GST_EVENT_CAPS:
    {
        GstCaps *caps, *crop_caps;

        gst_event_parse_caps(event, &caps);

        ret = gst_pad_set_caps(hailo_basecropper->srcpad_main, caps);
        if (!ret)
        {
            GST_ERROR_OBJECT(hailo_basecropper, "Unable to set caps on main srcpad");
            return FALSE;
        }

        ret = gst_crop_scale_setcaps(hailo_basecropper);
        if (!ret)
        {
            GST_ERROR_OBJECT(hailo_basecropper, "Unable to set caps on crop srcpad");
            return FALSE;
        }

        // Get caps from the crop pad
        crop_caps = gst_pad_get_current_caps(hailo_basecropper->srcpad_crop);

        // Create new allocation query with the crop caps
        GstQuery *crop_query = gst_query_new_allocation(crop_caps, FALSE);

        // Propose allocation to the crop srcpad
        GST_DEBUG_OBJECT(hailo_basecropper, "Sending allocation query to the crop srcpad");
        if (!gst_pad_peer_query (hailo_basecropper->srcpad_crop, crop_query)) {
            GST_DEBUG_OBJECT(hailo_basecropper, "Peer crop srcpad allocation query failed");
        }

        // Peer srcpad_crop has replied, perform decide allocation
        gst_hailo_basecropper_decide_allocation(hailo_basecropper, crop_query);

        gst_query_unref(crop_query);
        break;
    }
    case GST_EVENT_STREAM_START:
    {
        const gchar *stream_id;
        gst_event_parse_stream_start(event, &stream_id);
        if (!stream_id)
        {
            GST_ERROR_OBJECT(hailo_basecropper, "No stream ID");
            return FALSE;
        }
        else
        {
            GST_DEBUG_OBJECT(hailo_basecropper, "hailo cropper cropping stream %s", stream_id);
            std::string streamid_key(strdup(stream_id));
            hailo_basecropper->stream_ids_buff_offset[streamid_key] = 0;
        }
        ret = gst_pad_event_default(pad, parent, event);
        break;
    }
    default:
        ret = gst_pad_event_default(pad, parent, event);
        break;
    }

    return ret;
}

#ifdef HAILO15_TARGET
dsp_interpolation_type_t get_dsp_interpolation_type_from_cv(GstHailoBaseCropper *hailo_basecropper, cv::InterpolationFlags interpolation)
{
    switch (interpolation)
    {
    case cv::INTER_NEAREST:
        return INTERPOLATION_TYPE_NEAREST_NEIGHBOR;
    case cv::INTER_LINEAR:
        return INTERPOLATION_TYPE_BILINEAR;
    case cv::INTER_AREA:
        return INTERPOLATION_TYPE_AREA;
    case cv::INTER_CUBIC:
        return INTERPOLATION_TYPE_BICUBIC;
    default:
        GST_ERROR_OBJECT(hailo_basecropper, "Unsupported interpolation type %d", interpolation);
        std::cerr << "Unsupported interpolation type " << interpolation << std::endl;
        return INTERPOLATION_TYPE_BILINEAR;
    }
}
#endif

static GstBuffer* gst_hailo_basecropper_allocate_new_buffer(GstHailoBaseCropper *hailo_basecropper, size_t buffer_size)
{
    GstBuffer *output_buffer = NULL;
    
    #ifdef HAILO15_TARGET
    if (hailo_basecropper->use_dsp)
    {
        if (!hailo_basecropper->buffer_pool)
        {
            GST_ERROR_OBJECT(hailo_basecropper, "DSP buffer allocation requested form pool - but buffer pool is not initialized");
            return NULL;
        }
        GstFlowReturn ret = GST_FLOW_OK;
        ret = gst_buffer_pool_acquire_buffer(GST_BUFFER_POOL(hailo_basecropper->buffer_pool), &output_buffer, NULL);
        if (ret != GST_FLOW_OK)
        {
            GST_ERROR_OBJECT(hailo_basecropper, "Failed to acquire buffer from pool");
            return NULL;
        }
        
    }
    else
    {
        output_buffer = gst_buffer_new_allocate(NULL, buffer_size, NULL);
    }
    #else
    output_buffer = gst_buffer_new_allocate(NULL, buffer_size, NULL);
    #endif

    return output_buffer;
}

#ifdef HAILO15_TARGET
static gboolean dsp_crop_and_resize(GstHailoBaseCropper *hailo_basecropper, cv::Rect crop_rect, std::shared_ptr<HailoMat> resized_image, \
                                    GstBuffer *input_buffer, GstVideoInfo *input_video_info, GstBuffer *output_buffer,  GstVideoInfo *output_video_info)
{
    crop_resize_dims_t crop_resize_dims = {
        .perform_crop = 1,
        .crop_start_x = (size_t)crop_rect.x,
        .crop_end_x = (size_t)crop_rect.x + crop_rect.width,
        .crop_start_y = (size_t)crop_rect.y,
        .crop_end_y = (size_t)crop_rect.y + crop_rect.height,
        .destination_width = (size_t)resized_image->native_width(),
        .destination_height = (size_t)resized_image->native_height(),
    };

    int input_width = GST_VIDEO_INFO_WIDTH(input_video_info);
    int input_height = GST_VIDEO_INFO_HEIGHT(input_video_info);

    // Map input and output buffers to GstVideoFrame
    GstVideoFrame input_video_frame;
    if (!gst_video_frame_map(&input_video_frame, input_video_info, input_buffer, GST_MAP_READ))
    {
        GST_ERROR_OBJECT(hailo_basecropper, "Cannot map input buffer to frame");
        throw std::runtime_error("Cannot map input buffer to frame");
    }

    if (!gst_buffer_is_writable(output_buffer))
    {
        GST_ERROR_OBJECT(hailo_basecropper, "Output buffer is not writable");
        throw std::runtime_error("Output buffer is not writable");
    }

    GstVideoFrame output_video_frame;
    if (!gst_video_frame_map(&output_video_frame, output_video_info, output_buffer, GST_MAP_READWRITE))
    {
        GST_ERROR_OBJECT(hailo_basecropper, "Cannot map output buffer to frame");
        throw std::runtime_error("Cannot map output buffer to frame");
    }
    GstVideoFormat format = GST_VIDEO_FRAME_FORMAT(&input_video_frame);

    // If the crop rect is the same as the input image (whole buffer), we request a resize only
    if (crop_rect.x == 0 && crop_rect.y == 0 && crop_rect.width == input_width && crop_rect.height == input_height)
    {
        crop_resize_dims.perform_crop = 0;
        GST_DEBUG_OBJECT(hailo_basecropper, "DSP Resize (Format: %d): Input Width: %d, Height: %d. \
                                            Resize target Width: %ld Height: %ld", format, input_width, input_height,
                                            crop_resize_dims.destination_width, crop_resize_dims.destination_height);
    }
    else
    {
        GST_DEBUG_OBJECT(hailo_basecropper, "DSP Crop + Resize (Format: %d): Input Width: %d, Height: %d. \
                                            Target Crop shape X: %d Y: %d Width: %d Height: %d. and Resize target Width: %ld Height: %ld",
                                            format, input_width, input_height, crop_rect.x, crop_rect.y, crop_rect.width, crop_rect.height,
                                            crop_resize_dims.destination_width, crop_resize_dims.destination_height);
    }

    // Create dsp image properties from both input and output video frame objects
    dsp_image_properties_t input_image_properties = create_image_properties_from_video_frame(&input_video_frame);
    dsp_image_properties_t output_image_properties = create_image_properties_from_video_frame(&output_video_frame);

    // Perform the crop and resize
    dsp_status result = perform_dsp_crop_and_resize(&input_image_properties, &output_image_properties, crop_resize_dims, 
                                                get_dsp_interpolation_type_from_cv(hailo_basecropper, cv::InterpolationFlags::INTER_LINEAR));

    // Free resources
    free_image_property_planes(&input_image_properties);
    free_image_property_planes(&output_image_properties);
    gst_video_frame_unmap(&input_video_frame);
    gst_video_frame_unmap(&output_video_frame);

    if (result != DSP_SUCCESS)
    {
        GST_ERROR_OBJECT(hailo_basecropper, "Failed to perform dsp resize. return status: %d", result);
        return false;
    }

    return true;
}
#endif

static gboolean opencv_crop_and_resize(GstHailoBaseCropper *hailo_basecropper, std::shared_ptr<HailoMat> resized_image, std::shared_ptr<HailoMat> full_image, GstVideoInfo *full_image_info, HailoROIPtr crop_roi)
{
    GstHailoBaseCropperClass *hailo_basecropperclass = GST_HAILO_BASE_CROPPER_GET_CLASS(hailo_basecropper);
    cv::Mat &resized_cv_mat = resized_image->get_mat();

    GST_DEBUG_OBJECT(hailo_basecropper, "Opencv Crop + Resize: Input Width: %d, Height: %d. \
                    Target Crop shape X: %f Y: %f Width: %f Height: %f. \
                    Resize width %d height %d\n", full_image->width(), full_image->height(),
                    crop_roi->get_bbox().xmin(), crop_roi->get_bbox().ymin(), 
                    crop_roi->get_bbox().width(), crop_roi->get_bbox().height(), resized_cv_mat.cols, resized_cv_mat.rows);
    cv::Mat cropped_cv_mat = full_image->crop(crop_roi);

    GstVideoFormat image_format = GST_VIDEO_INFO_FORMAT(full_image_info);
    hailo_basecropperclass->resize(hailo_basecropper, cropped_cv_mat, resized_cv_mat, crop_roi, image_format);
    cropped_cv_mat.release();
    resized_cv_mat.release();
    return true;
}

/**
 * Create a new GstBuffer, crop and resize the frame to match the crop_roi, add the buffer to the metadata.
 *
 * @param[in] hailo_basecropper Cropping element.
 * @param[in] input_buffer      Buffer to crop & resize.
 * @param[in] crop_roi          Reference to a ROI Object to crop dimensions.
 * @return A new buffer, cropped and scaled for a second network.
 */
static GstBuffer *handle_one_crop(GstHailoBaseCropper *hailo_basecropper, GstBuffer *input_buffer, HailoROIPtr crop_roi)
{
    GstCaps *incaps, *outcaps;
    GstBuffer *output_buffer = NULL;

    incaps = gst_pad_get_current_caps(hailo_basecropper->sinkpad);
    if (!incaps)
    {
        GST_ERROR_OBJECT(hailo_basecropper, "Failed to get input CAPS from sinkpad");
        return NULL;
    }

    outcaps = gst_pad_get_current_caps(hailo_basecropper->srcpad_crop);
    if (!outcaps)
    {
        GST_ERROR_OBJECT(hailo_basecropper, "Failed to get output CAPS from srcpad (crop)");
        gst_caps_unref(incaps);
        return NULL;
    }

    GstVideoInfo *full_image_info = gst_video_info_new();
    gst_video_info_from_caps(full_image_info, incaps);

    GstVideoInfo *resized_image_info = gst_video_info_new();
    gst_video_info_from_caps(resized_image_info, outcaps);

    HailoBBox roi_bbox = crop_roi->get_bbox();
    bool crop_roi_is_whole_buffer = (roi_bbox.width() == 1.0f && roi_bbox.height() == 1.0f && roi_bbox.xmin() == 0.0f && roi_bbox.ymin() == 0.0f);
    bool input_res_equals_output_res = (full_image_info->width == resized_image_info->width && full_image_info->height == resized_image_info->height);

    // If the crop ROI is the whole buffer and the input and output resolutions are the same, we can just return a copy of the buffer
    if (crop_roi_is_whole_buffer && input_res_equals_output_res)
    {
        GST_DEBUG_OBJECT(hailo_basecropper, "Crop ROI is the whole buffer and input and output resolutions are the same, returning a copy of the buffer");
        output_buffer = gst_buffer_ref(input_buffer);
        gst_buffer_add_hailo_meta(output_buffer, crop_roi);
        gst_video_info_free(full_image_info);
        gst_video_info_free(resized_image_info);
        gst_caps_unref(incaps);
        gst_caps_unref(outcaps);
        return output_buffer;
    }

    size_t buffer_size = get_size(outcaps);
    GST_DEBUG_OBJECT(hailo_basecropper, "Allocating output buffer size: %d", (int)buffer_size);
    // Allocate new GstBuffer
    output_buffer = gst_hailo_basecropper_allocate_new_buffer(hailo_basecropper, buffer_size);

    // Get cv matrix of full image from buffer
    std::shared_ptr<HailoMat> full_image = get_mat_by_format(input_buffer, full_image_info);

    // Get cv matrix of cropped image from buffer
    std::shared_ptr<HailoMat> resized_image = get_mat_by_format(output_buffer, resized_image_info);

    // Crop and resize the frame
    #ifdef HAILO15_TARGET
        if (hailo_basecropper->use_dsp)
        {
            cv::Rect crop_rect = full_image->get_crop_rect(crop_roi);
            dsp_crop_and_resize(hailo_basecropper, crop_rect, resized_image, input_buffer, full_image_info, output_buffer, resized_image_info);
        }
        else
        {
            opencv_crop_and_resize(hailo_basecropper,resized_image, full_image, full_image_info, crop_roi);
        }
    #else
        opencv_crop_and_resize(hailo_basecropper,resized_image, full_image, full_image_info, crop_roi);
    #endif

    GST_DEBUG_OBJECT(hailo_basecropper, "Crop and resize done, freeing resources and returning buffer");

    gst_video_info_free(full_image_info);
    gst_video_info_free(resized_image_info);

    // Add the croopped ROI to the buffer
    gst_buffer_add_hailo_meta(output_buffer, crop_roi);

    gst_caps_unref(incaps);
    gst_caps_unref(outcaps);

    return output_buffer;
}

/**
 * Creates new crop buffers from given HailoROIs
 *
 * @param[in] hailo_basecropper      cropping element.
 * @param[in] buf               Buffer to crop.
 * @param[in] crop_rois        Vector of HailoROI of buf to crop from.
 * @return boolean, whether all cropping were successful.
 */
static gboolean handle_crops(GstHailoBaseCropper *hailo_basecropper, GstBuffer *buf, std::vector<HailoROIPtr> &crop_rois)
{
    for (HailoROIPtr &crop_roi : crop_rois)
    {
        if (!gst_pad_is_active(hailo_basecropper->srcpad_crop))
        {
            GST_INFO_OBJECT(hailo_basecropper, "Crop src pad is not active, dropping buffer");
            return TRUE;
        }
        GstBuffer *newbuf = handle_one_crop(hailo_basecropper, buf, crop_roi);
        if (!newbuf)
        {
            GST_WARNING_OBJECT(hailo_basecropper, "Could not crop buffer with offset %jd", buf->offset);
            return FALSE;
        }
        newbuf->offset = buf->offset;

        // Push the cropped buffer into the crop src pad.
        gst_pad_push(hailo_basecropper->srcpad_crop, newbuf);
    }
    return TRUE;
}

uint filter_streams_have_name(GstHailoBaseCropper *hailo_basecropper, const gchar *name)
{
    for (uint i = 0; i < hailo_basecropper->num_streams_to_filter; ++i)
    {
        if (strcmp(name, hailo_basecropper->filter_streams[i]) == 0)
            return 1;
    }
    return 0;
}

static GstFlowReturn gst_hailo_basecropper_chain(GstPad *pad, GstObject *parent, GstBuffer *buf)
{
    GstHailoBaseCropper *hailo_basecropper = GST_HAILO_BASE_CROPPER_CAST(parent);
    GstHailoBaseCropperClass *hailo_basecropperclass = GST_HAILO_BASE_CROPPER_GET_CLASS(hailo_basecropper);
    buf = gst_buffer_make_writable(buf);

    // Prepare crops and flags
    gchar *stream_id = gst_pad_get_stream_id(pad);
    std::string streamid_key(stream_id);
    std::vector<HailoROIPtr> crop_rois;
    bool stream_requested = true;
    bool cropping_period_reached = true;
    gchar *input_stream_name = g_strdup("");

    // Get the input stream name from the stream metadata on the buffer
    GstHailoStreamMeta *input_stream_meta = gst_buffer_get_hailo_stream_meta(buf);
    if (input_stream_meta)
        input_stream_name = input_stream_meta->pad_name;

    // Check if this stream was requested (default support all streams)
    if (hailo_basecropper->num_streams_to_filter != 0 && !filter_streams_have_name(hailo_basecropper, input_stream_name))
        stream_requested = false;

    // Check if the requested cyle period is reached (default cycle is every buffer)
    if ((hailo_basecropper->stream_ids_buff_offset[streamid_key] % hailo_basecropper->cropping_period) != 0)
        cropping_period_reached = false;

    // If both flags are true then we can crop this frame
    if (stream_requested && cropping_period_reached)
        crop_rois = hailo_basecropperclass->prepare_crops(hailo_basecropper, buf);

    GST_DEBUG_OBJECT(hailo_basecropper, "received buffer %p", buf);

    // If there is nothing to crop and dropping is enabled then drop now
    if (hailo_basecropper->drop_uncropped_buffers && crop_rois.size() == 0)
    {
        g_free(stream_id);
        gst_buffer_unref(buf);
        return GST_FLOW_OK;
    }

    if (hailo_basecropper->use_internal_offset)
    {
        buf->offset = hailo_basecropper->internal_offset;
        hailo_basecropper->internal_offset++;
    }
    hailo_basecropper->stream_ids_buff_offset[streamid_key]++;

    gst_buffer_add_hailo_cropping_meta(buf, crop_rois.size());

    // Push the main buffer into the main src pad.
    if (crop_rois.empty())
    {
        gst_pad_push(hailo_basecropper->srcpad_main, buf);
    }
    else
    {
        gst_pad_push(hailo_basecropper->srcpad_main, gst_buffer_ref(buf));
        gboolean handle_crops_ret = handle_crops(hailo_basecropper, buf, crop_rois);
        gst_buffer_unref(buf);
        if (!handle_crops_ret)
        {
            g_free(stream_id);
            return GST_FLOW_ERROR;
        }
    }
    g_free(stream_id);
    return GST_FLOW_OK;
}

/**
 * @brief Resize the an image without aspect preservation.
 *        Supports RGB/BGR, NV12, and YUY2
 *
 * @param method - cv::InterpolationFlags
 *        The interpulation to use.
 *
 * @param cropped_image - cv::Mat &
 *        The cropped image to resize
 *
 * @param resized_image - cv::Mat &
 *        The resized image container to fill
 *        (dims for resizing are assumed from here)
 *
 * @param roi - HailoROIPtr
 *        ROI to resize in, used in inheriting classes
 * @param image_format - GstVideoFormat
 *        The format of the matrices.
 */
void resize_normal(cv::InterpolationFlags method,
                   cv::Mat &cropped_image, cv::Mat &resized_image,
                   GstVideoFormat image_format)
{
    switch (image_format)
    {
    case GST_VIDEO_FORMAT_YUY2:
    {
        resize_yuy2(cropped_image, resized_image, method);
        break;
    }
    case GST_VIDEO_FORMAT_NV12:
    {
        resize_nv12(cropped_image, resized_image, method);
        break;
    }
    default:
    {
        cv::resize(cropped_image, resized_image, cv::Size(resized_image.cols, resized_image.rows), 0, 0, method);
        break;
    }
    }
}

/**
 * @brief Resize the an image using Letterbox strategy
 *        Supports RGB/BGR
 *
 * @param method - cv::InterpolationFlags
 *        The interpulation to use.
 *
 * @param cropped_image - cv::Mat &
 *        The cropped image to resize
 *
 * @param resized_image - cv::Mat &
 *        The resized image container to fill
 *        (dims for resizing are assumed from here)
 *
 * @param roi - HailoROIPtr
 *        ROI to resize in, used to update scaling bbox
 *
 * @param image_format - GstVideoFormat
 *        The format of the matrices.
 */
void resize_letterbox(cv::InterpolationFlags method,
                      cv::Mat &cropped_image, cv::Mat &resized_image,
                      HailoROIPtr roi, GstVideoFormat image_format)
{
    switch (image_format)
    {
    case GST_VIDEO_FORMAT_NV12:
    {
        static const cv::Scalar color(130, 130, 130);
        HailoBBox letterboxed_scale = resize_letterbox_nv12(cropped_image, resized_image, color, method);
        roi->set_scaling_bbox(letterboxed_scale);
        break;
    }
    case GST_VIDEO_FORMAT_RGBA:
    case GST_VIDEO_FORMAT_RGB:
    {
        static const cv::Scalar color(114, 114, 114);
        HailoBBox letterboxed_scale = resize_letterbox_rgb(cropped_image, resized_image, color, method);
        roi->set_scaling_bbox(letterboxed_scale);
        break;
    }
    default:
    {
        std::cerr << "Letterbox resizing is supported only for RGB at this moment." << std::endl;
        break;
    }
    }
}