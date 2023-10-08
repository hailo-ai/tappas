#include "gsthailodsp.h"
#include <string.h>
#include <stdio.h>
#include <gst/gst.h>

static dsp_device device = NULL;
static guint dsp_device_refcount = 0;

static const char *interpolations_strings[INTERPOLATION_TYPE_COUNT] = {
    [INTERPOLATION_TYPE_NEAREST_NEIGHBOR] = "nearest",
    [INTERPOLATION_TYPE_BILINEAR] = "bilinear",
    [INTERPOLATION_TYPE_AREA] = "area",
    [INTERPOLATION_TYPE_BICUBIC] = "bicubic"};

/**
 * Create a DSP device, and store it globally
 * The function requests a device from the DSP library.
 * @return dsp_status
 */
static dsp_status create_device()
{
    dsp_status create_device_status = DSP_UNINITIALIZED;
    // If the device is not initialized, initialize it, else return SUCCESS
    if (device == NULL)
    {
        GST_CAT_DEBUG(GST_CAT_DEFAULT, "Openning DSP device");
        create_device_status = dsp_create_device(&device);
        if (create_device_status != DSP_SUCCESS)
        {
            GST_CAT_ERROR(GST_CAT_DEFAULT, "Open DSP device failed with status %d", create_device_status);
        }
    }

    return DSP_SUCCESS;
}

/**
 * Release the DSP device.
 * The function releases the device using the DSP library.
 * If there are other references to the device, just decrement the refcount and skip the release.
 * @return dsp_status
 */
dsp_status release_device()
{
    if (device == NULL)
    {
        GST_CAT_WARNING(GST_CAT_DEFAULT, "Release device skipped: Dsp device is already NULL");
        return DSP_SUCCESS;
    }

    dsp_device_refcount--;
    if (dsp_device_refcount > 0)
    {
        GST_CAT_INFO(GST_CAT_DEFAULT, "Release dsp device skipped, refcount is %d", dsp_device_refcount);
    }
    else
    {
        GST_CAT_DEBUG(GST_CAT_DEFAULT, "Releasing dsp device, refcount is %d", dsp_device_refcount);
        dsp_status status = dsp_release_device(device);
        if (status != DSP_SUCCESS)
        {
            GST_CAT_ERROR(GST_CAT_DEFAULT, "Release device failed with status %d", status);
            return status;
        }
        device = NULL;
        GST_CAT_INFO(GST_CAT_DEFAULT, "Dsp device released successfully");
    }

    return DSP_SUCCESS;
}

/**
 * Acquire the DSP device.
 * This function creates the DSP device using the DSP library once, and then increases the reference count.
 *
 * @return dsp_status
 */
dsp_status acquire_device()
{
    if (device == NULL)
    {
        dsp_status status = create_device();
        if (status != DSP_SUCCESS)
            return status;
    }
    dsp_device_refcount++;
    GST_CAT_DEBUG(GST_CAT_DEFAULT, "Acquired dsp device, refcount is %d", dsp_device_refcount);
    return DSP_SUCCESS;
}

/**
 * Create a buffer on the DSP
 * The function requests a buffer from the DSP library.
 * The buffer can be used later for DSP operations.
 * @param[in] size the size of the buffer to create
 * @param[out] buffer a pointer to a buffer - DSP library will allocate the buffer
 * @return dsp_status
 */
dsp_status create_hailo_dsp_buffer(size_t size, void **buffer)
{
    if (device != NULL)
    {
        GST_CAT_DEBUG(GST_CAT_DEFAULT, "Creating dsp buffer with size %ld", size);
        dsp_status status = dsp_create_buffer(device, size, buffer);
        if (status != DSP_SUCCESS)
        {
            GST_CAT_ERROR(GST_CAT_DEFAULT, "Create buffer failed with status %d", status);
            return status;
        }
    }
    else
    {
        GST_CAT_ERROR(GST_CAT_DEFAULT, "Create buffer failed: device is NULL");
        return DSP_UNINITIALIZED;
    }

    return DSP_SUCCESS;
}

/**
 * Release a buffer allocated by the DSP
 * @param[in] buffer the buffer to release
 * @return dsp_status
 */
dsp_status release_hailo_dsp_buffer(void *buffer)
{
    if (device == NULL)
    {
        GST_CAT_ERROR(GST_CAT_DEFAULT, "DSP release buffer failed: device is NULL");
        return DSP_UNINITIALIZED;
    }

    GST_CAT_DEBUG(GST_CAT_DEFAULT, "performing DSP buffer release");
    dsp_status status = dsp_release_buffer(device, buffer);
    if (status != DSP_SUCCESS)
    {
        GST_CAT_ERROR(GST_CAT_DEFAULT, "DSP release buffer failed with status %d", status);
        return status;
    }

    GST_CAT_DEBUG(GST_CAT_DEFAULT, "DSP buffer released successfully");
    return DSP_SUCCESS;
}

/**
 * Perform DSP Resize
 * The function calls the DSP library to perform resize on a given buffer.
 * DSP will place the result in the output buffer.
 *
 * @param[in] input_image_properties input image properties
 * @param[in] output_image_properties output image properties
 * @param[in] dsp_interpolation_type interpolation type to use
 * @return dsp_status
 */
