/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include <gst/gst.h>
#include <opencv2/opencv.hpp>

#include "gst_hailo_meta.hpp"
#include "gsthailotilecropper.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_hailotilecropper_debug);
#define GST_CAT_DEFAULT gst_hailotilecropper_debug

#define _do_init \
    GST_DEBUG_CATEGORY_INIT(gst_hailotilecropper_debug, "hailotilecropper", 0, "hailotilecropper element");

#define DEFAULT_NUM_OF_TILES_X_AXIS 2
#define DEFAULT_NUM_OF_TILES_Y_AXIS 2
#define DEFAULT_OVERLAP_X_AXIS 0
#define DEFAULT_OVERLAP_Y_AXIS 0
#define DEFAULT_MULTI_SCALE_LEVEL 2
static const uint scales_template[][2]{{1, 1}, {2, 2}, {3, 3}};

enum
{
    PROP_0,
    PROP_TILES_X_AXIS,
    PROP_TILES_Y_AXIS,
    PROP_OVERLAP_X_AXIS,
    PROP_OVERLAP_Y_AXIS,
    PROP_TILING_MODE,
    PROP_MULTI_SCALE_LEVEL,
};

#define gst_hailotilecropper_parent_class parent_class

G_DEFINE_TYPE_WITH_CODE(GstHailoTileCropper, gst_hailotilecropper, GST_TYPE_HAILO_BASE_CROPPER, _do_init);

#define GST_TYPE_HAILOTILECROPPER_TILING_MODE (gst_hailotilecropper_tiling_mode_get_type())
static GType
gst_hailotilecropper_tiling_mode_get_type(void)
{
    static GType cropper_tiling_mode = 0;
    static const GEnumValue hailotilecropper_tiling_modes[] = {
        {SINGLE_SCALE, "Single Scale", "single-scale"},
        {MULTI_SCALE, "Multi Scale", "multi-scale"},
        {0, NULL, NULL},
    };
    if (!cropper_tiling_mode)
    {
        cropper_tiling_mode =
            g_enum_register_static("GstHailoTileCropperTilingMode", hailotilecropper_tiling_modes);
    }
    return cropper_tiling_mode;
}

