/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>
#include "filter/gsthailofilter.hpp"
#include "muxer/gsthailomuxer.hpp"
#include "cropping/gsthailoaggregator.hpp"
#include "cropping/gsthailocropper.hpp"
#include "overlay/gsthailooverlay.hpp"
#include "gst_hailo_meta.hpp"
#include "cropping/gst_hailo_cropping_meta.hpp"
#include "tiling/gsthailotilecropper.hpp"
#include "tiling/gsthailotileaggregator.hpp"
#include "tracking/gsthailotracker.hpp"
#include "gallery/gsthailogallery.hpp"


static gboolean
plugin_init(GstPlugin *plugin)
{
    gst_element_register(plugin, "hailooverlay", GST_RANK_PRIMARY, GST_TYPE_HAILO_OVERLAY);
    gst_element_register(plugin, "hailofilter", GST_RANK_PRIMARY, GST_TYPE_HAILO_FILTER);
    gst_element_register(plugin, "hailomuxer", GST_RANK_PRIMARY, GST_TYPE_HAILO_MUXER);
    gst_element_register(plugin, "hailocropper", GST_RANK_PRIMARY, GST_TYPE_HAILO_CROPPER);
    gst_element_register(plugin, "hailotilecropper", GST_RANK_PRIMARY, GST_TYPE_HAILO_TILE_CROPPER);
    gst_element_register(plugin, "hailoaggregator", GST_RANK_PRIMARY, GST_TYPE_HAILO_AGGREGATOR);
    gst_element_register(plugin, "hailotileaggregator", GST_RANK_PRIMARY, GST_TYPE_HAILO_TILE_AGGREGATOR);
    gst_element_register(plugin, "hailotracker", GST_RANK_PRIMARY, GST_TYPE_HAILO_TRACKER);
    gst_element_register(plugin, "hailogallery", GST_RANK_PRIMARY, GST_TYPE_HAILO_GALLERY);
    gst_hailo_meta_get_info();
    gst_hailo_meta_api_get_type();
    gst_hailo_cropping_meta_get_info();
    gst_hailo_cropping_meta_api_get_type();

    return TRUE;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, hailotools, "hailo tools plugin", plugin_init,
                  VERSION, "unknown", PACKAGE, "https://hailo.ai/")
