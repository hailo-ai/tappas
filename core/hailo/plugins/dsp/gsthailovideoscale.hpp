#pragma once

#include "dsp/gsthailodspbasetransform.hpp"
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_HAILO_VIDEOSCALE \
  (gst_hailo_videoscale_get_type())
#define GST_HAILO_VIDEOSCALE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_HAILO_VIDEOSCALE,GstHailoVideoScale))
#define GST_HAILO_VIDEOSCALE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_HAILO_VIDEOSCALE,GstHailoVideoScaleClass))
#define GST_IS_HAILO_VIDEOSCALE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_HAILO_VIDEOSCALE))
#define GST_IS_HAILO_VIDEOSCALE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_HAILO_VIDEOSCALE))
#define GST_HAILO_VIDEOSCALE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj),GST_TYPE_HAILO_VIDEOSCALE,GstHailoVideoScaleClass))

#define HAILO_VIDEOSCALE_SUPPORTED_FORMATS "{ RGB, NV12 }"
#define HAILO_VIDEOSCALE_VIDEO_CAPS \
    GST_VIDEO_CAPS_MAKE(HAILO_VIDEOSCALE_SUPPORTED_FORMATS)

typedef struct _GstHailoVideoScale GstHailoVideoScale;
typedef struct _GstHailoVideoScaleClass GstHailoVideoScaleClass;

struct _GstHailoVideoScale {
  GstHailoDspBaseTransform parent;
};

struct _GstHailoVideoScaleClass {
  GstHailoDspBaseTransformClass parent_class;
};

GType gst_hailo_videoscale_get_type (void);

G_END_DECLS