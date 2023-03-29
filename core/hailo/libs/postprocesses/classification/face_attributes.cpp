/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include <vector>
#include "common/labels/celeb_a.hpp"
#include "common/tensors.hpp"
#include "common/math.hpp"
#include "face_attributes.hpp"
#include "hailo_tracker.hpp"
#include "xtensor/xadapt.hpp"
#include "xtensor/xarray.hpp"

#define RESNET_V1_18_FACE_OUTPUT_LAYER_NAME "face_attr_resnet_v1_18/fc3"
#define RESNET_V1_18_FACE_NUMBER_OF_CLASSES 40
#define RESNET_V1_18_FACE_THRESHOLD 0.3f

std::string tracker_name="hailo_face_tracker";

xt::xarray<float> get_face_attributes(HailoROIPtr roi, std::string output_layer_name)
{
    // Extract the relevant output tensor.
    HailoTensorPtr outp_tensor = roi->get_tensor(output_layer_name);

    // Convert the tensor to xarray
    xt::xarray<float> output_dequantize = common::get_xtensor_float(outp_tensor);

    // Reshape the output tensor to 40 classes view
    output_dequantize = xt::reshape_view(output_dequantize, {1, RESNET_V1_18_FACE_NUMBER_OF_CLASSES, 2});

    // Get the face attributes values by argmax
    xt::xarray<float>  attr_predictions = xt::argmax(output_dequantize, -1);
    return attr_predictions;
}

void add_attribute_prediction_to_roi(HailoROIPtr roi, std::vector<HailoUniqueIDPtr> unique_ids, std::string jde_tracker_name, std::string label, float confidence, int index)
{
    HailoClassificationPtr classification;

    if (confidence > RESNET_V1_18_FACE_THRESHOLD)
    {
        if (label != "No_Beard")
        {
            // Create the classification result
            classification = std::make_shared<HailoClassification>(std::string("face_attributes"),
                                                                   index,
                                                                   label,
                                                                   confidence);
        }
    }
    else
    {
        std::string new_label = "";
        if (label == "Male")
            new_label = "Female";
        else if(label == "No_Beard")
            new_label = "Beard";
        if (new_label != "")
        {
            // Create the classification result
            classification = std::make_shared<HailoClassification>(std::string("face_attributes"),
                                                                   index,
                                                                   new_label,
                                                                   0.99f);
        }
    }

    if (classification != nullptr)
    {
        if(unique_ids.empty())
        {
            hailo_common::add_object(roi, classification);
        }
        else
        {
            // Update the tracker with the results
            HailoTracker::GetInstance().add_object_to_track(jde_tracker_name,
                                                            unique_ids[0]->get_id(),
                                                            classification);
        }
    }
}

void face_attributes_postprocess(HailoROIPtr roi, std::string output_layer_name)
{
    if (!roi->has_tensors())
    {
        return;
    }

    xt::xarray<float> attr_predictions = get_face_attributes(roi, output_layer_name);

    std::string jde_tracker_name = tracker_name + "_" + roi->get_stream_id();
    std::vector<HailoUniqueIDPtr> unique_ids = hailo_common::get_hailo_unique_id(roi);
    if (!unique_ids.empty())
    {
        HailoTracker::GetInstance().remove_classifications_from_track(jde_tracker_name,
                                                                    unique_ids[0]->get_id(),
                                                                    std::string("face_attributes"));
    }

    std::string label = "";
    // Iterate over the attribute predictions
    for (int i = 0; i < RESNET_V1_18_FACE_NUMBER_OF_CLASSES; i++)
    {
        // Get the label from the celeb_a labels
        label = labels::celeb_a_filtered[i];
        if (label == "")
            continue;

        // Get the confidence
        float confidence = (attr_predictions(0, i)*0.99f);
        add_attribute_prediction_to_roi(roi, unique_ids, jde_tracker_name, label, confidence, i);
    }
}

void filter(HailoROIPtr roi)
{
    face_attributes_postprocess(roi, RESNET_V1_18_FACE_OUTPUT_LAYER_NAME);
}

void face_attributes_rgba(HailoROIPtr roi)
{
    face_attributes_postprocess(roi, "face_attr_resnet_v1_18_rgbx/fc3");
}
