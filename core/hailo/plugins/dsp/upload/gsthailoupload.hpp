#pragma once

#include "dsp/gsthailodspbasetransform.hpp"
#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_TYPE_HAILO_UPLOAD \
  (gst_hailo_upload_get_type())
#define GST_HAILO_UPLOAD(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_HAILO_UPLOAD,GstHailoUpload))
#define GST_HAILO_UPLOAD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_HAILO_UPLOAD,GstHailoUploadClass))
#define GST_IS_HAILO_UPLOAD(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_HAILO_UPLOAD))
#define GST_IS_HAILO_UPLOAD_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_HAILO_UPLOAD))
#define GST_HAILO_UPLOAD_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj),GST_TYPE_HAILO_UPLOAD,GstHailoUploadClass))

typedef struct _GstHailoUpload GstHailoUpload;
typedef struct _GstHailoUploadClass GstHailoUploadClass;

struct _GstHailoUpload {
  GstHailoDspBaseTransform parent;
  int dma_memcpy_fd;
  gboolean use_gpdma;
};

struct _GstHailoUploadClass {
  GstHailoDspBaseTransformClass parent_class;
  // add any additional class variables here
};

GType gst_hailo_upload_get_type (void);

G_END_DECLS