dsp_status perform_dsp_resize(dsp_image_properties_t *input_image_properties, dsp_image_properties_t *output_image_properties, dsp_interpolation_type_t dsp_interpolation_type)
{
    if (device == NULL)
    {
        GST_CAT_ERROR(GST_CAT_DEFAULT, "Perform DSP crop and resize ERROR: Device is NULL");
        return DSP_UNINITIALIZED;
    }

    dsp_resize_params_t resize_params = {
        .src = input_image_properties,
        .dst = output_image_properties,
        .interpolation = dsp_interpolation_type,
    };

    GST_CAT_DEBUG(GST_CAT_DEFAULT,
                "Perform DSP resize %s (no crop) to destination width: %ld, destination height: %ld",
                interpolations_strings[dsp_interpolation_type], output_image_properties->width, output_image_properties->height);
    dsp_status status = dsp_resize(device, &resize_params);

    if (status != DSP_SUCCESS)
    {
        GST_CAT_ERROR(GST_CAT_DEFAULT, "DSP Resize command failed with status %d", status);
        return status;
    }

    GST_CAT_DEBUG(GST_CAT_DEFAULT, "DSP  Resize command completed successfully");
    return DSP_SUCCESS;
}

/**
 * Perform DSP crop and resize
 * The function calls the DSP library to perform crop and resize on a given buffer.
 * DSP will place the result in the output buffer.
 *
 * @param[in] input_image_properties input image properties
 * @param[in] output_image_properties output image properties
 * @param[in] args crop and resize arguments
 * @param[in] dsp_interpolation_type interpolation type to use
 * @return dsp_status
 */
dsp_status perform_dsp_crop_and_resize(dsp_image_properties_t *input_image_properties, dsp_image_properties_t *output_image_properties,
                                            crop_resize_dims_t args, dsp_interpolation_type_t dsp_interpolation_type)
{
    if (device == NULL)
    {
        GST_CAT_ERROR(GST_CAT_DEFAULT, "Perform DSP crop and resize ERROR: Device is NULL");
        return DSP_UNINITIALIZED;
    }

    dsp_resize_params_t resize_params = {
        .src = input_image_properties,
        .dst = output_image_properties,
        .interpolation = dsp_interpolation_type,
    };

    dsp_status status;
    if (args.perform_crop)
    {
        GST_CAT_DEBUG(GST_CAT_DEFAULT,
                      "Perform DSP crop & resize %s to destination width: %ld, destination height: %ld, crop: (%ld,%ld)-(%ld-%ld)",
                      interpolations_strings[dsp_interpolation_type], args.destination_width, args.destination_height,
                      args.crop_start_x, args.crop_start_y, args.crop_end_x, args.crop_end_y);

        dsp_crop_api_t crop_params = {
            .start_x = args.crop_start_x,
            .start_y = args.crop_start_y,
            .end_x = args.crop_end_x,
            .end_y = args.crop_end_y,
        };
        status = dsp_crop_and_resize(device, &resize_params, &crop_params);
    }
    else
    {
        GST_CAT_DEBUG(GST_CAT_DEFAULT,
                      "Perform DSP resize %s (no crop) to destination width: %ld, destination height: %ld",
                      interpolations_strings[dsp_interpolation_type], args.destination_width, args.destination_height);
        status = dsp_resize(device, &resize_params);
    }

    if (status != DSP_SUCCESS)
    {
        GST_CAT_ERROR(GST_CAT_DEFAULT, "DSP Crop & resize command failed with status %d", status);
        return status;
    }

    GST_CAT_DEBUG(GST_CAT_DEFAULT, "DSP Crop & resize command completed successfully");
    return DSP_SUCCESS;
}

/**
 * Perform DSP blending using multiple overlays
 * The function calls the DSP library to perform blending between one
 * main buffer and multiple overlay buffers.
 * DSP will blend the overlay buffers onto the image frame in place
 *
 * @param[in] image_frame pointer to input image to blend on
 * @param[in] overlay pointer to input images to overlay with
 * @param[in] overlays_count number of overlays to blend
 * @return dsp_status
 */
dsp_status perform_dsp_multiblend(dsp_image_properties_t *image_frame, dsp_overlay_properties_t *overlay, size_t overlays_count)
{
    return dsp_blend(device, image_frame, overlay, overlays_count);
}

/**
 * Free DSP struct resources
 *
 * @param[in] overlay_properties pointer to the properties to free
 * @return void
 */
void free_overlay_property_planes(dsp_overlay_properties_t *overlay_properties)
{
    free_image_property_planes(&(overlay_properties->overlay));
}

/**
 * Free DSP struct resources
 *
 * @param[in] image_properties pointer to the properties to free
 * @return void
 */
void free_image_property_planes(dsp_image_properties_t *image_properties)
{
    free(image_properties->planes);
}

/**
 * Creates and populates a dsp_image_properties_t
 * struct with data of a given GstVideoFrame
 *
 * @param[in] video_frame Gst video frame with plane data
 * @return populated dsp_image_properties_t of the video frame
 */
