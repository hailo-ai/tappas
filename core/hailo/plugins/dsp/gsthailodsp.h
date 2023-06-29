#pragma once

#include "hailo/hailodsp.h"
#include <stdint.h>
#include <gst/video/video.h>

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

#undef __BEGIN_DECLS
#undef __END_DECLS
#ifdef __cplusplus
#define __BEGIN_DECLS \
    extern "C"        \
    {
#define __END_DECLS }
#else
#define __BEGIN_DECLS /* empty */
#define __END_DECLS   /* empty */
#endif


__BEGIN_DECLS

typedef struct {
    int perform_crop;
    size_t crop_start_x;
    size_t crop_end_x;
    size_t crop_start_y;
    size_t crop_end_y;
    size_t destination_width;
    size_t destination_height;
} crop_resize_dims_t;

dsp_status create_hailo_dsp_buffer(size_t size, void **buffer);
dsp_status release_hailo_dsp_buffer(void *buffer);
dsp_status release_device();
dsp_status acquire_device();
dsp_image_properties_t create_image_properties_from_video_frame(GstVideoFrame *video_frame);
dsp_status perform_dsp_resize(dsp_image_properties_t *input_image_properties, dsp_image_properties_t *output_image_properties, dsp_interpolation_type_t dsp_interpolation_type);
dsp_status perform_dsp_crop_and_resize(dsp_image_properties_t *input_image_properties, dsp_image_properties_t *output_image_properties,
                                            crop_resize_dims_t args, dsp_interpolation_type_t dsp_interpolation_type);
dsp_status perform_dsp_multiblend(dsp_image_properties_t *image_frame, dsp_overlay_properties_t *overlay, size_t overlays_count);
void free_overlay_property_planes(dsp_overlay_properties_t *overlay_properties);
void free_image_property_planes(dsp_image_properties_t *image_properties);

extern dsp_interpolation_type_t dsp_interpolation_type;
extern dsp_status dsp_command_status;
__END_DECLS
