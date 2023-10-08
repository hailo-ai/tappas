#include <gst/gst.h>

#include "dsp/gsthailodspbufferpool.hpp"
#include "dsp/gsthailodsp.h"

G_DEFINE_TYPE(GstHailoDspBufferPool, gst_hailo_dsp_buffer_pool, GST_TYPE_BUFFER_POOL)

static GstFlowReturn gst_hailo_dsp_buffer_pool_alloc_buffer(GstBufferPool *pool, GstBuffer **output_buffer_ptr, GstBufferPoolAcquireParams *params);
static void gst_hailo_dsp_buffer_pool_free_buffer(GstBufferPool *pool, GstBuffer *buffer);
static void gst_hailo_dsp_buffer_pool_dispose(GObject *object);

static void
gst_hailo_dsp_buffer_pool_class_init(GstHailoDspBufferPoolClass *klass)
{
    GObjectClass *const object_class = G_OBJECT_CLASS(klass);
    GstBufferPoolClass *const pool_class = GST_BUFFER_POOL_CLASS(klass);

    GST_INFO_OBJECT(object_class, "Hailo DSP buffer pool class init");

    pool_class->alloc_buffer = GST_DEBUG_FUNCPTR(gst_hailo_dsp_buffer_pool_alloc_buffer);
    pool_class->free_buffer = GST_DEBUG_FUNCPTR(gst_hailo_dsp_buffer_pool_free_buffer);
    object_class->dispose = GST_DEBUG_FUNCPTR(gst_hailo_dsp_buffer_pool_dispose);
}

static void
gst_hailo_dsp_buffer_pool_dispose(GObject *object)
{
    G_OBJECT_CLASS(gst_hailo_dsp_buffer_pool_parent_class)->dispose(object);
    GstHailoDspBufferPool *pool = GST_HAILO_DSP_BUFFER_POOL(object);
    GST_INFO_OBJECT(pool, "Hailo DSP buffer pool dispose");
    // Release DSP device
    if (pool->config)
    {
        gst_structure_free(pool->config);
        pool->config = NULL;
    }
    dsp_status result = release_device();
    if (result != DSP_SUCCESS)
    {
        GST_ERROR_OBJECT(pool, "Release DSP device failed with status code %d", result);
    }
}

static void
gst_hailo_dsp_buffer_pool_init(GstHailoDspBufferPool *pool)
{
    GST_INFO_OBJECT(pool, "New Hailo DSP buffer pool");
    // Acquire DSP device
    dsp_status status = acquire_device();
    if (status != DSP_SUCCESS)
    {
        GST_ERROR_OBJECT(pool, "Accuire DSP device failed with status code %d", status);
    }
}

GstBufferPool *
gst_hailo_dsp_buffer_pool_new(guint padding)
{
    GstHailoDspBufferPool *pool = GST_HAILO_DSP_BUFFER_POOL(g_object_new(GST_TYPE_HAILO_DSP_BUFFER_POOL, NULL));
    pool->padding = padding;
    pool->config = NULL;
    return GST_BUFFER_POOL_CAST(pool);
}

static GstFlowReturn
gst_hailo_dsp_buffer_pool_alloc_buffer(GstBufferPool *pool, GstBuffer **output_buffer_ptr, GstBufferPoolAcquireParams *params)
{
    GstHailoDspBufferPool *hailo_dsp_pool = GST_HAILO_DSP_BUFFER_POOL(pool);
    guint buffer_size=0;

    // Get the size of a buffer from the config of the pool
    if (!hailo_dsp_pool->config)
    {
        hailo_dsp_pool->config = gst_buffer_pool_get_config(pool);
    }
    gst_buffer_pool_config_get_params(hailo_dsp_pool->config, NULL, &buffer_size, NULL, NULL);

    // Validate the size of the buffer
    if (buffer_size == 0)
    {
        GST_ERROR_OBJECT(hailo_dsp_pool, "Invalid buffer size");
        return GST_FLOW_ERROR;
    }
    GST_INFO_OBJECT(hailo_dsp_pool, "Allocating buffer of size %d with padding %d", buffer_size, hailo_dsp_pool->padding);

    void *buffer_ptr = NULL;
    dsp_status result = create_hailo_dsp_buffer((size_t)buffer_size+hailo_dsp_pool->padding, &buffer_ptr);
    if (result != DSP_SUCCESS)
    {
        GST_ERROR_OBJECT(pool, "Failed to create buffer with status code %d", result);
        return GST_FLOW_ERROR;
    }
    GST_INFO_OBJECT(hailo_dsp_pool, "Allocated buffer of size %d from dsp memory", buffer_size);

    void *aligned_buffer_ptr = (void *)(((size_t)buffer_ptr + hailo_dsp_pool->padding));
    *output_buffer_ptr = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_PHYSICALLY_CONTIGUOUS,
                                                    aligned_buffer_ptr, (size_t)buffer_size, 0, (size_t)buffer_size, NULL, NULL);

    GST_INFO_OBJECT(hailo_dsp_pool, "Allocated buffer memory wrapped");

    return GST_FLOW_OK;
}

static void
gst_hailo_dsp_buffer_pool_free_buffer(GstBufferPool *pool, GstBuffer *buffer)
{
    GstHailoDspBufferPool *hailo_dsp_pool = GST_HAILO_DSP_BUFFER_POOL(pool);
    GST_DEBUG_OBJECT(hailo_dsp_pool, "Freeing buffer %p with padding %d", buffer, hailo_dsp_pool->padding);
    guint memory_count = gst_buffer_n_memory(buffer);
    for (guint i = 0; i < memory_count; i++)
    {
        GstMemory *memory = gst_buffer_peek_memory(buffer, i);
        GstMapInfo memory_map_info;
        gst_memory_map(memory, &memory_map_info, GST_MAP_READ);
        void *buffer_ptr = memory_map_info.data;

        void *aligned_buffer_ptr = (void *)(((size_t)buffer_ptr - hailo_dsp_pool->padding));
        dsp_status result = release_hailo_dsp_buffer(aligned_buffer_ptr);
        if (result != DSP_SUCCESS)
        {
            GST_ERROR_OBJECT(hailo_dsp_pool, "Failed to release dsp buffer %p number %d out of %d", buffer_ptr, (i+1), memory_count);
        }
        else
        {
            GST_INFO_OBJECT(hailo_dsp_pool, "Released dsp buffer %p number %d out of %d", buffer_ptr, (i+1), memory_count);
        }

        gst_memory_unmap(memory, &memory_map_info);
    }

    gst_buffer_unref(buffer);
}