dsp_image_properties_t create_image_properties_from_video_frame(GstVideoFrame *video_frame)
{
    GstVideoFormat format = GST_VIDEO_FRAME_FORMAT(video_frame);
    dsp_image_properties_t image_properties;
    size_t image_width = GST_VIDEO_FRAME_WIDTH(video_frame);
    size_t image_height = GST_VIDEO_FRAME_HEIGHT(video_frame);
    size_t n_planes = GST_VIDEO_FRAME_N_PLANES(video_frame);

    switch (format)
    {
    case GST_VIDEO_FORMAT_RGB:
    {
        // RGB is non-planar, since all channels are interleaved, we treat the whole image as 1 plane
        void *data = (void *)GST_VIDEO_FRAME_PLANE_DATA(video_frame, 0);
        size_t input_line_stride = GST_VIDEO_FRAME_PLANE_STRIDE(video_frame, 0);
        size_t input_size = GST_VIDEO_FRAME_SIZE(video_frame);

        // Allocate memory for the plane
        dsp_data_plane_t *plane = malloc(sizeof(*plane));
        plane->userptr = data;
        plane->bytesperline = input_line_stride;
        plane->bytesused = input_size;

        // Fill in dsp_image_properties_t values
        image_properties = (dsp_image_properties_t){
            .width = image_width,
            .height = image_height,
            .planes = plane,
            .planes_count = n_planes,
            .format = DSP_IMAGE_FORMAT_RGB};
        break;
    }
    case GST_VIDEO_FORMAT_YUY2:
    {
        GST_CAT_ERROR(GST_CAT_DEFAULT, "DSP image properties from GstVideoFrame failed: YUY2 not yet supported.");
        break;
    }
    case GST_VIDEO_FORMAT_NV12:
    {
        // NV12 is semi-planar, where the Y channel is a seprate plane from the UV channels
        // Gather y channel info
        void *y_channel_data = (void *)GST_VIDEO_FRAME_PLANE_DATA(video_frame, 0);
        size_t y_channel_stride = GST_VIDEO_FRAME_PLANE_STRIDE(video_frame, 0);
        size_t y_channel_size = y_channel_stride * image_height;
        dsp_data_plane_t y_plane_data = {
            .userptr = y_channel_data,
            .bytesperline = y_channel_stride,
            .bytesused = y_channel_size,
        };
        // Gather uv channel info
        void *uv_channel_data = (void *)GST_VIDEO_FRAME_PLANE_DATA(video_frame, 1);
        size_t uv_channel_stride = GST_VIDEO_FRAME_PLANE_STRIDE(video_frame, 1);
        size_t uv_channel_size = uv_channel_stride * image_height / 2;
        dsp_data_plane_t uv_plane_data = {
            .userptr = uv_channel_data,
            .bytesperline = uv_channel_stride,
            .bytesused = uv_channel_size,
        };
        dsp_data_plane_t *yuv_planes = malloc(sizeof(*yuv_planes) * 2);
        yuv_planes[0] = y_plane_data;
        yuv_planes[1] = uv_plane_data;

        GST_CAT_DEBUG(GST_CAT_DEFAULT, "DSP image properties from GstVideoFrame: buffer offset = %zu", GST_BUFFER_OFFSET(video_frame->buffer));
        GST_CAT_DEBUG(GST_CAT_DEFAULT, "DSP image properties from GstVideoFrame: NV12, y ptr %p, y stride %zu, y size %zu, uv ptr %p, uv stride %zu, uv size %zu",
                      y_channel_data, y_channel_stride, y_channel_size, uv_channel_data, uv_channel_stride, uv_channel_size);
        // Fill in dsp_image_properties_t values
        image_properties = (dsp_image_properties_t){
            .width = image_width,
            .height = image_height,
            .planes = yuv_planes,
            .planes_count = n_planes,
            .format = DSP_IMAGE_FORMAT_NV12};
        break;
    }
    case GST_VIDEO_FORMAT_A420:
    {
        // A420 is fully planar (4:4:2:0), essentially I420 YUV with an extra alpha channel at full size
        // Gather channel info
        dsp_data_plane_t *a420_planes = malloc(sizeof(*a420_planes) * n_planes);
        for (guint i = 0; i < n_planes; ++i)
        {
            void *channel_data = (void *)GST_VIDEO_FRAME_PLANE_DATA(video_frame, i);
            size_t channel_stride = GST_VIDEO_FRAME_PLANE_STRIDE(video_frame, i);
            size_t channel_size = channel_stride * image_height;
            if (i == 1 || i == 2)
                channel_size /= 2;
            dsp_data_plane_t plane_data = {
                .userptr = channel_data,
                .bytesperline = channel_stride,
                .bytesused = channel_size,
            };
            a420_planes[i] = plane_data;
        }

        // Fill in dsp_image_properties_t values
        image_properties = (dsp_image_properties_t){
            .width = image_width,
            .height = image_height,
            .planes = a420_planes,
            .planes_count = n_planes,
            .format = DSP_IMAGE_FORMAT_A420};
        break;
    }
    default:
        break;
    }

    return image_properties;
}
