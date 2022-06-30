/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include <gst/gst.h>
#include <opencv2/opencv.hpp>
#include "hailo_objects.hpp"
#include "hailo_common.hpp"
#include "gst_hailo_meta.hpp"
#include "gsthailotileaggregator.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_hailotileaggregator_debug);
#define GST_CAT_DEFAULT gst_hailotileaggregator_debug

#define _do_init \
    GST_DEBUG_CATEGORY_INIT(gst_hailotileaggregator_debug, "hailotileaggregator", 0, "hailotileaggregator element");

enum
{
    PROP_0,
    PROP_IOU_THRESHOLD,
    PROP_BORDER_THRESHOLD,
    PROP_REMOVE_LARGE_LANDSCAPE,
};

#define DEFAULT_IOU_THRESHOLD 0.3
#define DEFAULT_BORDER_THRESHOLD 0.1
#define DEFAULT_REMOVE_LARGE_LANDSCAPE true

#define LARGE_LANDSCAPE_MASK_WIDTH_HEIGHT_RATIO 1.3
#define LARGE_LANDSCAPE_MASK_SIZE 0.05

#define gst_hailotileaggregator_parent_class parent_class

G_DEFINE_TYPE_WITH_CODE(GstHailoTileAggregator, gst_hailotileaggregator, GST_TYPE_HAILO_AGGREGATOR, _do_init);

