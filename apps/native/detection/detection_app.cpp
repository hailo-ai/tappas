/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
/**
 * @file detection_app_c_api.cpp
 * @brief This example demonstrates running inference with virtual streams using the Hailort's C API on yolov5m
 **/

#include "hailo/hailort.h"
#include "double_buffer.hpp"
#include "bitmap_utils.hpp"
#include "yolov5_post_processing.hpp"

#include <algorithm>
#include <future>
#include <stdio.h>
#include <stdlib.h>

#define INPUT_COUNT (1)
#define OUTPUT_COUNT (3)
#define INPUT_FILES_COUNT (10)
#define HEF_FILE ("yolov5m_wo_spp_60p.hef")

#define REQUIRE_ACTION(cond, action, label, ...)                \
    do {                                                        \
        if (!(cond)) {                                          \
            std::cout << (__VA_ARGS__) << std::endl;            \
            action;                                             \
            goto label;                                         \
        }                                                       \
    } while(0)

#define REQUIRE_SUCCESS(status, label, ...) REQUIRE_ACTION((HAILO_SUCCESS == (status)), , label, __VA_ARGS__)

// Colors' names were taken from: https://en.wikipedia.org/wiki/Lists_of_colors
static const std::pair<Color, std::string> g_colors[] = {
        {Color(0x00,0xFF,0x00), "Green"},
        {Color(0xFF,0x00,0x00), "Red"},
        {Color(0x00,0x00,0xFF), "Blue"},
        {Color(0x00,0xFF,0xFF), "Cyan"},
        {Color(0xA5,0x2A,0x2A), "Brown"},
        {Color(0xFF,0xD7,0x00), "Gold"},
        {Color(0x40,0x40,0x40), "Blue"},
        {Color(0xFF,0x00,0xFF), "Magenta"},
        {Color(0xFF,0xA5,0x00), "Orange"},
        {Color(0xFF,0xC0,0xCB), "Pink"},
        {Color(0x8B,0x00,0x00), "Dark Red"},
        {Color(0xAB,0x27,0x4F), "Amaranth purple"},
        {Color(0x3B,0x7A,0x57), "Amazon"},
        {Color(0xFF,0xBF,0x00), "Amber"},
        {Color(0x3D,0xDC,0x84), "Android green"},
        {Color(0xCD,0x95,0x75), "Antique brass"},
        {Color(0x66,0x5D,0x1E), "Antique bronze"},
        {Color(0x8D,0xB6,0x00), "Apple green"},
        {Color(0xFB,0xCE,0xB1), "Apricot"},
        {Color(0x8F,0x97,0x79), "Artichoke"},
        {Color(0xA2,0xA2,0xD0), "Blue bell"},
        {Color(0xD8,0x91,0xEF), "Bright lilac"},
        {Color(0xC3,0x21,0x48), "Bright maroon"},
        {Color(0xFF,0xAA,0x1D), "Bright yellow (Crayola)"},
        {Color(0x00,0x42,0x25), "British racing green"},
        {Color(0x88,0x54,0x0B), "Brown"},
        {Color(0x80,0x00,0x20), "Burgundy"},
        {Color(0xDE,0xB8,0x87), "Burlywood"},
        {Color(0x5F,0x9E,0xA0), "Cadet blue"},
        {Color(0xEF,0xBB,0xCC), "Cameo pink"},
        {Color(0x59,0x27,0x20), "Caput mortuum"},
        {Color(0xC4,0x1E,0x3A), "Cardinal"},
        {Color(0xAC,0xE1,0xAF), "Celadon"},
        {Color(0xF7,0xE7,0xCE), "Champagne"},
        {Color(0x36,0x45,0x4F), "Charcoal"},
        {Color(0x58,0x11,0x1A), "Chocolate Cosmos"},
        {Color(0xFF,0xA7,0x00), "Chrome yellow"},
        {Color(0x98,0x81,0x7B), "Cinereous"},
        {Color(0XE3,0X42,0X34), "Cinnabar"},
        {Color(0XE4,0XD0,0X0A), "Citrine"},
        {Color(0X7F,0X17,0X34), "Claret"},
        {Color(0XD2,0X69,0X1E), "Cocoa brown"},
        {Color(0X6F,0X4E,0X37), "Coffee"},
        {Color(0xB9,0xD9,0xEB), "Columbia Blue"},
        {Color(0x8C,0x92,0xAC), "Cool grey"},
        {Color(0xB8,0x73,0x33), "Cordovan"},
        {Color(0x81,0x61,0x3C), "Coyote brown"},
        {Color(0x01,0x32,0x20), "Dark green"},
        {Color(0x1A,0x24,0x21), "Dark jungle green"},
        {Color(0xBD,0xB7,0x6B), "Dark khaki"}};

