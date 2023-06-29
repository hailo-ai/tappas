/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#include <vector>

#include "xtensor/xarray.hpp"
#include "xtensor/xtensor.hpp"
#include "xtensor/xsort.hpp"
#include "common/tensors.hpp"
#include "common/math.hpp"
#include "ocr_postprocess.hpp"

#define MIN_SCORE_THRESHOLD (0.90) // Min score threshold
#define MIN_CHARS (6)              // Min number of characters

const char *OUTPUT_LAYER_NAME = "lprnet/conv31";
/**
 * @brief recognize the characters that are in the license plate
 *
 * @param roi holds the network output data
 */
void OCR_postprocess(HailoROIPtr roi)
{
    HailoTensorPtr net_output = roi->get_tensor(OUTPUT_LAYER_NAME);
    if (nullptr == net_output)
        return;

    xt::xarray<float> output_dequantize;
    output_dequantize = common::get_xtensor_float(net_output);

    xt::xarray<float> prebs = xt::mean(output_dequantize, 0);
    xt::xarray<int> preb_label = xt::argmax(prebs, 1);

    common::softmax_2D(prebs.data(), prebs.shape(0), prebs.shape(1));
    xt::xarray<float> conf_label = xt::amax(prebs, {1});

    std::vector<int> no_repeat_label_index;
    // this will hold the indices of the recognized chars, from the AVAILABLE_CHARS vector declared in the hpp file
    std::vector<float> no_repeat_label_conf;
    // this will hold the confidence of the recognized chars
    std::ostringstream no_repeat_label;
    // this will hold the the recognized chars, from the AVAILABLE_CHARS vector declared in the hpp file

    int pre_char_index = preb_label[0];
    if (int(pre_char_index) != int(AVAILABLE_CHARS.size()) - 1)
    // the index is not the index of the "-" (the "-" char means this is not a valid char)
    {
        no_repeat_label_index.push_back(pre_char_index);
        no_repeat_label << AVAILABLE_CHARS[pre_char_index];
        no_repeat_label_conf.push_back(conf_label[0]);
    }

    for (int i = 0; i < int(preb_label.size()); i++)
    // this loop removes non valid chars
    {
        if ((pre_char_index == int(preb_label[i])) || (int(preb_label[i]) == int(AVAILABLE_CHARS.size()) - 1))
        {
            if (int(preb_label[i]) == int(AVAILABLE_CHARS.size()) - 1)
            {
                pre_char_index = preb_label[i];
                // no push to final classification
            }
            continue;
        }
        no_repeat_label_index.push_back(preb_label[i]);
        no_repeat_label << AVAILABLE_CHARS[preb_label[i]];
        no_repeat_label_conf.push_back(conf_label[i]);

        pre_char_index = preb_label[i];
    }

    float conf_mean = std::accumulate(no_repeat_label_conf.begin(), no_repeat_label_conf.end(), 0.0) / no_repeat_label_conf.size();
    if (conf_mean >= MIN_SCORE_THRESHOLD && no_repeat_label.str().size() > MIN_CHARS)
    {
        hailo_common::add_classification(roi, std::string("ocr"), no_repeat_label.str(), conf_mean);
    }
}

void filter(HailoROIPtr roi)
{
    OCR_postprocess(roi);
}