static float iou_calc(const HailoBBox &box_1, const HailoBBox &box_2);
static void nms(HailoROIPtr hailo_roi, const float iou_thr);
static void gst_hailotileaggregator_set_property(GObject *object,
                                                 guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_hailotileaggregator_get_property(GObject *object,
                                                 guint prop_id, GValue *value, GParamSpec *pspec);

static void gst_hailotileaggregator_dispose(GObject *object);
static void gst_hailotileaggregator_finalize(GObject *object);

static void gst_hailotileaggregator_post_aggregation(GstHailoAggregator *hailoaggregator, HailoROIPtr hailo_roi);
static void gst_hailotileaggregator_handle_sub_frame_roi(GstHailoAggregator *hailoaggregator, HailoROIPtr sub_buffer_roi);

static void
gst_hailotileaggregator_class_init(GstHailoTileAggregatorClass *klass)
{
    GstElementClass *gstelement_class;
    GstHailoAggregatorClass *hailoaggregator_class;

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gstelement_class = (GstElementClass *)klass;
    hailoaggregator_class = (GstHailoAggregatorClass *)klass;

    gobject_class->set_property = gst_hailotileaggregator_set_property;
    gobject_class->get_property = gst_hailotileaggregator_get_property;

    gobject_class->dispose = gst_hailotileaggregator_dispose;
    gobject_class->finalize = gst_hailotileaggregator_finalize;

    hailoaggregator_class->handle_main_roi_post_aggregation = gst_hailotileaggregator_post_aggregation;
    hailoaggregator_class->handle_sub_frame_roi = gst_hailotileaggregator_handle_sub_frame_roi;

    gst_element_class_set_static_metadata(gstelement_class,
                                          "hailotileaggregator",
                                          "Hailo/Tools",
                                          "Aggregates related tiles to the original Image",
                                          "hailo.ai <contact@hailo.ai>");

    g_object_class_install_property(gobject_class, PROP_IOU_THRESHOLD,
                                    g_param_spec_float("iou-threshold", "NMS IOU Threshold", "threshold", 0, 1, DEFAULT_IOU_THRESHOLD,
                                                       (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));

    g_object_class_install_property(gobject_class, PROP_BORDER_THRESHOLD,
                                    g_param_spec_float("border-threshold", "Multi-scale remove exceeded objects functionality border threshold", "border threshold", 0, 1, DEFAULT_BORDER_THRESHOLD,
                                                       (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));

    g_object_class_install_property(gobject_class, PROP_REMOVE_LARGE_LANDSCAPE,
                                    g_param_spec_boolean("remove-large-landscape", "Remove large landscape", "remove large landscape objects when running in multi-scale mode", true,
                                                         (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
}

static void
gst_hailotileaggregator_init(GstHailoTileAggregator *hailotileaggregator)
{
    hailotileaggregator->iou_threshold = DEFAULT_IOU_THRESHOLD;
    hailotileaggregator->border_threshold = DEFAULT_BORDER_THRESHOLD;
    hailotileaggregator->remove_large_landscape = DEFAULT_REMOVE_LARGE_LANDSCAPE;
}

void gst_hailotileaggregator_dispose(GObject *object)
{
    GstHailoTileAggregator *hailotileaggregator = GST_HAILO_TILE_AGGREGATOR(object);
    GST_DEBUG_OBJECT(hailotileaggregator, "dispose");
    G_OBJECT_CLASS(gst_hailotileaggregator_parent_class)->dispose(object);
}

void gst_hailotileaggregator_finalize(GObject *object)
{
    GstHailoTileAggregator *hailotileaggregator = GST_HAILO_TILE_AGGREGATOR(object);
    GST_DEBUG_OBJECT(hailotileaggregator, "finalize");
    G_OBJECT_CLASS(gst_hailotileaggregator_parent_class)->finalize(object);
}

static void gst_hailotileaggregator_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GstHailoTileAggregator *hailotileaggregator = GST_HAILO_TILE_AGGREGATOR(object);
    switch (prop_id)
    {
    case PROP_IOU_THRESHOLD:
        hailotileaggregator->iou_threshold = g_value_get_float(value);
        break;
    case PROP_BORDER_THRESHOLD:
        hailotileaggregator->border_threshold = g_value_get_float(value);
        break;
    case PROP_REMOVE_LARGE_LANDSCAPE:
        hailotileaggregator->remove_large_landscape = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_hailotileaggregator_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    GstHailoTileAggregator *hailotileaggregator = GST_HAILO_TILE_AGGREGATOR(object);
    switch (prop_id)
    {
    case PROP_IOU_THRESHOLD:
        g_value_set_float(value, hailotileaggregator->iou_threshold);
        break;
    case PROP_BORDER_THRESHOLD:
        g_value_set_float(value, hailotileaggregator->border_threshold);
        break;
    case PROP_REMOVE_LARGE_LANDSCAPE:
        g_value_set_boolean(value, hailotileaggregator->remove_large_landscape);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/**
 * Remove large detection objects in multi-scale.
 * Usually related to crowded people when defined as one object, or other network related annomalies.
 * A large object is defined by it's size and it's width to height ratio.
 *
 * @param[in] hailo_roi       main HailoROI taken from the buffer.
 * @param[in] frame_width    integer. width of the full frame.
 * @param[in] frame_height    integer. height of the full frame.
 * @return void.
 */
static void remove_large_landscape(HailoROIPtr hailo_roi, int &frame_width, int &frame_height)
{
    auto detections = hailo_common::get_hailo_detections(hailo_roi);
    for (const HailoDetectionPtr &detection : detections)
    {
        HailoBBox bbox = detection->get_bbox();
        float width = bbox.width() * frame_width;
        float height = bbox.height() * frame_height;

        bool is_landscape_mask = (width >= (height * LARGE_LANDSCAPE_MASK_WIDTH_HEIGHT_RATIO));
        bool is_landscape_size_mask = (((width * height) / (frame_height * frame_width)) > LARGE_LANDSCAPE_MASK_SIZE);
        if (is_landscape_mask && is_landscape_size_mask)
            hailo_roi->remove_object(detection);
    }
}

/**
 * Remove detections close to the boundary of the tile.
 * Not including tile borders that located on one of the borders of the full frame.
 *
 * @param[in] hailo_tile_roi  HailoTileROIPtr taken from the buffer.
 * @param[in] border_threshold    float.  threshold - 0 - 1 value of 'close to border' ratio.
 * @return void.
 */
static void remove_exceeded_bboxes(HailoTileROIPtr hailo_tile_roi, float border_threshold)
{
    auto detections = hailo_common::get_hailo_detections(hailo_tile_roi);
    HailoBBox tile_bbox = hailo_tile_roi->get_bbox();

    for (const HailoDetectionPtr &detection : detections)
    {
        HailoBBox bbox = detection->get_bbox();
        bool exceed_xmin = (tile_bbox.xmin() != 0 && bbox.xmin() < border_threshold);
        bool exceed_xmax = (tile_bbox.xmax() != 1 && (1 - bbox.xmax()) < border_threshold);
        bool exceed_ymin = (tile_bbox.ymin() != 0 && bbox.ymin() < border_threshold);
        bool exceed_ymax = (tile_bbox.ymax() != 1 && (1 - bbox.ymax()) < border_threshold);

        if (exceed_xmin || exceed_xmax || exceed_ymin || exceed_ymax)
            hailo_tile_roi->remove_object(detection);
    }
}

static void
gst_hailotileaggregator_post_aggregation(GstHailoAggregator *hailoaggregator, HailoROIPtr hailo_roi)
{
    GstHailoTileAggregator *hailotileaggregator = GST_HAILO_TILE_AGGREGATOR(hailoaggregator);

    // Get the frame width and height from the main sinkpad
    GstCaps *caps = gst_pad_get_current_caps(hailoaggregator->sinkpad_main);
    gint frame_width, frame_height;
    auto caps_st = gst_caps_get_structure(caps, 0);
    gst_structure_get_int(caps_st, "width", &frame_width);
    gst_structure_get_int(caps_st, "height", &frame_height);

    auto tiles = hailo_common::get_hailo_tiles(hailo_roi);
    if (tiles[0]->get_mode() == MULTI_SCALE && hailotileaggregator->remove_large_landscape)
        remove_large_landscape(hailo_roi, frame_width, frame_height);

    // Perform NMS on the main frame's detections after aggragation is done
    nms(hailo_roi, hailotileaggregator->iou_threshold);
}

static void
gst_hailotileaggregator_handle_sub_frame_roi(GstHailoAggregator *hailoaggregator, HailoROIPtr sub_buffer_roi)
{
    HailoTileROIPtr hailo_tile_roi = std::dynamic_pointer_cast<HailoTileROI>(sub_buffer_roi);
    if (hailo_tile_roi->get_mode() == MULTI_SCALE)
    {
        // Remove tile's exceeded objects (close to boundary) using given border_threshold
        GstHailoTileAggregator *hailotileaggregator = GST_HAILO_TILE_AGGREGATOR(hailoaggregator);
        remove_exceeded_bboxes(hailo_tile_roi, hailotileaggregator->border_threshold);
    }

    // Calling the base handle_sub_frame_roi of the parent (hailoaggregator)
    GST_HAILO_AGGREGATOR_CLASS(parent_class)->handle_sub_frame_roi(hailoaggregator, sub_buffer_roi);
}

float iou_calc(const HailoBBox &box_1, const HailoBBox &box_2)
{
    // Calculate IOU between two detection boxes
    const float width_of_overlap_area = std::min(box_1.xmax(), box_2.xmax()) - std::max(box_1.xmin(), box_2.xmin());
    const float height_of_overlap_area = std::min(box_1.ymax(), box_2.ymax()) - std::max(box_1.ymin(), box_2.ymin());
    const float positive_width_of_overlap_area = std::max(width_of_overlap_area, 0.0f);
    const float positive_height_of_overlap_area = std::max(height_of_overlap_area, 0.0f);
    const float area_of_overlap = positive_width_of_overlap_area * positive_height_of_overlap_area;
    const float box_1_area = (box_1.ymax() - box_1.ymin()) * (box_1.xmax() - box_1.xmin());
    const float box_2_area = (box_2.ymax() - box_2.ymin()) * (box_2.xmax() - box_2.xmin());
    // The IOU is a ratio of how much the boxes overlap vs their size outside the overlap.
    // Boxes that are similar will have a higher overlap threshold.
    return area_of_overlap / (box_1_area + box_2_area - area_of_overlap);
}

/**
 * @brief Perform IOU based NMS on detection objects of HailoRoi
 *
 * @param hailo_roi  -  HailoROIPtr
 *        The HailoROI contains detections to perform NMS on.
 *
 * @param iou_thr  -  float
 *        Threshold for IOU filtration
 */
void nms(HailoROIPtr hailo_roi, const float iou_thr)
{
    // The network may propose multiple detections of similar size/score,
    // which are actually the same detection. We want to filter out the lesser
    // detections with a simple nms.

    std::vector<HailoDetectionPtr> objects = hailo_common::get_hailo_detections(hailo_roi);
    std::sort(objects.begin(), objects.end(),
              [](HailoDetectionPtr a, HailoDetectionPtr b)
              { return a->get_confidence() > b->get_confidence(); });

    for (uint index = 0; index < objects.size(); index++)
    {
        for (uint jindex = index + 1; jindex < objects.size(); jindex++)
        {
            if (objects[index]->get_class_id() == objects[jindex]->get_class_id())
            {
                // For each detection, calculate the IOU against each following detection.
                float iou = iou_calc(objects[index]->get_bbox(), objects[jindex]->get_bbox());
                // If the IOU is above threshold, then we have two similar detections,
                // and want to delete the one.
                if (iou >= iou_thr)
                {
                    // The detections are arranged in highest score order,
                    // so we want to erase the latter detection.
                    hailo_roi->remove_object(objects[jindex]);
                    objects.erase(objects.begin() + jindex);
                    jindex--; // Step back jindex since we just erased the current detection.
                }
            }
        }
    }
}