class FeatureData {
public:
    FeatureData(uint32_t buffers_size, float32_t qp_zp, float32_t qp_scale, uint32_t width) :
    m_buffers(buffers_size), m_qp_zp(qp_zp), m_qp_scale(qp_scale), m_width(width)
    {}
    static bool sort_tensors_by_size (std::shared_ptr<FeatureData> i, std::shared_ptr<FeatureData> j) { return i->m_width < j->m_width; };

    DoubleBuffer m_buffers;
    float32_t m_qp_zp;
    float32_t m_qp_scale;
    uint32_t m_width;
};

hailo_status create_feature(hailo_output_vstream vstream, std::shared_ptr<FeatureData> &feature)
{
    hailo_vstream_info_t vstream_info = {};
    auto status = hailo_get_output_vstream_info(vstream, &vstream_info);
    if (HAILO_SUCCESS != status) {
        std::cerr << "Failed to get output vstream info with status = " << status << std::endl;
        return status;
    }

    size_t output_frame_size = 0;
    status = hailo_get_output_vstream_frame_size(vstream, &output_frame_size);
    if (HAILO_SUCCESS != status) {
        std::cerr << "Failed getting output virtual stream frame size with status = " << status << std::endl;
        return status;
    }

    feature = std::make_shared<FeatureData>(static_cast<uint32_t>(output_frame_size), vstream_info.quant_info.qp_zp,
        vstream_info.quant_info.qp_scale, vstream_info.shape.width);

    return HAILO_SUCCESS;
}

hailo_status dump_detected_object(const DetectionObject &detection, std::ofstream &detections_file, const std::string &color)
{
    if (detections_file.fail()) {
        return HAILO_FILE_OPERATION_FAILURE;
    }

    auto class_id_name = coco_eighty_classes.find(detection.class_id);
    if (coco_eighty_classes.end() == class_id_name) {
        std::cerr << "Failed to find class with id = " << detection.class_id << "\n";
        return HAILO_NOT_FOUND;
    }

    detections_file << "Detection object name:          " << class_id_name->second << "\n";
    detections_file << "Detection object color:         " << color << "\n";
    detections_file << "Detection object id:            " << detection.class_id << "\n";
    detections_file << "Detection object confidence:    " << detection.confidence << "\n";
    detections_file << "Detection object Xmax:          " << detection.xmax << "\n";
    detections_file << "Detection object Xmin:          " << detection.xmin << "\n";
    detections_file << "Detection object Ymax:          " << detection.ymax << "\n";
    detections_file << "Detection object Ymin:          " << detection.ymin << "\n" << std::endl;

    return HAILO_SUCCESS;
}

hailo_status draw(std::unique_ptr<BMPImage> &image, std::vector<DetectionObject> &detections)
{
    hailo_status status = HAILO_SUCCESS;
    if (detections.size() == 0) {
        std::cout << "No detections were found in file '" << image->name() << "'\n";
        return HAILO_SUCCESS;
    }

    auto detections_file = OUTPUT_DIR_PATH + image->name() + "_detections.txt";
    std::ofstream ofs(detections_file, std::ios::out);
    if (ofs.fail()) {
        std::cerr << "Failed opening output file: '" << detections_file << "'\n";  
        status = HAILO_OPEN_FILE_FAILURE;
    }
    
    uint32_t color_index = 0;
    for (auto &detection : detections) {
        if (0 == detection.confidence) {
            continue;
        }

        if ((detection.xmax >= YOLOV5M_IMAGE_WIDTH) ||
            (detection.ymax >= YOLOV5M_IMAGE_HEIGHT) ||
            (detection.xmin < 0) || (detection.ymin < 0)) {
            std::cerr << "Failed drawing detection object, the coordinates are not compatible to image size limits\n";  
            status = HAILO_INVALID_OPERATION;
            continue;
        }

        static_assert((MAX_BOXES == sizeof(g_colors)/sizeof(g_colors[0])), "The size of g_colors array must be MAX_BOXES");
        auto color_name_pair = g_colors[color_index];
        image->draw_border(static_cast<uint32_t>(detection.xmin), static_cast<uint32_t>(detection.xmax),
            static_cast<uint32_t>(detection.ymin), static_cast<uint32_t>(detection.ymax), color_name_pair.first);

        auto dump_status = dump_detected_object(detection, ofs, color_name_pair.second);
        if (HAILO_SUCCESS != dump_status) {
            status = dump_status;
        }
        color_index++;
    }

    return status;
}

