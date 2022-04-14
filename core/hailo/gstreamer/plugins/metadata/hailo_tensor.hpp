/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/**
 * @file hailo_tensor.hpp
 * @authors MAD Team.
 **/

#ifndef _HAILO_TENSOR_HPP_
#define _HAILO_TENSOR_HPP_
#include <gst/video/video.h>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include "tensor_meta.hpp"
#include "hailo/hailort.h"

class HailoTensor
{
public:
    GstParentBufferMeta *buffer_meta;
    GstMapInfo info;
    const hailo_vstream_info_t *vstream_info;
    float qp_zp;
    float qp_scale;
    uint32_t size;
    uint8_t *data;
    uint16_t *data_as_uint16;
    std::string name;
    uint32_t order;
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    uint32_t number_of_classes;
    uint32_t max_bboxes_per_class;
    HailoTensor(GstParentBufferMeta *buffer_meta) : buffer_meta(buffer_meta)
    {
        data = nullptr;
        data_as_uint16 = nullptr;
        info = {0};

        // TODO: Add error handling
        (void)gst_buffer_map(buffer_meta->buffer, &info, GST_MAP_READWRITE);
        vstream_info = &(reinterpret_cast<GstHailoTensorMeta *>(gst_buffer_get_meta(buffer_meta->buffer, g_type_from_name(TENSOR_META_API_NAME)))->info);

        if (vstream_info->format.type == HAILO_FORMAT_TYPE_UINT16)
            data_as_uint16 = reinterpret_cast<uint16_t *>(info.data);
        else
            data = info.data;

        name = vstream_info->name;
        qp_zp = vstream_info->quant_info.qp_zp;
        qp_scale = vstream_info->quant_info.qp_scale;
        order = vstream_info->format.order;

        if (HAILO_FORMAT_ORDER_HAILO_NMS != order)
        {
            width = vstream_info->shape.width;
            height = vstream_info->shape.height;
            channels = vstream_info->shape.features;
        }
        else
        {
            number_of_classes = vstream_info->nms_shape.number_of_classes;
            max_bboxes_per_class = vstream_info->nms_shape.max_bboxes_per_class;
        }
    }
    ~HailoTensor()
    {
        gst_buffer_unmap(buffer_meta->buffer, &info);
    }
    uint8_t get(uint row, uint col, uint channel)
    {
        int pos = (height * channels) * row + channels * col + channel;
        return data[pos];
    }
    float get_full_percision(uint row, uint col, uint channel)
    {
        int pos = (height * channels) * row + channels * col + channel;
        return fix_scale(data[pos]);
    }
    float fix_scale(uint8_t num)
    {
        return (num - qp_zp) * qp_scale;
    }
};

using HailoTensorPtr = std::shared_ptr<HailoTensor>;

#endif /* _HAILO_TENSOR_HPP_ */
