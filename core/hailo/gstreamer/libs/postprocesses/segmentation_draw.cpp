/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include "segmentation_draw.hpp"
#include "common/color_maps/cityscapes19.hpp"

#define PIXEL_SIZE 3
#define APPLY_MATRIX(colors, r, o, c) ((((colors[c * PIXEL_SIZE + o]) >> 1) + (r >> 1)))

void filter(HailoFramePtr hailo_frame)
{
    guint i, j;
    guint8 *tensor;
    gint r, g, b;
    guint8 color;
    gsize num_classes = sizeof(common::cityscapes19_colors) / sizeof(common::cityscapes19_colors[0]) / PIXEL_SIZE;
    auto tensors = hailo_frame->get_tensors();

    if (tensors.size() == 0)
    {
        return;
    }
    tensor = tensors[0]->data;
    auto offsets = hailo_frame->get_offsets();
    auto data = hailo_frame->plane_data;
    auto stride = hailo_frame->stride;
    auto pixel_stride = hailo_frame->pixel_stride;
    auto row_wrap = stride - pixel_stride * hailo_frame->width;

    for (i = 0; i < hailo_frame->height; i++)
    {
        for (j = 0; j < hailo_frame->width; j++)
        {
            r = data[offsets[0]];
            g = data[offsets[1]];
            b = data[offsets[2]];

            color = *tensor;
            if (color < num_classes)
            {
                r = APPLY_MATRIX(common::cityscapes19_colors, r, 0, color);
                g = APPLY_MATRIX(common::cityscapes19_colors, g, 1, color);
                b = APPLY_MATRIX(common::cityscapes19_colors, b, 2, color);
            }

            data[offsets[0]] = CLAMP(r, 0, 255);
            data[offsets[1]] = CLAMP(g, 0, 255);
            data[offsets[2]] = CLAMP(b, 0, 255);
            data += pixel_stride;
            tensor += 1;
        }
        data += row_wrap;
    }
}
