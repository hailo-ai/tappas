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
#define RESNET_V1_18_PERSON_THRESHOLD 0.7f

std::string tracker_name = "hailo_person_tracker";

xt::xarray<float> get_attr_predictions_from_tensor(HailoTensorPtr outp_tensor)
{
    // Convert the tensor to xarray
    xt::xarray<float> xscores = common::get_xtensor_float(outp_tensor);
    auto attr_predictions = xt::view(xscores, 0, 0, xt::all());

    // Calculate the face attributes values by sigmoid
    common::sigmoid(attr_predictions.data(), attr_predictions.size());
    return attr_predictions;
}

void person_attributes_postprocess(HailoROIPtr roi, std::string output_layer_name)
{
    if (!roi->has_tensors())
    {
        return;
    }

    // Extract the relevant output tensor.
    HailoTensorPtr outp_tensor = roi->get_tensor(output_layer_name);
    auto attr_predictions = get_attr_predictions_from_tensor(outp_tensor);

    std::string label = "";
    std::string jde_tracker_name = tracker_name + "_" + roi->get_stream_id();
    auto unique_ids = hailo_common::get_hailo_unique_id(roi);
    if (unique_ids.size() == 1)
    {
        HailoTracker::GetInstance().remove_classifications_from_track(jde_tracker_name,
                                                                      unique_ids[0]->get_id(),
                                                                      std::string("person_attributes"));
    }

    uint num_of_attributes = attr_predictions.shape()[0];
    // Iterate over the attribute predictions
    for (uint i = 0; i < num_of_attributes; i++)
    {
        // Get the confidence
        float confidence = attr_predictions(i);
        // Get the label from the peta labels
        label = labels::peta_filtered[i];

        // Filter confidence values by threshold
        HailoClassificationPtr classification;
        if (label != "" && confidence > RESNET_V1_18_PERSON_THRESHOLD)
        {
            classification = std::make_shared<HailoClassification>(std::string("person_attributes"),
                                                                   i,
                                                                   label,
                                                                   0.99f);
        }
        else if(label == "Male")
        {
            classification = std::make_shared<HailoClassification>(std::string("person_attributes"),
                                                        i,
                                                        "Female",
                                                        0.99f);
        }

        if (!classification)
            continue;

        if (unique_ids.empty())
        {
            hailo_common::add_object(roi, classification);
        }
        else if(unique_ids.size() == 1)
        {
            // We are updating the tracker with the results.
            // No need to add the object to the ROI because it is followed by fakesing - end of sub-pipeline.
            HailoTracker::GetInstance().add_object_to_track(jde_tracker_name,
                                                            unique_ids[0]->get_id(),
                                                            classification);
        }
    }
}

void filter(HailoROIPtr roi)
{
    person_attributes_postprocess(roi, RESNET_V1_18_PERSON_OUTPUT_LAYER_NAME);
}

void person_attributes_nv12(HailoROIPtr roi)
{
    person_attributes_postprocess(roi, RESNET_V1_18_PERSON_OUTPUT_LAYER_NAME);
}

void person_attributes_rgba(HailoROIPtr roi)
{
    person_attributes_postprocess(roi, "person_attr_resnet_v1_18_rgbx/fc1");
}