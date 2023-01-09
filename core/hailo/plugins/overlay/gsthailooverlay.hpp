/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once

#include <gst/base/gstbasetransform.h>
#include <vector>
#include "hailo_objects.hpp"

G_BEGIN_DECLS

#define GST_TYPE_HAILO_OVERLAY (gst_hailooverlay_get_type())
#define GST_HAILO_OVERLAY(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_OVERLAY, GstHailoOverlay))
#define GST_HAILO_OVERLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_OVERLAY, GstHailoOverlayClass))
#define GST_IS_HAILO_OVERLAY(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_OVERLAY))
#define GST_IS_HAILO_OVERLAY_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_OVERLAY))

typedef struct _GstHailoOverlay GstHailoOverlay;
typedef struct _GstHailoOverlayClass GstHailoOverlayClass;

struct _GstHailoOverlay
{
    GstBaseTransform base_hailooverlay;
    gint line_thickness;
    gint font_thickness;
    gfloat landmark_point_radius;
    gboolean face_blur;
    gboolean show_confidence;
    gboolean local_gallery;
    guint mask_overlay_n_threads;
};

struct _GstHailoOverlayClass
{
    GstBaseTransformClass base_hailooverlay_class;
};

GType gst_hailooverlay_get_type(void);

G_END_DECLS