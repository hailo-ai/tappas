#include "gsthailodspbufferpoolutils.hpp"
#include "dsp/gsthailodspbufferpool.hpp"

gboolean
gst_is_hailo_dsp_pool_type(GstBufferPool *pool)
{
    return GST_IS_HAILO_DSP_BUFFER_POOL(pool);
}

gboolean
gst_hailo_dsp_configure_pool(GstDebugCategory *category, GstBufferPool *pool,
                         GstCaps *caps, gsize size, guint min_buffers, guint max_buffers)
{
    GstStructure *config = NULL;

    g_return_val_if_fail(size > 0, FALSE);
    g_return_val_if_fail(max_buffers > 0, FALSE);

    config = gst_buffer_pool_get_config(pool);

    gst_buffer_pool_config_set_params(config, caps, size, min_buffers, max_buffers);

    if (!gst_buffer_pool_set_config(pool, config))
    {
        GST_ERROR_OBJECT(pool, "Unable to set pool configuration");
        gst_object_unref(pool);
        return FALSE;
    }

    if (!gst_buffer_pool_config_validate_params(config, caps, size, min_buffers,
                                                max_buffers))
    {
        GST_ERROR_OBJECT(pool, "Pool configuration validation failed");
        gst_object_unref(pool);
        return FALSE;
    }

    return TRUE;
}

GstBufferPool *
gst_hailo_dsp_create_new_pool(GstDebugCategory *category, GstQuery *query, guint min_buffers,
                       guint max_buffers, gsize size, guint padding)
{
    GstCaps *caps = NULL;

    // Create a new bufferpool object
    GstBufferPool *pool = gst_hailo_dsp_buffer_pool_new(padding);
    if (pool == NULL)
    {
        GST_CAT_ERROR(category, "Create Hailo pool failed");
        return NULL;
    }

    gst_query_parse_allocation(query, &caps, NULL);

    // Configure the bufferpool
    gboolean res = gst_hailo_dsp_configure_pool(category, pool, caps, size, min_buffers, max_buffers);
    if (res == FALSE)
    {
        GST_ERROR_OBJECT(pool, "Unable to configure pool");
        return NULL;
    }

    gst_caps_unref(caps);
    GST_DEBUG_OBJECT(pool, "Dsp Bufferpool created with buffer size: %ld min buffers: %d max buffers: %d and padding: %d",
                     size, min_buffers, max_buffers, padding);

    return pool;
}

GstBufferPool * 
gst_create_hailo_dsp_bufferpool_from_allocation_query(GstElement *element, GstQuery *query, guint bufferpool_min_size, guint bufferpool_max_size, guint padding)
{
    GstCaps *caps;
    GstAllocator *allocator = NULL;
    gboolean need_pool = FALSE;

    gst_query_parse_allocation(query, &caps, &need_pool);

    if (caps == NULL)
    {
        GST_ERROR_OBJECT(element, "Bufferpool creation from alloc query - Alloc query Caps is NULL");
        return NULL;
    }

    if (!gst_caps_is_fixed(caps))
    {
        GST_ERROR_OBJECT(element, "Bufferpool creation from alloc query - Caps is not fixed");
        gst_caps_unref(caps);
        return NULL;
    }

    GST_INFO_OBJECT(element, "Bufferpool creation from alloc query - caps are fixed");

    // Get the width and height of the caps
    GstVideoInfo *video_info = gst_video_info_new();
    gst_video_info_from_caps(video_info, caps);
    gst_caps_unref(caps);

    guint buffer_size = video_info->size;

    GST_INFO_OBJECT(element, "Bufferpool creation from alloc query - Creating new pool with buffer size: %d", buffer_size);

    gst_video_info_free(video_info);
    GstBufferPool *pool = gst_hailo_dsp_create_new_pool(GST_CAT_DEFAULT, query, bufferpool_min_size, bufferpool_max_size, buffer_size, padding);

    if (pool == NULL)
    {
        GST_ERROR_OBJECT(element, "Bufferpool creation from alloc query - Pool creation failed");
        return NULL;
    }

    GstAllocationParams params;

    gst_allocation_params_init(&params);

    // Get how many pools are in the query
    guint n_pools = gst_query_get_n_allocation_pools(query);
    if (n_pools > 0)
    {
        gst_query_set_nth_allocation_param(query, 0, allocator, &params);
        gst_query_set_nth_allocation_pool(query, 0, pool, buffer_size, bufferpool_min_size, bufferpool_max_size);
    }
    gst_query_add_allocation_param(query, allocator, &params);
    gst_query_add_allocation_pool(query, pool, buffer_size, bufferpool_min_size, bufferpool_max_size);

    if (!gst_buffer_pool_set_active(pool, TRUE))
    {
        GST_ERROR_OBJECT(element, "Bufferpool creation from alloc query - Unable to set pool active");
        if (allocator)
        {
            gst_object_unref(allocator);
        }
        return NULL;
    }

    if (gst_query_get_n_allocation_pools(query) == 0)
    {
        GST_ERROR_OBJECT(element, "Bufferpool creation from alloc query - No pools in query");
        if (allocator)
        {
            gst_object_unref(allocator);
        }
        return NULL;
    }

    if (allocator)
    {
        gst_object_unref(allocator);
    }

    return pool;
}