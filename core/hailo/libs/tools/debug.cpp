/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include <iostream>
#include <map>
#include <typeinfo>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <unistd.h>
#include <random>
#include "debug.hpp"

#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"
#include <xtensor/xnpy.hpp>

// Frame counter A
int frame_count_a = 0;
void frame_counter_a(HailoROIPtr roi)
{
    std::cout << "Frame Counter A: " << frame_count_a++ << std::endl;
}

// Frame counter B
int frame_count_b = 0;
void frame_counter_b(HailoROIPtr roi)
{
    std::cout << "Frame Counter B: " << frame_count_b++ << std::endl;
}

// Frame counter C
int frame_count_c = 0;
void frame_counter_c(HailoROIPtr roi)
{
    std::cout << "Frame Counter C: " << frame_count_c++ << std::endl;
}

// Frame counter D
int frame_count_d = 0;
void frame_counter_d(HailoROIPtr roi)
{
    std::cout << "Frame Counter D: " << frame_count_d++ << std::endl;
}

// Frame counter E
int frame_count_e = 0;
void frame_counter_e(HailoROIPtr roi)
{
    std::cout << "Frame Counter E: " << frame_count_e++ << std::endl;
}

// Print current time
void time_a(HailoROIPtr roi)
{
    auto timenow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::cout << "Timestamp A: " << timenow << std::endl;
}

// Print current time
void time_b(HailoROIPtr roi)
{

    auto timenow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::cout << "Timestamp B: " << timenow << std::endl;
}

// Print current time
void time_c(HailoROIPtr roi)
{

    auto timenow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::cout << "Timestamp C: " << timenow << std::endl;
}

// Print current time
void time_d(HailoROIPtr roi)
{

    auto timenow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::cout << "Timestamp D: " << timenow << std::endl;
}

// Print current time
void time_e(HailoROIPtr roi)
{
    auto timenow = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::cout << "Timestamp E: " << timenow << std::endl;
}

// Sleep for 10 seconds
void sleep10(HailoROIPtr roi)
{
    sleep(10);
}

// Generate a random float ranged from M to N
float get_random(float M = 0, float N = 1)
{
    // Change the random seed each time
    auto seed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    srand(static_cast <unsigned> (seed));
    if (M >= N)
        return M;
    float random_f = M + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(N-M)));
    return random_f;
}

// Generates a random (1-10) number of detections
// Each detection will have random x,y,w,h and confidence
void generate_random_detections(HailoROIPtr roi)
{
    // Get the bounding limits for the new boxes
    HailoBBox bbox_limit = roi->get_bbox();
    float xmin_limit = bbox_limit.xmin() * 1.001;
    float ymin_limit = bbox_limit.ymin() * 1.001;
    float xmax_limit = bbox_limit.xmax() * 0.999;
    float ymax_limit = bbox_limit.ymax() * 0.999;
    int num_detections_to_add = (int)get_random(1.0, 11.0); // Decide how many detections to add
    float r_confidence, r_xmin, r_ymin, r_xmax, r_ymax;
    bool added_bbox = false;
    for (int i = 0; i < num_detections_to_add; i++)
    {
        // We generate random bboxes until we have a valid box (width/height > 0)
        // Once we have a valid box, then it can be added to the roi
        added_bbox = false;
        while(!added_bbox)
        {
            // Generate random dimensions for the new detection
            r_confidence = CLAMP(static_cast <float> (rand()) / static_cast <float> (RAND_MAX), 0.0, 1.0);
            r_xmin = CLAMP(get_random(xmin_limit, xmax_limit), xmin_limit, xmax_limit);
            r_ymin = CLAMP(get_random(ymin_limit, ymax_limit), ymin_limit, ymax_limit);
            r_xmax = CLAMP(get_random(r_xmin, xmax_limit), r_xmin, xmax_limit);
            r_ymax = CLAMP(get_random(r_ymin, ymax_limit), r_ymin, ymax_limit);
            HailoBBox r_bbox = HailoBBox(r_xmin, r_ymin, r_xmax - r_xmin, r_ymax - r_ymin);
            // If this bbox has valid proportions, then add it, else try again
            if (r_bbox.width() > RANDOM_DIM_LIMIT && r_bbox.height() > RANDOM_DIM_LIMIT)
            {
                hailo_common::add_detection(roi, r_bbox, "debug_generated", r_confidence);
                added_bbox = true;
            }
        }
    }
}

