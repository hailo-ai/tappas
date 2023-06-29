#include "dsp/gsthailodspbasetransform.hpp"
#include "dsp/gsthailodspbufferpool.hpp"
#include "dsp/gsthailodspbufferpoolutils.hpp"
#include <gst/gst.h>

GST_DEBUG_CATEGORY_STATIC(gst_hailo_dsp_base_transform_debug);
#define GST_CAT_DEFAULT gst_hailo_dsp_base_transform_debug

enum
{
    PROP_0,
    PROP_POOL_SIZE,
};

#define _debug_init \
    GST_DEBUG_CATEGORY_INIT(gst_hailo_dsp_base_transform_debug, "hailodspbasetransform", 0, "Hailo DSP base transform element");

#define gst_hailo_dsp_base_transform_parent_class parent_class
G_DEFINE_ABSTRACT_TYPE_WITH_CODE(GstHailoDspBaseTransform, gst_hailo_dsp_base_transform, GST_TYPE_BASE_TRANSFORM, _debug_init);

static gboolean
gst_hailo_dsp_base_transform_propose_allocation(GstBaseTransform *trans,
                                                GstQuery *decide_query, GstQuery *query);
static void gst_hailo_dsp_base_transform_set_property(GObject *object,
                                                      guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_hailo_dsp_base_transform_get_property(GObject *object,
                                                      guint property_id, GValue *value, GParamSpec *pspec);
static gboolean gst_hailo_dsp_base_transform_decide_allocation(GstBaseTransform *trans, GstQuery *query);
static GstFlowReturn gst_hailo_dsp_base_prepare_output_buffer(GstBaseTransform *trans,
                                                              GstBuffer *inbuf, GstBuffer **outbuf);

static void
gst_hailo_dsp_base_transform_class_init(GstHailoDspBaseTransformClass *klass)
{
    GObjectClass *const object_class = G_OBJECT_CLASS(klass);
    GstBaseTransformClass *const base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);

    object_class->set_property = gst_hailo_dsp_base_transform_set_property;
    object_class->get_property = gst_hailo_dsp_base_transform_get_property;

    base_transform_class->propose_allocation = GST_DEBUG_FUNCPTR(gst_hailo_dsp_base_transform_propose_allocation);
    base_transform_class->decide_allocation = GST_DEBUG_FUNCPTR(gst_hailo_dsp_base_transform_decide_allocation);
    base_transform_class->prepare_output_buffer = GST_DEBUG_FUNCPTR(gst_hailo_dsp_base_prepare_output_buffer);

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                          "Hailo DSP base transform",
                                          "Hailo/Tools",
                                          "Base class for Hailo15 DSP transformations",
                                          "hailo.ai <contact@hailo.ai>");

    g_object_class_install_property(object_class, PROP_POOL_SIZE,
                                    g_param_spec_uint("pool-size", "Pool Size",
                                                      "Size of the pool of buffers to use for cropping. Default 10",
                                                      1, G_MAXINT, 10,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
}

static void
gst_hailo_dsp_base_transform_init(GstHailoDspBaseTransform *self)
{
    self->bufferpool_max_size = 10;
    self->bufferpool_min_size = 1;
    self->pool_is_active = FALSE;
    self->bufferpool_padding = 0;
}

static gboolean
gst_hailo_dsp_base_transform_propose_allocation(GstBaseTransform *trans,
                                                GstQuery *decide_query, GstQuery *query)
{
    gboolean ret;

    gst_query_add_allocation_meta(query, GST_VIDEO_META_API_TYPE, NULL);
    ret = GST_BASE_TRANSFORM_CLASS(parent_class)->propose_allocation(trans, decide_query, query);
    return ret;
}

static GstFlowReturn
gst_hailo_dsp_base_prepare_output_buffer(GstBaseTransform *trans,
                                         GstBuffer *inbuf, GstBuffer **outbuf)
{
    GstFlowReturn ret = GST_FLOW_OK;
    GstBaseTransformClass *base_transform_class;

    base_transform_class = GST_BASE_TRANSFORM_GET_CLASS(trans);
    GstHailoDspBaseTransform *hailo_dsp_transform = GST_HAILO_DSP_BASE_TRANSFORM(trans);

    GST_DEBUG_OBJECT(hailo_dsp_transform, "Preparing DSP output buffer");

    if (gst_base_transform_is_passthrough(trans))
    {
        // Passthrough mdoe - modify the incoming buffer, output = input
        GST_DEBUG_OBJECT(trans, "Passthrough: reusing input buffer");
        *outbuf = inbuf;
        return GST_FLOW_OK;
    }

    GST_DEBUG_OBJECT(hailo_dsp_transform, "NON passthrough: allocating new buffer");

    GstBufferPool *buffer_pool = gst_base_transform_get_buffer_pool(trans);

    if (!buffer_pool)
    {
        GST_ERROR_OBJECT(hailo_dsp_transform, "DSP buffer allocation requested form pool - but buffer pool is not initialized");
        return GST_FLOW_ERROR;
    }

    if (!hailo_dsp_transform->pool_is_active)
    {
        GST_DEBUG_OBJECT(hailo_dsp_transform, "Setting pool %p active", buffer_pool);
        if (!gst_buffer_pool_set_active(buffer_pool, TRUE))
        {
            GST_ERROR_OBJECT(hailo_dsp_transform, "Failed to activate buffer pool");
            return GST_FLOW_ERROR;
        }
        hailo_dsp_transform->pool_is_active = TRUE;
    }

    GST_DEBUG_OBJECT(hailo_dsp_transform, "Buffer pool is active, Acquiring buffer from pool %p", buffer_pool);

    ret = gst_buffer_pool_acquire_buffer(GST_BUFFER_POOL(buffer_pool), outbuf, NULL);
    if (ret != GST_FLOW_OK)
    {
        GST_ERROR_OBJECT(hailo_dsp_transform, "Failed to acquire buffer from pool");
        return ret;
    }

    GST_DEBUG_OBJECT(hailo_dsp_transform, "Acquired buffer %p from pool", *outbuf);

    // copy the metadata
    if (base_transform_class->copy_metadata)
    {
        if (!base_transform_class->copy_metadata(trans, inbuf, *outbuf))
        {
            GST_ELEMENT_WARNING(trans, STREAM, NOT_IMPLEMENTED,
                                ("could not copy metadata"), (NULL));
        }
    }

    return ret;
}

static gboolean
gst_hailo_dsp_base_transform_decide_allocation(GstBaseTransform *trans,
                                               GstQuery *query)
{
    GstHailoDspBaseTransform *self = GST_HAILO_DSP_BASE_TRANSFORM(trans);
    GstElement *element = GST_ELEMENT_CAST(self);

    GST_DEBUG_OBJECT(self, "Performing decide allocation");

    GstBufferPool *pool = gst_create_hailo_dsp_bufferpool_from_allocation_query(element, query, self->bufferpool_min_size, self->bufferpool_max_size, self->bufferpool_padding);
    GST_DEBUG_OBJECT(self, "Created hailo buffer pool %p", pool);
    if (pool == NULL)
    {
        GST_ERROR_OBJECT(self, "Decide Allocation - Failed to create buffer pool");
        return FALSE;
    }
    GstBufferPool *existing_pool = gst_base_transform_get_buffer_pool(trans);
    GST_DEBUG_OBJECT(self, "Existing pool %p", existing_pool);

    // Replace existing pool with the new one
    if (existing_pool)
    {
        gst_object_unref(existing_pool);
    }
    existing_pool = pool;
    GST_DEBUG_OBJECT(self, "Existing pool moved to %p", existing_pool);

    self->pool_is_active = TRUE;
    gst_object_unref(pool);
    GST_INFO_OBJECT(self, "Decide allocation done - hailo buffer pool created");
    return TRUE;
}

static void
gst_hailo_dsp_base_transform_set_property(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
    GstHailoDspBaseTransform *self = GST_HAILO_DSP_BASE_TRANSFORM(object);

    switch (property_id)
    {
    case PROP_POOL_SIZE:
        self->bufferpool_max_size = g_value_get_uint(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
gst_hailo_dsp_base_transform_get_property(GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
    GstHailoDspBaseTransform *self = GST_HAILO_DSP_BASE_TRANSFORM(object);

    switch (property_id)
    {
    case PROP_POOL_SIZE:
        g_value_set_uint(value, self->bufferpool_max_size);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}