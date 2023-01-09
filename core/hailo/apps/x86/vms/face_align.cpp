/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/

#include <gst/video/video.h>

// Hailo includes
#include "face_align.hpp"
#include "hailo_common.hpp"
#include "image.hpp"

// Open source includes
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

std::vector<float> DEST_VECTOR{38.2946f, 51.6963f,
                               73.5318f, 51.5014f,
                               56.0252f, 71.7366f,
                               41.5493f, 92.3655f,
                               70.7299f, 92.2041f};

cv::Mat DEST_MATRIX(5, 2, cv::DataType<float>::type, DEST_VECTOR.data());


/**
 * @brief Get the face landmarks from the ROI
 * Aling the points of the landmarks object to the given dimensions.
 *
 * @param height guint height to align to
 * @param width guint width to align to
 * @param roi the region of interest
 * @return cv::Mat contains the aligned landmarks
 */
cv::Mat get_landmarks(guint height, guint width, HailoROIPtr roi)
{
    cv::Mat landmarks(5, 2, cv::DataType<float>::type);
    auto landmarks_objects = roi->get_objects_typed(HAILO_LANDMARKS);

    if (landmarks_objects.empty())
    {
        GST_WARNING("There are no landmarks in buffer");
    }
    else if (landmarks_objects.size() > 1)
    {
        GST_WARNING("Too many landmarks");
    }
    else
    {
        HailoLandmarksPtr landmarks_obj = std::dynamic_pointer_cast<HailoLandmarks>(landmarks_objects[0]);
        std::vector<HailoPoint> points = landmarks_obj->get_points();

        for (size_t i = 0; i < points.size(); i++)
        {
            float x = points[i].x() * width;
            float y = points[i].y() * height;

            landmarks.at<float>(i, 0) = x;
            landmarks.at<float>(i, 1) = y;
        }
    }
    return landmarks;
}

void filter(HailoROIPtr roi, GstVideoFrame *frame, gchar *current_stream_id)
{
    // Get the CV::Mat from the GstVideoFrame
    cv::Mat image = get_mat_from_gst_frame(frame);

    // Get the landmarks from the ROI
    auto landmarks = get_landmarks(image.rows, image.cols, roi);

    // Perform similarity transform between the landmarks and the destination matrix
    auto warp_mat = FacePreprocess::similarTransform(landmarks, DEST_MATRIX);
    warp_mat = warp_mat.rowRange(0, 2);

    // Warp the image by Affine transformation
    cv::warpAffine(image, image, warp_mat, image.size());
}