static void gst_hailotilecropper_set_property(GObject *object,
                                              guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_hailotilecropper_get_property(GObject *object,
                                              guint prop_id, GValue *value, GParamSpec *pspec);

static void gst_hailotilecropper_dispose(GObject *object);
static void gst_hailotilecropper_finalize(GObject *object);

static std::vector<HailoROIPtr> gst_hailotilecropper_prepare_crops(GstHailoBaseCropper *hailocropper,
                                                                   GstBuffer *buf);
void tiling_resize(GstHailoBaseCropper *basecropper, cv::Mat &cropped_image, cv::Mat &resized_image, HailoROIPtr roi, GstVideoFormat image_format);


static void
gst_hailotilecropper_class_init(GstHailoTileCropperClass *klass)
{
    GstElementClass *gstelement_class;
    GstHailoBaseCropperClass *hailobasecropper_class;

    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    gstelement_class = (GstElementClass *)klass;
    hailobasecropper_class = (GstHailoBaseCropperClass *)klass;

    gobject_class->set_property = gst_hailotilecropper_set_property;
    gobject_class->get_property = gst_hailotilecropper_get_property;

    gobject_class->dispose = gst_hailotilecropper_dispose;
    gobject_class->finalize = gst_hailotilecropper_finalize;

    hailobasecropper_class->prepare_crops = gst_hailotilecropper_prepare_crops;
    hailobasecropper_class->resize = tiling_resize;

    gst_element_class_set_details_simple(gstelement_class,
                                         "hailotilecropper - Tiling",
                                         "Hailo/Tools",
                                         "Crops and scales an image into tiles", "hailo.ai <contact@hailo.ai>");

    g_object_class_install_property(gobject_class, PROP_TILES_X_AXIS,
                                    g_param_spec_uint("tiles-along-x-axis", "Tiles along x axis", "Number of tiles along x axis (columns)", 1, 20, 2,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));

    g_object_class_install_property(gobject_class, PROP_TILES_Y_AXIS,
                                    g_param_spec_uint("tiles-along-y-axis", "Tiles along y axis", "Number of tiles along x axis (rows)", 1, 20, 2,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));

    g_object_class_install_property(gobject_class, PROP_OVERLAP_X_AXIS,
                                    g_param_spec_float("overlap-x-axis", "Overlap x axis", "Overlap in percentage between tiles along x axis (columns)", 0, 1, 0,
                                                       (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));

    g_object_class_install_property(gobject_class, PROP_OVERLAP_Y_AXIS,
                                    g_param_spec_float("overlap-y-axis", "Overlap y axis", "Overlap in percentage between tiles along y axis (rows)", 0, 1, 0,
                                                       (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));

    g_object_class_install_property(gobject_class, PROP_TILING_MODE,
                                    g_param_spec_enum("tiling-mode", "Tiling mode", "Tiling mode",
                                                      GST_TYPE_HAILOTILECROPPER_TILING_MODE, (gint)SINGLE_SCALE,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    g_object_class_install_property(gobject_class, PROP_MULTI_SCALE_LEVEL,
                                    g_param_spec_uint("scale-level", "Scale level", "Scales (layers of tiles) in addition to the main layer 1: [(1 X 1)] 2: [(1 X 1), (2 X 2)] 3: [(1 X 1), (2 X 2), (3 X 3)]]", 1, 3, 2,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
}

static void
gst_hailotilecropper_init(GstHailoTileCropper *hailotilecropper)
{
    hailotilecropper->tiles_along_x_axis = DEFAULT_NUM_OF_TILES_X_AXIS;
    hailotilecropper->tiles_along_y_axis = DEFAULT_NUM_OF_TILES_Y_AXIS;
    hailotilecropper->overlap_x_axis = DEFAULT_OVERLAP_X_AXIS;
    hailotilecropper->overlap_y_axis = DEFAULT_OVERLAP_Y_AXIS;
    hailotilecropper->tiling_mode = SINGLE_SCALE;
    hailotilecropper->multi_scale_level = DEFAULT_MULTI_SCALE_LEVEL;
}

void gst_hailotilecropper_dispose(GObject *object)
{
    GstHailoTileCropper *hailotilecropper = GST_HAILO_TILE_CROPPER(object);
    GST_DEBUG_OBJECT(hailotilecropper, "dispose");
    G_OBJECT_CLASS(gst_hailotilecropper_parent_class)->dispose(object);
}

void gst_hailotilecropper_finalize(GObject *object)
{
    GstHailoTileCropper *hailotilecropper = GST_HAILO_TILE_CROPPER(object);
    GST_DEBUG_OBJECT(hailotilecropper, "finalize");
    G_OBJECT_CLASS(gst_hailotilecropper_parent_class)->finalize(object);
}

static void
gst_hailotilecropper_set_property(GObject *object, guint prop_id,
                                  const GValue *value, GParamSpec *pspec)
{
    GstHailoTileCropper *hailotilecropper = GST_HAILO_TILE_CROPPER(object);
    switch (prop_id)
    {
    case PROP_TILES_X_AXIS:
        hailotilecropper->tiles_along_x_axis = g_value_get_uint(value);
        break;
    case PROP_TILES_Y_AXIS:
        hailotilecropper->tiles_along_y_axis = g_value_get_uint(value);
        break;
    case PROP_OVERLAP_X_AXIS:
        hailotilecropper->overlap_x_axis = g_value_get_float(value);
        break;
    case PROP_OVERLAP_Y_AXIS:
        hailotilecropper->overlap_y_axis = g_value_get_float(value);
        break;
    case PROP_MULTI_SCALE_LEVEL:
        hailotilecropper->multi_scale_level = g_value_get_uint(value);
        break;
    case PROP_TILING_MODE:
        GST_OBJECT_LOCK(hailotilecropper);
        hailotilecropper->tiling_mode = (hailo_tiling_mode_t)g_value_get_enum(value);
        GST_OBJECT_UNLOCK(hailotilecropper);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_hailotilecropper_get_property(GObject *object, guint prop_id,
                                  GValue *value, GParamSpec *pspec)
{
    GstHailoTileCropper *hailotilecropper = GST_HAILO_TILE_CROPPER(object);

    switch (prop_id)
    {
    case PROP_TILES_X_AXIS:
        g_value_set_uint(value, hailotilecropper->tiles_along_x_axis);
        break;
    case PROP_TILES_Y_AXIS:
        g_value_set_uint(value, hailotilecropper->tiles_along_y_axis);
        break;
    case PROP_OVERLAP_X_AXIS:
        g_value_set_float(value, hailotilecropper->overlap_x_axis);
        break;
    case PROP_OVERLAP_Y_AXIS:
        g_value_set_float(value, hailotilecropper->overlap_y_axis);
        break;
    case PROP_MULTI_SCALE_LEVEL:
        g_value_set_uint(value, hailotilecropper->multi_scale_level);
        break;
    case PROP_TILING_MODE:
        GST_OBJECT_LOCK(hailotilecropper);
        g_value_set_enum(value, (gint)hailotilecropper->tiling_mode);
        GST_OBJECT_UNLOCK(hailotilecropper);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/**
 * Create one HailoTileROI includes the bbox that represents a tile.
 * add the requested overlap, scale it to the width, height of the frame
 *
 * @param[in] index   uint,     index of the tile.
 * @param[in] col_overlap   float,     x axis overlap between tiles (columns).
 * @param[in] row_overlap   float,     y axis overlap between tiles (rows).
 * @param[in] xmin   float,   min X of tile tile.
 * @param[in] ymin   float,   min Y of tile tile.
 * @param[in] xmax   float,   max X of tile tile.
 * @param[in] ymax   float,   max Y of tile tile.
 * @return HailoTileROI, prepared Tile ROI object.
 */
static HailoTileROI create_tile_roi(const uint &index, const float &col_overlap, const float &row_overlap, const float &xmin, const float &ymin, const float &xmax, const float &ymax, const uint &layer, hailo_tiling_mode_t tiling_mode)
{
    float x = CLAMP(xmin - col_overlap, 0, 1);
    float y = CLAMP(ymin - row_overlap, 0, 1);
    float width = CLAMP(xmax + col_overlap, 0, 1) - x;
    float height = CLAMP(ymax + row_overlap, 0, 1) - y;

    return HailoTileROI(HailoBBox(x, y, width, height), index, col_overlap, row_overlap, layer, tiling_mode);
}

static void prepare_tiles(HailoROIPtr hailo_roi, std::vector<HailoROIPtr> &crop_rois, float tiles_along_x_axis, float tiles_along_y_axis, float overlap_x_axis, float overlap_y_axis, uint layer, hailo_tiling_mode_t tiling_mode)
{
    // Calculate the scale for a tile for col and row
    double row_step = 1 / double(tiles_along_y_axis);
    double row_offset = 0;
    double col_step = 1 / double(tiles_along_x_axis);

    // Re scale the overlap value to match a tile size
    float col_overlap = overlap_x_axis * col_step;
    float row_overlap = overlap_y_axis * row_step;

    double col_offset = 0;
    uint index = 0;
    while (float(row_offset + row_step) <= 1)
    {
        while (float(col_offset + col_step) <= 1)
        {
            // Create new tile ROI
            HailoTileROIPtr tile_roi = std::make_shared<HailoTileROI>(create_tile_roi(index, col_overlap, row_overlap,
                                                                                      col_offset, row_offset, (col_offset + col_step), (row_offset + row_step),
                                                                                      layer, tiling_mode));
            // Add the tile to the result vector and into the main hailo_roi.
            crop_rois.emplace_back(tile_roi);
            hailo_roi->add_object(tile_roi);

            col_offset += col_step;
            index++;
        }
        row_offset += row_step;
        col_offset = 0;
    }
}

/**
 * Creates vector of HailoROI as a preparation for the crop scale phase,
 * overrides hailocropper base functionality.
 * prepares vector of tiles in row/column structure (determined by elemnet properties) (HailoTileROI for each tile).
 * adds each one to the main roi. tiles can overlap each other.
 *
 * @param[in] hailocropper    cropping element.
 * @param[in] hailo_roi       main HailoROI taken from the buffer.
 * @return std::vector<HailoROIPtr>, prepared vector of tiles
 */
static std::vector<HailoROIPtr> gst_hailotilecropper_prepare_crops(GstHailoBaseCropper *hailocropper, GstBuffer *buf)
{
    GstHailoTileCropper *hailotilecropper = GST_HAILO_TILE_CROPPER(hailocropper);

    // Get main HailoROI, in case this is the first hailo element in the pipeline, create one.
    HailoROIPtr hailo_roi = get_hailo_main_roi(buf, true);

    // Calculate the total number of tiles
    uint total_num_of_tiles = hailotilecropper->tiles_along_x_axis * hailotilecropper->tiles_along_x_axis;
    uint num_of_scales = hailotilecropper->multi_scale_level;

    if (hailotilecropper->tiling_mode == MULTI_SCALE)
    {
        // In multi-scale mode - calculate the proper number of tiles referring to scales
        for (uint i = 0; i < num_of_scales; i++)
            total_num_of_tiles += (scales_template[i][0] * scales_template[i][1]);
    }

    std::vector<HailoROIPtr> crop_rois;
    crop_rois.reserve(total_num_of_tiles);

    // Prepare tiles for the main scale
    prepare_tiles(hailo_roi, crop_rois, hailotilecropper->tiles_along_x_axis, hailotilecropper->tiles_along_y_axis,
                  hailotilecropper->overlap_x_axis, hailotilecropper->overlap_y_axis, 0, hailotilecropper->tiling_mode);

    // Prepare tiles for every scale requsted as multi scale
    if (hailotilecropper->tiling_mode == MULTI_SCALE)
        for (uint i = 0; i < num_of_scales; i++)
            prepare_tiles(hailo_roi, crop_rois, scales_template[i][0], scales_template[i][1], hailotilecropper->overlap_x_axis, hailotilecropper->overlap_y_axis, (i + 1), (hailo_tiling_mode_t)hailotilecropper->tiling_mode);

    return crop_rois;
}

void tiling_resize(GstHailoBaseCropper *basecropper, cv::Mat &cropped_image, cv::Mat &resized_image, HailoROIPtr roi, GstVideoFormat image_format)
{
    resize_normal(cv::INTER_LINEAR, cropped_image, resized_image, image_format);
}