hailo_status post_processing_all(std::vector<std::shared_ptr<FeatureData>> &features, const size_t frames_count,
    std::vector<std::unique_ptr<BMPImage>> &input_images)
{
    auto status = HAILO_SUCCESS;

    std::sort(features.begin(), features.end(), &FeatureData::sort_tensors_by_size);
    for (size_t i = 0; i < frames_count; i++) {
        auto detections = post_processing(
            features[0]->m_buffers.get_read_buffer().data(), features[0]->m_qp_zp, features[0]->m_qp_scale,
            features[1]->m_buffers.get_read_buffer().data(), features[1]->m_qp_zp, features[1]->m_qp_scale,
            features[2]->m_buffers.get_read_buffer().data(), features[2]->m_qp_zp, features[2]->m_qp_scale);
    
        for (auto &feature : features) {
            feature->m_buffers.release_read_buffer();
        }

        auto draw_status = draw(input_images[i], detections);
        if (HAILO_SUCCESS != draw_status) {
            std::cerr << "Failed drawing detecftions on image '" << input_images[i]->name() << "'. Got status "<< draw_status << "\n";
            status = draw_status;
        }

        auto dump_image_status = input_images[i]->dump_image();
        if (HAILO_SUCCESS != draw_status) {
            std::cerr << "Failed dumping image '" << input_images[i]->name() << "'. Got status "<< dump_image_status << "\n";
            status = dump_image_status;
        }
    }
    return status;
}

hailo_status write_all(hailo_input_vstream input_vstream, const std::vector<std::unique_ptr<BMPImage>> &input_images)
{
    for (auto &input_image : input_images) {
        const auto &input_data = input_image->get_data();
        hailo_status status = hailo_vstream_write_raw_buffer(input_vstream, input_data.data(), input_data.size());
        if (HAILO_SUCCESS != status) {
            std::cerr << "Failed writing to device data of image '" << input_image->name() << "'. Got status = " <<  status << std::endl;
            return status;
        }
    }
    return HAILO_SUCCESS;
}

hailo_status read_all(hailo_output_vstream output_vstream, const size_t frames_count, std::shared_ptr<FeatureData> feature)
{
    for (size_t i = 0; i < frames_count; i++) {
        auto &buffer = feature->m_buffers.get_write_buffer();
        hailo_status status = hailo_vstream_read_raw_buffer(output_vstream, buffer.data(), buffer.size());
        feature->m_buffers.release_write_buffer();

        if (HAILO_SUCCESS != status) {
            std::cerr << "Failed reading with status = " <<  status << std::endl;
            return status;
        }
    }
    return HAILO_SUCCESS;
}

hailo_status run_inference_threads(hailo_input_vstream input_vstream, hailo_output_vstream *output_vstreams,
    const size_t output_vstreams_size, std::vector<std::unique_ptr<BMPImage>> &input_images)
{
    // Create features data to be used for post-processing
    std::vector<std::shared_ptr<FeatureData>> features;
    features.reserve(output_vstreams_size);
    for (size_t i = 0; i < output_vstreams_size; i++) {
        std::shared_ptr<FeatureData> feature(nullptr);
        auto status = create_feature(output_vstreams[i], feature);
        if (HAILO_SUCCESS != status) {
            std::cerr << "Failed creating feature with status = " << status << std::endl;
            return status;
        }

        features.emplace_back(feature);
    }

    // Create read threads
    const size_t frames_count = input_images.size();
    std::vector<std::future<hailo_status>> output_threads;
    output_threads.reserve(output_vstreams_size);
    for (size_t i = 0; i < output_vstreams_size; i++) {
        output_threads.emplace_back(std::async(read_all, output_vstreams[i], frames_count, features[i]));
    }

    // Create write thread
    auto input_thread(std::async(write_all, input_vstream, std::ref(input_images)));

    // Create post-process thread
    auto pp_thread(std::async(post_processing_all, std::ref(features), frames_count, std::ref(input_images)));

    // End threads
    hailo_status out_status = HAILO_SUCCESS;
    for (size_t i = 0; i < output_threads.size(); i++) {
        auto status = output_threads[i].get();
        if (HAILO_SUCCESS != status) {
            out_status = status;
        }
    }
    auto input_status = input_thread.get();
    auto pp_status = pp_thread.get();

    if (HAILO_SUCCESS != input_status) {
        std::cerr << "Write thread failed with status " << input_status << std::endl;
        return input_status;
    }
    if (HAILO_SUCCESS != out_status) {
        std::cerr << "Read failed with status " << out_status << std::endl;
        return out_status;
    }
    if (HAILO_SUCCESS != pp_status) {
        std::cerr << "Post-processing failed with status " << pp_status << std::endl;
        return pp_status;
    }

    std::cout << "Inference finished successfully" << std::endl;

    return HAILO_SUCCESS;
}

