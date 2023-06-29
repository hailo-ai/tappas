#pragma once

#include "dsp/gsthailodspbasetransform.hpp"
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_HAILO_UPLOAD2 \
  (gst_hailo_upload2_get_type())
#define GST_HAILO_UPLOAD2(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_HAILO_UPLOAD2,GstHailoUpload2))
#define GST_HAILO_UPLOAD2_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_HAILO_UPLOAD2,GstHailoUpload2Class))
#define GST_IS_HAILO_UPLOAD2(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_HAILO_UPLOAD2))
#define GST_IS_HAILO_UPLOAD2_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_HAILO_UPLOAD2))
#define GST_HAILO_UPLOAD2_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj),GST_TYPE_HAILO_UPLOAD2,GstHailoUpload2Class))

typedef struct _GstHailoUpload2 GstHailoUpload2;
typedef struct _GstHailoUpload2Class GstHailoUpload2Class;

struct _GstHailoUpload2 {
  GstHailoDspBaseTransform parent;
  // add any additional instance variables here
};

struct _GstHailoUpload2Class {
  GstHailoDspBaseTransformClass parent_class;
  // add any additional class variables here
};

GType gst_hailo_upload2_get_type (void);

G_END_DECLS