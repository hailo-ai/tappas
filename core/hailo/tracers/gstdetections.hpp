#pragma once

#include "gstsharktracer.hpp"

G_BEGIN_DECLS

#define GST_TYPE_DETECTIONS_TRACER (gst_detections_tracer_get_type ())
G_DECLARE_FINAL_TYPE (GstDetectionsTracer, gst_detections_tracer, GST, DETECTIONS_TRACER, GstSharkTracer)

G_END_DECLS
