#include "dsp/gsthailodspbasetransform.hpp"
#include "dsp/upload2/gsthailoupload2.hpp"
#include <gst/gst.h>
#include <gst/video/video.h>
#include "dsp/gsthailodsp.h"

GST_DEBUG_CATEGORY_STATIC(gst_hailo_upload2_debug);
#define GST_CAT_DEFAULT gst_hailo_upload2_debug

#define _do_init \
    GST_DEBUG_CATEGORY_INIT(gst_hailo_upload2_debug, "hailoupload2", 0, "Hailo Upload2");
#define gst_hailo_upload2_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstHailoUpload2, gst_hailo_upload2, GST_TYPE_HAILO_DSP_BASE_TRANSFORM, _do_init);

static void gst_hailo_upload2_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_hailo_upload2_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static gboolean gst_hailo_upload2_propose_allocation(GstBaseTransform *trans, GstQuery *decide_query, GstQuery *query);

static void
gst_hailo_upload2_class_init(GstHailoUpload2Class *klass)
{
    GObjectClass *const object_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *const base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);

    object_class->set_property = gst_hailo_upload2_set_property;
    object_class->get_property = gst_hailo_upload2_get_property;

    base_transform_class->propose_allocation = GST_DEBUG_FUNCPTR(gst_hailo_upload2_propose_allocation);

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                          "Hailo Upload 2",
                                          "Hailo/Tools",
                                          "Manages a buffer pool using Hailo15 DSP memory, and propogates it upstream for pipeline usage.",
                                          "hailo.ai <contact@hailo.ai>");

    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS,
                                                            GST_CAPS_ANY));
    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
                                       gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
                                                            GST_CAPS_ANY));
}

static void
gst_hailo_upload2_init(GstHailoUpload2 *self)
{
}

static void gst_hailo_upload2_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    switch (prop_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void gst_hailo_upload2_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    switch (prop_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static gboolean
gst_hailo_upload2_propose_allocation (GstBaseTransform *trans,
                                      GstQuery *decide_query, GstQuery *query)
{
    GstHailoUpload2 *hailoupload = GST_HAILO_UPLOAD2(trans);
    GstHailoDspBaseTransform *dspbasetrans = GST_HAILO_DSP_BASE_TRANSFORM(trans);
    GST_DEBUG_OBJECT(hailoupload, "hailoupload2 propose allocation callback");
    gboolean ret;

    GstBufferPool *buffer_pool = gst_base_transform_get_buffer_pool(trans);
    // Get the size of a buffer from the config of the pool
    GstStructure *config = gst_buffer_pool_get_config(buffer_pool);
    guint buffer_size = 0;
    gst_buffer_pool_config_get_params(config, NULL, &buffer_size, NULL, NULL);

    gst_query_add_allocation_pool(query, buffer_pool, buffer_size, dspbasetrans->bufferpool_min_size, dspbasetrans->bufferpool_max_size);

    gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, NULL);
    ret = GST_BASE_TRANSFORM_CLASS(parent_class)->propose_allocation(trans, decide_query, query);
    return ret;
}