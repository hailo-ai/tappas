/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/*
 * GStreamer Tiling cropper element.
 *
 */

#pragma once

#include <gst/gst.h>
#include "cropping/gsthailobasecropper.hpp"
#include "hailo_objects.hpp"

G_BEGIN_DECLS

#define GST_TYPE_HAILO_TILE_CROPPER (gst_hailotilecropper_get_type())
#define GST_HAILO_TILE_CROPPER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_TILE_CROPPER, GstHailoTileCropper))
#define GST_HAILO_TILE_CROPPER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_TILE_CROPPER, GstHailoTileCropperClass))
#define GST_IS_HAILO_TILE_CROPPER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_TILE_CROPPER))
#define GST_IS_HAILO_TILE_CROPPER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_TILE_CROPPER))
#define GST_HAILO_TILE_CROPPER_CAST(obj) ((GstHailoTileCropper *)obj)

typedef struct _GstHailoTileCropper GstHailoTileCropper;
typedef struct _GstHailoTileCropperClass GstHailoTileCropperClass;

struct _GstHailoTileCropper
{
    GstHailoBaseCropper hailo_cropper;
    guint tiles_along_x_axis;
    guint tiles_along_y_axis;
    gfloat overlap_x_axis;
    gfloat overlap_y_axis;
    guint multi_scale_level;
    hailo_tiling_mode_t tiling_mode;
};

struct _GstHailoTileCropperClass
{
    GstHailoBaseCropperClass parent_class;
};

GType gst_hailotilecropper_get_type(void);

G_END_DECLS
