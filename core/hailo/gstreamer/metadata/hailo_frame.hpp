/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/**
 * @file hailo_frame.hpp
 * @authors MAD Team.
 **/

#ifndef _HAILO_FRAME_HPP_
#define _HAILO_FRAME_HPP_
#include <gst/video/video.h>
#include <stdexcept>
#include <memory>
#include <vector>
#include <map>
#include "tensor_meta.hpp"
#include "hailo_tensor.hpp"
#include "hailo_detection.hpp"

class HailoFrame
{
public:
    guint width;
    guint height;
    guint stride;
    guint pixel_stride;
    GstVideoFormat format;
    guint8 *plane_data;
    GstBuffer *buffer;
    gchar *_stream_id;
    HailoFrame(GstVideoFrame *frame, gchar *stream_id = nullptr)
        : _stream_id(stream_id), _frame(frame)
    {
        width = (guint)GST_VIDEO_FRAME_WIDTH(_frame);
        height = (guint)GST_VIDEO_FRAME_HEIGHT(_frame);
        plane_data = (guint8 *)GST_VIDEO_FRAME_PLANE_DATA(_frame, 0);
        format = _frame->info.finfo->format;
        buffer = _frame->buffer;
        stride = (guint)GST_VIDEO_FRAME_PLANE_STRIDE(_frame, 0);
        pixel_stride = (guint)GST_VIDEO_FRAME_COMP_PSTRIDE(_frame, 0);
    }
    ~HailoFrame()
    {
        (void)remove_all_tensors();
    }
    std::vector<HailoTensorPtr> get_tensors()
    {
        gpointer state = NULL;
        GstMeta *meta;
        std::vector<HailoTensorPtr> tensors;
        while ((meta = gst_buffer_iterate_meta_filtered(_frame->buffer, &state, GST_PARENT_BUFFER_META_API_TYPE)))
        {
            tensors.emplace_back(std::make_shared<HailoTensor>(reinterpret_cast<GstParentBufferMeta *>(meta)));
        }
        return tensors;
    }
    std::vector<HailoDetectionPtr> get_detections()
    {
        gpointer state = NULL;
        GstMeta *meta;
        std::vector<HailoDetectionPtr> _regions;
        while ((meta = gst_buffer_iterate_meta_filtered(_frame->buffer, &state, GST_VIDEO_REGION_OF_INTEREST_META_API_TYPE)))
        {
            _regions.emplace_back(std::make_shared<HailoDetection>(reinterpret_cast<GstVideoRegionOfInterestMeta *>(meta)));
        }
        return _regions;
    }

    /**
     * @brief Attach RegionOfInterest to this VideoFrame. This function takes ownership of region_tensor, if passed
     * @param x x coordinate of the upper left corner of bounding box
     * @param y y coordinate of the upper left corner of bounding box
     * @param w width of the bounding box
     * @param h height of the bounding box
     * @param label object label
     * @param confidence detection confidence
     * @param normalized if False, bounding box coordinates are pixel coordinates in range from 0 to image width/height.
    if True, bounding box coordinates normalized to [0,1] range.
     * @return new RegionOfInterest instance
     */
    void add_detection(float x, float y, float w, float h, std::string label, uint class_id,
                       float confidence = 0.0, // bool normalized = false,
                       GstStructure *landmarks = nullptr, GstStructure *segmentation_data = nullptr)
    {
        if (!gst_buffer_is_writable(buffer))
            throw std::runtime_error("Buffer is not writable.");

        GstVideoRegionOfInterestMeta *meta = gst_buffer_add_video_region_of_interest_meta(
            buffer, "detection", 0, 0, 0, 0);

        // Add detection tensor
        GstStructure *detection =
            gst_structure_new("detection", "x_min", G_TYPE_FLOAT, x, "x_max", G_TYPE_FLOAT, x + w, "y_min",
                              G_TYPE_FLOAT, y, "y_max", G_TYPE_FLOAT, y + h,
                              "label", G_TYPE_STRING, label.c_str(), "class_id", G_TYPE_INT, class_id, "confidence", G_TYPE_FLOAT, confidence, NULL);
        gst_video_region_of_interest_meta_add_param(meta, detection);
        if (nullptr != landmarks)
        {
            gst_video_region_of_interest_meta_add_param(meta, gst_structure_copy(landmarks));
        }
        if (nullptr != segmentation_data)
        {
            gst_video_region_of_interest_meta_add_param(meta, gst_structure_copy(segmentation_data));
        }
    }

    void add_detection(DetectionObject &det)
    {
        add_detection(det.xmin, det.ymin, det.width, det.height, det.label, det.class_id, det.confidence, det.landmarks, det.segmentation_data);
    }

    std::map<std::string, HailoTensorPtr> get_tensors_by_name()
    {
        std::map<std::string, HailoTensorPtr> tensors_by_name;
        auto tensors = get_tensors();
        for (auto tensor : tensors)
        {
            tensors_by_name[tensor->name] = tensor;
        }
        return tensors_by_name;
    }

    gboolean remove_all_tensors()
    {
        gpointer state = NULL;
        GstMeta *meta;
        std::vector<GstMeta *> meta_vector;
        gboolean ret = false;
        while ((meta = gst_buffer_iterate_meta_filtered(_frame->buffer, &state, GST_PARENT_BUFFER_META_API_TYPE)))
        {
            meta_vector.emplace_back(std::move(meta));
        }
        for (auto meta : meta_vector)
        {
            ret = gst_buffer_remove_meta(_frame->buffer, meta);
            if (ret == false)
                return ret;
        }
        return true;
    }

    std::vector<gint> get_offsets()
    {
        std::vector<gint> frame_offsets;
        frame_offsets.reserve(3);
        frame_offsets[0] = GST_VIDEO_FRAME_COMP_OFFSET(_frame, 0);
        frame_offsets[1] = GST_VIDEO_FRAME_COMP_OFFSET(_frame, 1);
        frame_offsets[2] = GST_VIDEO_FRAME_COMP_OFFSET(_frame, 2);
        return frame_offsets;
    }

private:
    GstVideoFrame *_frame;
};
using HailoFramePtr = std::shared_ptr<HailoFrame>;

#endif /* _HAILO_FRAME_HPP_ */