hailo_status infer(std::vector<std::unique_ptr<BMPImage>> &input_images)
{
    hailo_status status = HAILO_UNINITIALIZED;
    hailo_device device = NULL;
    hailo_hef hef = NULL;
    hailo_configure_params_t config_params = {0};
    hailo_configured_network_group network_group = NULL;
    size_t network_group_size = 1;
    hailo_input_vstream_params_by_name_t input_vstream_params[INPUT_COUNT] = {0};
    hailo_output_vstream_params_by_name_t output_vstream_params[OUTPUT_COUNT] = {0};
    size_t input_vstreams_size = INPUT_COUNT;
    size_t output_vstreams_size = OUTPUT_COUNT;
    hailo_activated_network_group activated_network_group = NULL;
    hailo_input_vstream input_vstreams[INPUT_COUNT] = {NULL};
    hailo_output_vstream output_vstreams[OUTPUT_COUNT] = {NULL};

    status = hailo_create_pcie_device(NULL, &device);
    REQUIRE_SUCCESS(status, l_exit, "Failed to create pcie_device");

    status = hailo_create_hef_file(&hef, HEF_FILE);
    REQUIRE_SUCCESS(status, l_release_device, "Failed reading hef file");

    status = hailo_init_configure_params(hef, HAILO_STREAM_INTERFACE_PCIE, &config_params);
    REQUIRE_SUCCESS(status, l_release_hef, "Failed initializing configure parameters");

    status = hailo_configure_device(device, hef, &config_params, &network_group, &network_group_size);
    REQUIRE_SUCCESS(status, l_release_hef, "Failed configure devcie from hef");
    REQUIRE_ACTION(network_group_size == 1, status = HAILO_INVALID_ARGUMENT, l_release_hef, "Invalid network group size");

    status = hailo_make_input_vstream_params(network_group, true, HAILO_FORMAT_TYPE_AUTO,
        input_vstream_params, &input_vstreams_size);
    REQUIRE_SUCCESS(status, l_release_hef, "Failed making input virtual stream params");

    status = hailo_make_output_vstream_params(network_group, true, HAILO_FORMAT_TYPE_AUTO,
        output_vstream_params, &output_vstreams_size);
    REQUIRE_SUCCESS(status, l_release_hef, "Failed making output virtual stream params");

    REQUIRE_ACTION(((input_vstreams_size == INPUT_COUNT) || (output_vstreams_size == OUTPUT_COUNT)),
        status = HAILO_INVALID_OPERATION, l_release_hef, "Expected one input vstream and three outputs vstreams");

    status = hailo_create_input_vstreams(network_group, input_vstream_params, input_vstreams_size, input_vstreams);
    REQUIRE_SUCCESS(status, l_release_hef, "Failed creating input virtual streams");

    status = hailo_create_output_vstreams(network_group, output_vstream_params, output_vstreams_size, output_vstreams);
    REQUIRE_SUCCESS(status, l_release_input_vstream, "Failed creating output virtual streams");

    status = hailo_activate_network_group(network_group, NULL, &activated_network_group);
    REQUIRE_SUCCESS(status, l_release_output_vstream, "Failed activating network group");

    status = run_inference_threads(input_vstreams[0], output_vstreams, output_vstreams_size, input_images);
    REQUIRE_SUCCESS(status, l_deactivate_network_group, "Inference failure");

    status = HAILO_SUCCESS;
l_deactivate_network_group:
    (void)hailo_deactivate_network_group(activated_network_group);
l_release_output_vstream:
    (void)hailo_release_output_vstreams(output_vstreams, output_vstreams_size);
l_release_input_vstream:
    (void)hailo_release_input_vstreams(input_vstreams, input_vstreams_size);
l_release_hef:
    (void)hailo_release_hef(hef);
l_release_device:
    (void)hailo_release_device(device);
l_exit:
    return status;
}

int main()
{
    std::vector<std::unique_ptr<BMPImage>> input_images;
    input_images.reserve(INPUT_FILES_COUNT);
    auto status = BMPImage::get_images(input_images, INPUT_FILES_COUNT);
    if (HAILO_SUCCESS != status) {
        std::cerr << "get_images() failed to with status = " << status << std::endl;
        return status;
    }
    
    status = infer(input_images);
    if (HAILO_SUCCESS != status) {
        std::cerr << "Inference failed with status = " << status << std::endl;
        return status;
    }

    return HAILO_SUCCESS;
}
