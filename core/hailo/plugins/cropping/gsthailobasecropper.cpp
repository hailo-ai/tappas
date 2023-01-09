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

GST_DEBUG_CATEGORY_STATIC(gst_hailo_basecropper_debug);
#define GST_CAT_DEFAULT gst_hailo_basecropper_debug

enum
{
    PROP_0,
    PROP_USE_INTERNAL_OFFSET,
    PROP_DROP_UNCROPPED_BUFFERS,
    PROP_CROPPING_PERIOD,
    PROP_FILTER_STREAMS,
};

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink",
                                                                   GST_PAD_SINK,
                                                                   GST_PAD_ALWAYS,
                                                                   gst_caps_from_string(HAILO_BASE_CROPPER_VIDEO_CAPS));

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src",
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
static GstFlowReturn gst_hailo_basecropper_chain(GstPad *pad,
                                                 GstObject *parent, GstBuffer *buf);

static void
gst_hailo_basecropper_class_init(GstHailoBaseCropperClass *klass)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;

    gobject_class = (GObjectClass *)klass;
    gstelement_class = (GstElementClass *)klass;

    gobject_class->set_property = gst_hailo_basecropper_set_property;
    gobject_class->get_property = gst_hailo_basecropper_get_property;

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
    
    gst_element_class_add_pad_template(gstelement_class,
                                       gst_static_pad_template_get(&src_factory));
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

    hailo_basecropper->srcpad_main = gst_pad_new_from_static_template(&src_factory, "src_0");
    GST_PAD_SET_PROXY_CAPS(hailo_basecropper->srcpad_main);
    gst_element_add_pad(GST_ELEMENT(hailo_basecropper), hailo_basecropper->srcpad_main);

    hailo_basecropper->srcpad_crop = gst_pad_new_from_static_template(&src_factory, "src_1");
    GST_PAD_SET_PROXY_CAPS(hailo_basecropper->srcpad_crop);
    gst_element_add_pad(GST_ELEMENT(hailo_basecropper), hailo_basecropper->srcpad_crop);

    // Set default values.
    hailo_basecropper->use_internal_offset = false;
    hailo_basecropper->internal_offset = 0;
    hailo_basecropper->cropping_period = 1;
    hailo_basecropper->num_streams_to_filter = 0;
    hailo_basecropper->drop_uncropped_buffers = false;
    hailo_basecropper->stream_ids_buff_offset.clear();
    for (uint i = 0; i < GST_HAILO_CROPPER_MAX_FILTER_STREAMS; i++)
        hailo_basecropper->filter_streams[i] = "";
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
    if (len > 0)
    {
        for (uint i = 0; i < len; i++)
        {
            const GValue *val = gst_value_array_get_value(value, i);
            hailo_basecropper->filter_streams[i] = g_strdup(g_value_get_string(val));
        }
        hailo_basecropper->num_streams_to_filter = len;
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
gst_hailo_basecropper_sink_query(GstPad *pad,
                                 GstObject *parent, GstQuery *query)
{
    gboolean ret;
    GstCaps *caps_result, *allowed_caps, *qcaps;

    switch (GST_QUERY_TYPE(query))
    {
    case GST_QUERY_CAPS:
    {
        /* we should report the supported caps here which are all */
        allowed_caps = gst_pad_get_pad_template_caps(pad);
        gst_query_parse_caps(query, &qcaps);

        if (qcaps)
        {
            // caps query - intersect template caps (allowed caps) with incomming caps query
            caps_result = gst_caps_intersect(allowed_caps, qcaps);
            gst_caps_unref(allowed_caps);
        }
        else
        {
            gst_caps_unref(allowed_caps);
            caps_result = allowed_caps;
        }

        gst_query_set_caps_result(query, caps_result);
        ret = TRUE;
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
        GstCaps *caps;

        gst_event_parse_caps(event, &caps);
        ret = gst_pad_set_caps(hailo_basecropper->srcpad_main, caps);
        if (!ret)
        {
            GST_ELEMENT_ERROR(hailo_basecropper, STREAM, FAILED,
                              ("Unable to set main src caps"), (NULL));
            return FALSE;
        }

        ret = gst_crop_scale_setcaps(hailo_basecropper);

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

/**
 * Create a new GstBuffer, crop and resize the frame to match the crop_roi, add the buffer to the metadata.
 *
 * @param[in] hailo_basecropper      cropping element.
 * @param[in] buf               Buffer to crop.
 * @param[in] crop_roi          Reference to a ROI Object to crop.
 * @return A new buffer, cropped and scaled for a second network.
 */
static GstBuffer *handle_one_crop(GstHailoBaseCropper *hailo_basecropper, GstBuffer *buf, HailoROIPtr crop_roi)
{
    GstHailoBaseCropperClass *hailo_basecropperclass = GST_HAILO_BASE_CROPPER_GET_CLASS(hailo_basecropper);

    // GstBuffer *buffer;
    GstCaps *incaps, *outcaps;
    GstBuffer *cropped_buf = NULL;

    // Allocate new GstBuffer
    incaps = gst_pad_get_current_caps(hailo_basecropper->sinkpad);
    outcaps = gst_pad_get_current_caps(hailo_basecropper->srcpad_crop);
    if (!incaps || !outcaps)
    {
        return cropped_buf;
    }
    cropped_buf = gst_buffer_new_allocate(NULL, get_size(outcaps), NULL);

    // get cv matrix of full image from buffer
    GstVideoInfo *full_image_info = gst_video_info_new();
    GstMapInfo full_image_map;
    gst_buffer_map(buf, &full_image_map, GST_MAP_READ);
    gst_video_info_from_caps(full_image_info, incaps);
    std::shared_ptr<HailoMat> full_image = get_mat_by_format(full_image_info, &full_image_map);
    GstVideoFormat image_format = GST_VIDEO_INFO_FORMAT(full_image_info) ;
    gst_video_info_free(full_image_info);

    // get cv matrix of cropped image from buffer
    GstVideoInfo *resized_image_info = gst_video_info_new();
    GstMapInfo resized_image_map;
    gst_buffer_map(cropped_buf, &resized_image_map, GST_MAP_READWRITE);
    gst_video_info_from_caps(resized_image_info, outcaps);
    std::shared_ptr<HailoMat> resized_image = get_mat_by_format(resized_image_info, &resized_image_map);
    gst_video_info_free(resized_image_info);

    // Crop and resize the the frame
    cv::Mat cropped_cv_mat = full_image->crop(crop_roi);
    cv::Mat &resized_cv_mat = resized_image->get_mat();
    hailo_basecropperclass->resize(hailo_basecropper, cropped_cv_mat, resized_cv_mat, crop_roi, image_format);

    // Add the croopped ROI to the buffer
    gst_buffer_add_hailo_meta(cropped_buf, crop_roi);

    cropped_cv_mat.release();
    resized_cv_mat.release();
    gst_caps_unref(incaps);
    gst_caps_unref(outcaps);
    gst_buffer_unmap(buf, &full_image_map);
    gst_buffer_unmap(cropped_buf, &resized_image_map);
    return cropped_buf;
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
    for (uint i = 0; i < hailo_basecropper->num_streams_to_filter; ++i) {
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
        handle_crops(hailo_basecropper, buf, crop_rois);
        gst_buffer_unref(buf);
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
 */
void resize_letterbox(cv::InterpolationFlags method, 
                      cv::Mat &cropped_image, cv::Mat &resized_image, 
                      HailoROIPtr roi)
{
    cv::Mat tmp;
    static const cv::Scalar color(114, 114, 114);
    float ratio = std::min(float(resized_image.rows) / cropped_image.rows,
                            float(resized_image.cols) / cropped_image.cols);
    int new_width = std::round(cropped_image.cols * ratio);
    int new_height = std::round(cropped_image.rows * ratio);
    float middle_point_width = (resized_image.cols - new_width) / 2;
    float middle_point_height = (resized_image.rows - new_height) / 2;
    // Calculate the number of pixels we should colorize
    int top = std::round(middle_point_height - 0.1);
    int bottom = std::round(middle_point_height + 0.1);
    int left = std::round(middle_point_width - 0.1);
    int right = std::round(middle_point_width + 0.1);
    // Apply the letterboxing to the roi scale factor
    HailoBBox letterboxed_scale = HailoBBox(-(left / float(new_width)),                      // x-offset
                                            -(top / float(new_height)),                      // y-offset
                                            1.0 / (new_width / float(resized_image.cols)),   // width factor
                                            1.0 / (new_height / float(resized_image.rows))); // height factor
    roi->set_scaling_bbox(letterboxed_scale);

    // Reverse RGB order to BGR is required in the resize & border
    cv::resize(cropped_image, tmp, cv::Size(new_width, new_height), 0, 0, method);
    cv::copyMakeBorder(tmp, resized_image, top, bottom, left, right, cv::BORDER_CONSTANT, color);
    tmp.release();
}
