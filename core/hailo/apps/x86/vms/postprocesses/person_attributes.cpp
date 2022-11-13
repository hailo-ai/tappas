/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include <vector>
#include "common/labels/peta.hpp"
#include "common/tensors.hpp"
#include "common/math.hpp"
#include "hailo_tracker.hpp"
#include "person_attributes.hpp"
#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"

#define RESNET_V1_18_PERSON_OUTPUT_LAYER_NAME "person_attr_resnet_v1_18/fc1"

std::string tracker_name="hailo_person_tracker";

void person_attributes_postprocess(HailoROIPtr roi, GstVideoFrame *frame, gchar *current_stream_id, std::string output_layer_name)
{
    if (!roi->has_tensors())
    {
        return;
    }

    // Extract the relevant output tensor.
    HailoTensorPtr outp_tensor = roi->get_tensor(output_layer_name);

    // Convert the tensor to xarray
    xt::xarray<float> attr_predictions = common::get_xtensor_float(outp_tensor);

    std::string label = "";
    int num_of_classes = attr_predictions.shape()[2];
    auto unique_ids = hailo_common::get_hailo_unique_id(roi);
    if (unique_ids.empty())
        return;

    std::string jde_tracker_name = tracker_name + "_" + current_stream_id;
    HailoTracker::GetInstance().remove_classifications_from_track(jde_tracker_name,
                                                                  unique_ids[0]->get_id(),
                                                                  std::string("person_attributes"));
    // Iterate over the attribute predictions
    for (int i = 0; i < num_of_classes; i++)
    {
        // Get the confidence
        float confidence = attr_predictions(0, 0, 0, i);
        // Filter only positive confidence values
        if (confidence > 0.0f)
        {
            // Get the label from the peta labels
            label = labels::peta[i];

            // Update the tracker with the found ocr
            HailoClassificationPtr classification;

            classification = std::make_shared<HailoClassification>(std::string("person_attributes"),
                                                                   i,
                                                                   label,
                                                                   0.99f);
            // We are updating the tracker with the results.
            // No need to add the object to the ROI because it is followed by fakesing - end of sub-pipeline.
            HailoTracker::GetInstance().add_object_to_track(jde_tracker_name,
                                                            unique_ids[0]->get_id(),
                                                            classification);
        }
    }
}

void filter(HailoROIPtr roi, GstVideoFrame *frame, gchar *current_stream_id)
{
    person_attributes_postprocess(roi, frame, current_stream_id, RESNET_V1_18_PERSON_OUTPUT_LAYER_NAME);
}

void person_attributes_rgba(HailoROIPtr roi, GstVideoFrame *frame, gchar *current_stream_id)
{
    person_attributes_postprocess(roi, frame, current_stream_id, "person_attr_resnet_v1_18_rgbx/fc1");
}