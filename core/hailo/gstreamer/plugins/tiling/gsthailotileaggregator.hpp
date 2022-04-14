/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/*
 * GStreamer Tiling aggregator element.
 *
 */

#pragma once

#include <gst/gst.h>
#include "cropping/gsthailoaggregator.hpp"

G_BEGIN_DECLS

#define GST_TYPE_HAILO_TILE_AGGREGATOR (gst_hailotileaggregator_get_type())
#define GST_HAILO_TILE_AGGREGATOR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_TILE_AGGREGATOR, GstHailoTileAggregator))
#define GST_HAILO_TILE_AGGREGATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_TILE_AGGREGATOR, GstHailoTileAggregatorClass))
#define GST_IS_HAILO_TILE_AGGREGATOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_TILE_AGGREGATOR))
#define GST_IS_HAILO_TILE_AGGREGATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_TILE_AGGREGATOR))
#define GST_HAILO_TILE_AGGREGATOR_CAST(obj) ((GstHailoTileAggregator *)obj)

typedef struct _GstHailoTileAggregator GstHailoTileAggregator;
typedef struct _GstHailoTileAggregatorClass GstHailoTileAggregatorClass;

struct _GstHailoTileAggregator
{
    GstHailoAggregator hailo_aggregator;

    gfloat iou_threshold;
    gfloat border_threshold;
    gboolean remove_large_landscape;
};

struct _GstHailoTileAggregatorClass
{
    GstHailoAggregatorClass parent_class;
};

GType gst_hailotileaggregator_get_type(void);

G_END_DECLS
