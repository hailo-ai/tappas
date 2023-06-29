#pragma once

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

#define GST_TYPE_HAILO_DSP_BASE_TRANSFORM \
  (gst_hailo_dsp_base_transform_get_type())
#define GST_HAILO_DSP_BASE_TRANSFORM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_DSP_BASE_TRANSFORM, GstHailoDspBaseTransform))
#define GST_HAILO_DSP_BASE_TRANSFORM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_DSP_BASE_TRANSFORM, GstHailoDspBaseTransformClass))
#define GST_IS_HAILO_DSP_BASE_TRANSFORM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_DSP_BASE_TRANSFORM))
#define GST_IS_HAILO_DSP_BASE_TRANSFORM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_DSP_BASE_TRANSFORM))
#define GST_HAILO_DSP_BASE_TRANSFORM_CAST(obj) \
  ((GstHailoDspBaseTransform *)(obj))
#define GST_HAILO_DSP_BASE_TRANSFORM_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_HAILO_DSP_BASE_TRANSFORM, GstHailoDspBaseTransformClass))

typedef struct _GstHailoDspBaseTransform GstHailoDspBaseTransform;
typedef struct _GstHailoDspBaseTransformClass GstHailoDspBaseTransformClass;

struct _GstHailoDspBaseTransform
{
  GstBaseTransform parent;
  gboolean pool_is_active;
  guint bufferpool_max_size;
  guint bufferpool_min_size;
  guint bufferpool_padding;
  // add any additional instance variables here
};

struct _GstHailoDspBaseTransformClass
{
  GstBaseTransformClass parent_class;
  // add any additional class variables here
};

G_GNUC_INTERNAL GType gst_hailo_dsp_base_transform_get_type(void);

G_END_DECLS