// Generates a one detection in the center of the roi
// The detection will have 50% width/height of the roi, and random confidence
void generate_center_detection(HailoROIPtr roi)
{
    // Get the bounding limits for the new boxes
    HailoBBox bbox_limit = roi->get_bbox();
    float roi_width = CLAMP(bbox_limit.width(), 0, 1);
    float roi_height = CLAMP(bbox_limit.height(), 0, 1);
    float r_confidence, r_xmin, r_ymin;

    // We generate on box in the center of the roi with 50% width and height
    // Once we have a valid box, then it can be added to the roi
    r_confidence = CLAMP(static_cast <float> (rand()) / static_cast <float> (RAND_MAX), 0.0, 1.0);
    r_xmin = bbox_limit.xmin() + (roi_width / 4);
    r_ymin = bbox_limit.ymin() + (roi_height / 4);
    HailoBBox r_bbox = HailoBBox(r_xmin, r_ymin, roi_width / 2, roi_height / 2);

    // Add the detection to the roi
    hailo_common::add_detection(roi, r_bbox, "debug_generated", r_confidence);
}

// Generates a one detection in the bottom of the roi
// The detection will have 25% width/height of the roi, and random confidence
void generate_bottom_detection(HailoROIPtr roi)
{
    // Get the bounding limits for the new boxes
    HailoBBox bbox_limit = roi->get_bbox();
    float roi_width = CLAMP(bbox_limit.width(), 0, 1);
    float roi_height = CLAMP(bbox_limit.height(), 0, 1);
    float r_confidence, r_xmin, r_ymin;

    // We generate on box in the bottom of the roi with 25% width and height
    // Once we have a valid box, then it can be added to the roi
    r_confidence = CLAMP(static_cast <float> (rand()) / static_cast <float> (RAND_MAX), 0.0, 1.0);
    r_xmin = bbox_limit.xmin() + ((3 * roi_width) / 8);
    r_ymin = bbox_limit.ymin() + ((5 * roi_height) / 8);
    HailoBBox r_bbox = HailoBBox(r_xmin, r_ymin, roi_width / 4, roi_height / 4);

    // Add the detection to the roi
    hailo_common::add_detection(roi, r_bbox, "debug_generated", r_confidence);
}

void print_roi_bboxs(HailoROIPtr roi)
{
    for(HailoDetectionPtr &detection : hailo_common::get_hailo_detections(roi))
    {
        HailoBBox bbox = detection->get_bbox();

        std::cout << "-------" << std::endl;
        std::cout << "Confidence: " << detection->get_confidence() << " Label: " << detection->get_label() << " ClassID: " << detection->get_class_id() << std::endl;
        std::cout << "X: " << bbox.xmin() << " Y:" << bbox.ymin() <<  " Width:" << bbox.width() <<  " Height: " << bbox.height() << std::endl;
        std::cout << "-------" << std::endl;
    }
}

void dump_tensors_to_npy(HailoROIPtr roi)
{
    for (auto const& [name, tensor] : roi->get_tensors_by_name())
    {
	std::string output_name = name;
	std::replace( output_name.begin(), output_name.end(), '/', '_');

        xt::xarray<uint8_t> xtensor = xt::adapt(tensor->data(), tensor->size(), xt::no_ownership(), tensor->shape());    
        xt::dump_npy(output_name + ".npy", xtensor);
    }

}

// Do Nothing
void identity(HailoROIPtr roi)
{
    return;
}

// Default filter calls identity
void filter(HailoROIPtr roi)
{
    identity(roi);
}
