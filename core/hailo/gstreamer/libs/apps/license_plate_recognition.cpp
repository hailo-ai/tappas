/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
// General cpp includes
#include <chrono>
#include <condition_variable>
#include <cxxopts.hpp>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/app/gstappsink.h>
#include <iostream>
#include <mutex>
#include <chrono>
#include <ctime>
#include <shared_mutex>
#include <stdio.h>
#include <thread>
#include <unistd.h>

// Tappas includes
#include "hailo_objects.hpp"
#include "hailo_common.hpp"
#include "metadata/gst_hailo_meta.hpp"

// Open source includes
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

//******************************************************************
// GLOBALS
//******************************************************************
#define OCR_SCORE_THRESHOLD (0.90)     // OCR score threshold
gboolean PRINT_FPS = false;            // When true, fps signals are emited and printed
std::condition_variable_any condvar_sync;  // Conditional variable for sync_mutex
std::condition_variable_any condvar_state; // Conditional variable for change_state_mutex
bool change_state_flag = true;         // When false - all hailonets are blocked for state change
bool sync_flag = true;                 // When true - context 0 can operate and context 1 is blocked

std::shared_mutex sync_mutex;          // Mutex to control what context should be active
std::shared_mutex change_state_mutex;  // Mutex to block hailonets when making a context switch

// Frame pad counting to protect network switching
volatile int sink_h0_frame_id = 0;
volatile int src_h0_frame_id = 0;
volatile int sink_h1_frame_id = 0;
volatile int src_h1_frame_id = 0;
volatile int sink_h2_frame_id = 0;
volatile int src_h2_frame_id = 0;

#define FILE_LIMIT (5)     // OCR score threshold
volatile int file_suffix = 0;
std::vector<int> seen_ocr_track_ids;

// Pipeline Macros
const gchar *TAPPAS_WORKSPACE = "";                 // Tappas workspace, retrieved at runtime
const gchar *CONTEXT_0_DETECTION_NET = "v_detection_net";  // Vehicle Detection hailonet name
const gchar *CONTEXT_1_DETECTION_NET = "lp_detection_net"; // License Plate Detection hailonet name
const gchar *CONTEXT_1_OCR_NET = "ocr_net";                // License Plate OCR hailonet name

const gchar *PIPELINE_SRC = "pipeline_src";
const gchar *CONTEXT_1_APP_SINK = "context_1_app_sink";
const gchar *CONTEXT_0_DISPLAY = "hailo_display";
const gchar *OCR_LABEL_TYPE = "ocr";
const std::string CONTEXT_0_T = "context_0_t";
const std::string CONTEXT_0_T_SRC = CONTEXT_0_T + ".";

// Basic Directories
const std::string X86_DIR = "/apps/gstreamer/x86";
const std::string POSTPROCESS_DIR = X86_DIR + "/libs/";
const std::string CROPPING_ALGORITHMS_DIR = POSTPROCESS_DIR + "/cropping_algorithms/";
const std::string RESOURCES_DIR = X86_DIR + "/license_plate_recognition/resources/";
const std::string DEFAULT_VIDEO_SRC = RESOURCES_DIR + "lpr_ayalon_loop.mp4";

// Vehicle Detection Macros
const std::string VEHICLE_DETECTION_HEF = RESOURCES_DIR + "yolov5m_vehicles_79FPS.hef";
const std::string VEHICLE_DETECTION_POST = POSTPROCESS_DIR + "libnew_yolo_post.so";
const std::string VEHICLE_DETECTION_POST_FUNC = "yolov5_vehicles_only";

// License Plate Detection and OCR Macros
// HEF and net names
const std::string LICENSE_PLATE_HEF = RESOURCES_DIR + "joined_tiny_yolov4_license_plates_lprnet_200fps.hef";
const std::string LICENSE_PLATE_DETECTION_NET_NAME = "joined_tiny_yolov4_license_plates_lprnet/tiny_yolov4_license_plates";
const std::string LICENSE_PLATE_OCR_NET_NAME = "joined_tiny_yolov4_license_plates_lprnet/lprnet";
// Post-process .so's and function names
const std::string LICENSE_PLATE_DETECTION_POST = POSTPROCESS_DIR + "libnew_yolo_post.so";
const std::string LICENSE_PLATE_DETECTION_POST_FUNC = "tiny_yolov4_license_plates";
const std::string LICENSE_PLATE_OCR_POST = POSTPROCESS_DIR + "libocr_post.so";
// Cropping algorithm .so and function names
const std::string LICENSE_PLATE_CROP_SO = CROPPING_ALGORITHMS_DIR + "liblpr_croppers.so";
const std::string LICENSE_PLATE_DETECTION_CROP_FUNC = "vehicles_without_ocr";
const std::string LICENSE_PLATE_OCR_CROP_FUNC = "license_plate_quality_estimation";

// License Plate Drawing
const std::string LPR_OVERLAY = POSTPROCESS_DIR + "liblpr_overlay.so";



//******************************************************************
// NETWORK SWITCH UTILITIES
//******************************************************************
static GstPadProbeReturn cb_hailonet_0_sink(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    std::shared_lock<std::shared_mutex> state_lock(change_state_mutex);
    condvar_state.wait(state_lock, []
                        { return change_state_flag; });

    std::unique_lock<std::shared_mutex> ulock(sync_mutex);
    condvar_sync.wait(ulock, []
                    { return sync_flag; });

    sink_h0_frame_id++;
    return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn cb_hailonet_1_sink(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    std::shared_lock<std::shared_mutex> state_lock(change_state_mutex);
    condvar_state.wait(state_lock, []
                        { return change_state_flag; });

    std::shared_lock<std::shared_mutex> shared_lock(sync_mutex);
    condvar_sync.wait(shared_lock, []
                    { return !sync_flag; });

    sink_h1_frame_id++;
    return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn cb_hailonet_2_sink(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    std::shared_lock<std::shared_mutex> state_lock(change_state_mutex);
    condvar_state.wait(state_lock, []
                        { return change_state_flag; });

    std::shared_lock<std::shared_mutex> shared_lock(sync_mutex);
    condvar_sync.wait(shared_lock, []
                        { return !sync_flag; });

    sink_h2_frame_id++;
    return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn src_counter_h0(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    src_h0_frame_id++;
    return GST_PAD_PROBE_OK;
}
static GstPadProbeReturn src_counter_h1(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    src_h1_frame_id++;
    return GST_PAD_PROBE_OK;
}
static GstPadProbeReturn src_counter_h2(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    src_h2_frame_id++;
    return GST_PAD_PROBE_OK;
}

static void overrun_cb(GstElement *queue, gpointer user_data)
{
    int c0_is_active;
    int c1_is_active;
    GstElement *pipeline = GST_ELEMENT(user_data);
    GstElement *c0_detection_net = gst_bin_get_by_name(GST_BIN(pipeline), CONTEXT_0_DETECTION_NET);
    GstElement *c1_detection_net = gst_bin_get_by_name(GST_BIN(pipeline), CONTEXT_1_DETECTION_NET);
    GstElement *c1_ocr_net = gst_bin_get_by_name(GST_BIN(pipeline), CONTEXT_1_OCR_NET);
    g_object_get(G_OBJECT(c0_detection_net), "is-active", &c0_is_active, NULL);
    g_object_get(G_OBJECT(c1_detection_net), "is-active", &c1_is_active, NULL);
    if (!c0_is_active && c1_is_active)
    {
        return;
    }

    change_state_flag = false; // Block all hailonets
    
    // Context 0 is now blocked, so flush it
    g_signal_emit_by_name(c0_detection_net, "flush");
    while (sink_h0_frame_id - src_h0_frame_id > 0 )
       ;

    // Now all hailonets are cleared and blocked so it is safe to make the switch.
    // Assume context 0 net is active and context 1 nets are inactive
    g_object_set(c0_detection_net, "is-active", false, NULL);
    g_object_set(c1_detection_net, "is-active", true, NULL);
    g_object_set(c1_ocr_net, "is-active", true, NULL);

    sync_flag = false;

    change_state_flag = true; // Unblock all hailonets

    condvar_state.notify_all();
    condvar_sync.notify_all();
}

static void underrun_cb(GstElement *queue, gpointer user_data)
{
    int c0_is_active;
    int c1_is_active;
    GstElement *pipeline = GST_ELEMENT(user_data);
    GstElement *c0_detection_net = gst_bin_get_by_name(GST_BIN(pipeline), CONTEXT_0_DETECTION_NET);
    GstElement *c1_detection_net = gst_bin_get_by_name(GST_BIN(pipeline), CONTEXT_1_DETECTION_NET);
    GstElement *c1_ocr_net = gst_bin_get_by_name(GST_BIN(pipeline), CONTEXT_1_OCR_NET);
    g_object_get(G_OBJECT(c0_detection_net), "is-active", &c0_is_active, NULL);
    g_object_get(G_OBJECT(c1_detection_net), "is-active", &c1_is_active, NULL);
    if (c0_is_active && !c1_is_active)
    {
        return;
    }

    change_state_flag = false; // Block all hailonets

    // Context 1 hailonets are now blocked, so flush them
    g_signal_emit_by_name(c1_detection_net, "flush");
    g_signal_emit_by_name(c1_ocr_net, "flush");
    while (sink_h1_frame_id - src_h1_frame_id > 0 && sink_h2_frame_id - src_h2_frame_id > 0)
       ;

    // Now all hailonets are cleared and blocked so it is safe to make the switch.
    // Assume context 1 nets are active and context 0 net is inactive
    g_object_set(c1_detection_net, "is-active", false, NULL);
    g_object_set(c1_ocr_net, "is-active", false, NULL);
    g_object_set(c0_detection_net, "is-active", true, NULL);

    sync_flag = true;  

    change_state_flag = true; // Unblock all hailonets

    condvar_state.notify_all();
    condvar_sync.notify_all();
}

//******************************************************************
// PIPELINE CREATION
//******************************************************************
/**
 * @brief Create the pipeline string object
 *
 * @param input_src  -  std::string
 *        A video file path or usb camera name (/dev/video*)
 *
 * @return std::string
 *         The full pipeline string.
 */
std::string create_pipeline_string(std::string input_src)
{
    std::string src_pipeline_string = "";
    std::string vehicle_detection_subpipeline = "";
    std::string license_plate_detection_subpipeline = "";
    std::string internal_offset = "";

    // Source sub-pipeline
    if (input_src.rfind("/dev/video", 0) == 0)
    {
        src_pipeline_string = "v4l2src name="+std::string(PIPELINE_SRC)+" device=" + input_src + " name=src_0 ! videoflip video-direction=horiz";
        internal_offset = "false";
    } else {
        src_pipeline_string = "multifilesrc loop=true name="+std::string(PIPELINE_SRC)+" location=" + input_src + " name=src_0 ! decodebin qos=false";
        internal_offset = "true";
    }
    src_pipeline_string += " ! videoscale ! video/x-raw,pixel-aspect-ratio=1/1 ! videoconvert qos=false";

    // Vehicle Detection sub-pipeline
    vehicle_detection_subpipeline = src_pipeline_string + " ! "
                                    "queue name=q0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! "
                                    "hailonet name="+CONTEXT_0_DETECTION_NET+" hef-path=" + TAPPAS_WORKSPACE + VEHICLE_DETECTION_HEF + " device-count=1 vdevice-key=1 debug=False is-active=true qos=false ! "
                                    "queue name=q1 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! "
                                    "hailofilter2 so-path=" + TAPPAS_WORKSPACE + VEHICLE_DETECTION_POST + " function-name=" + VEHICLE_DETECTION_POST_FUNC + "  qos=false ! "
                                    "queue name=q2 name=q1leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! "
                                    "hailotracker name=hailo_tracker  kalman-dist-thr=.5 iou-thr=.6 keep-tracked-frames=2 keep-lost-frames=2 ! "
                                    "queue name=q3 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! "
                                    "tee name="+CONTEXT_0_T+" "+
                                    CONTEXT_0_T_SRC + " ! "
                                        "queue name=q4 leaky=no max-size-buffers=50 min-threshold-buffers=0 max-size-bytes=0 max-size-time=0 ! "
                                        "hailooverlay line-thickness=3 font-thickness=1 qos=false ! "
                                        "hailofilter so-path=" + TAPPAS_WORKSPACE + LPR_OVERLAY + " qos=false ! "
                                        "videoconvert ! "
                                        "fpsdisplaysink video-sink=xvimagesink name="+CONTEXT_0_DISPLAY+" sync=true text-overlay=false signal-fps-measurements=true";

    // License Plate Recognition sub-pipeline
    license_plate_detection_subpipeline =  " " + CONTEXT_0_T_SRC + " ! "
                                           "queue name=q5 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! "
                                           "hailocropper drop-uncropped-buffers=true internal-offset="+internal_offset+" so-path=" + TAPPAS_WORKSPACE + LICENSE_PLATE_CROP_SO + " function-name=" + LICENSE_PLATE_DETECTION_CROP_FUNC + " name=cropper1 "
                                           "hailoaggregator name=agg1 flatten-detections=false "
                                           "cropper1. ! "
                                                "queue name=lp_detection_bypass_q leaky=no max-size-buffers=50 max-size-bytes=0 max-size-time=0 ! "
                                           "agg1. "
                                           "cropper1. ! "
                                                "queue name=q7 leaky=no max-size-buffers=50 max-size-bytes=0 max-size-time=0 ! "
                                                "queue name=license_plates_q leaky=no max-size-buffers=5 max-size-bytes=0 max-size-time=0 silent=false ! "
                                                "hailonet name="+CONTEXT_1_DETECTION_NET+" hef-path=" + TAPPAS_WORKSPACE + LICENSE_PLATE_HEF + " net-name=" + LICENSE_PLATE_DETECTION_NET_NAME + " device-count=1 vdevice-key=1 debug=False is-active=false qos=false ! "
                                                "queue name=lp_detection_q1 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! "
                                                "hailofilter2 so-path=" + TAPPAS_WORKSPACE + LICENSE_PLATE_DETECTION_POST + " function-name=" + LICENSE_PLATE_DETECTION_POST_FUNC + "  qos=false ! "
                                                "queue name=lp_detection_q2 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! "
                                           "agg1. "
                                           "agg1. ! queue name=q8 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! "
                                           "hailocropper drop-uncropped-buffers=true method=bilinear internal-offset="+internal_offset+" so-path=" + TAPPAS_WORKSPACE + LICENSE_PLATE_CROP_SO + " function-name=" + LICENSE_PLATE_OCR_CROP_FUNC + "  name=cropper2 "
                                           "hailoaggregator name=agg2 flatten-detections=false "
                                           "cropper2. ! "
                                                "queue name=lp_ocr_bypass_q leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! "
                                           "agg2. "
                                           "cropper2. ! "
                                                "queue name=lp_ocr_q0 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! "
                                                "hailonet name="+CONTEXT_1_OCR_NET+" hef-path=" + TAPPAS_WORKSPACE + LICENSE_PLATE_HEF + " net-name=" + LICENSE_PLATE_OCR_NET_NAME + " device-count=1 vdevice-key=1 debug=False is-active=false qos=false ! "
                                                "queue name=lp_ocr_q1 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! "
                                                "hailofilter2 so-path=" + TAPPAS_WORKSPACE + LICENSE_PLATE_OCR_POST + " qos=false ! "
                                                "queue name=lp_ocr_q2 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! "
                                           "agg2. "
                                           "agg2. ! queue  name=q9 leaky=no max-size-buffers=30 max-size-bytes=0 max-size-time=0 ! "
                                           "appsink sync=false qos=false emit-signals=true name=" + CONTEXT_1_APP_SINK;

    std::cout << "Pipeline:" <<std::endl;
    std::cout << "gst-launch-1.0 " << vehicle_detection_subpipeline + license_plate_detection_subpipeline << std::endl;
    // Combine and return the pipeline:
    return (vehicle_detection_subpipeline + license_plate_detection_subpipeline);
}

//******************************************************************
// PIPELINE UTILITIES
//******************************************************************
/**
 * @brief callback of new fps measurement signal
 *
 * @param fpsdisplaysink the element who sent the signal
 * @param fps the fps measured
 * @param droprate drop rate measured
 * @param avgfps average fps measured
 * @param udata extra data from the user
 */
static void fps_measurements_callback(GstElement *fpsdisplaysink,
                                      gdouble fps,
                                      gdouble droprate,
                                      gdouble avgfps,
                                      gpointer udata)
{
    gchar *name;
    g_object_get(G_OBJECT(fpsdisplaysink), "name", &name, NULL);

    std::cout << name << ", FPS: " << fps << " AVG_FPS: " << avgfps << " DROP RATE: " << droprate << std::endl;
}

size_t get_size(GstCaps *caps)
{
    size_t size;
    GstVideoInfo *info = gst_video_info_new();
    gst_video_info_from_caps(info, caps);
    size = info->size;
    gst_video_info_free(info);
    return size;
}

cv::Mat get_image(GstBuffer *buffer, GstCaps *caps, GstMapFlags flags)
{
    GstMapInfo map;
    GstVideoInfo *info = gst_video_info_new();
    cv::Mat img;
    gst_buffer_map(buffer, &map, flags);
    gst_video_info_from_caps(info, caps);
    switch (info->finfo->format)
    {
    case GST_VIDEO_FORMAT_YUY2:
        img = cv::Mat(info->height, info->width / 2, CV_8UC4, (char *)map.data, info->stride[0]);

        break;
    case GST_VIDEO_FORMAT_RGB:
        img = cv::Mat(info->height, info->width, CV_8UC3, (char *)map.data, info->stride[0]);
        break;
    default:
        break;
    }
    gst_buffer_unmap(buffer, &map);
    gst_video_info_free(info);
    return img;
}

static GstFlowReturn context_1_ocr_results_callback(GstAppSink * sink, gpointer udata)
{
    GstPad *tracker_src_pad = (GstPad *)udata;
    GstSample *sample;  // The new sample available in the appsink
    GstBuffer *buffer;  // The buffer stored in that sample
    HailoROIPtr roi;    // The ROI stored in that buffer 
    std::vector<NewHailoDetectionPtr> vehicle_detections; // The vehicle detections in that ROI
    std::vector<NewHailoDetectionPtr> lp_detections;      // The license plate detections in that ROI
    std::vector<HailoUniqueIDPtr>       unique_ids;       // The unique ids of those detections
    std::vector<HailoClassificationPtr> classifications;  // The classifications of those detections
    float confidence;                                     // The confidence of those classifications
    std::string license_plate_ocr_label;                  // The labels of those classifications

    // Get the sample from the appsink
    sample = gst_app_sink_pull_sample(sink);
    // If no sample was available (premature signal) then return, we will get it on the next pass
    if (nullptr == sample)
        return GST_FLOW_OK;

    buffer = gst_sample_get_buffer(sample);    // Extract the buffer from the sample
    buffer = gst_buffer_make_writable(buffer); // Get a read/writeable copy
    roi = get_hailo_main_roi(buffer, false);    // Extract the roi from the buffer 
    if (nullptr == roi)
    {
        gst_buffer_unref(buffer);
        return GST_FLOW_OK;
    }

    // For each roi, check the detections
    vehicle_detections = hailo_common::get_hailo_detections(roi);
    for(NewHailoDetectionPtr &vehicle_detection : vehicle_detections)
    {
        // Get the unique id of the detection
        unique_ids = hailo_common::get_hailo_unique_id(vehicle_detection);
        if (unique_ids.empty())
        {
            std::string label = vehicle_detection->get_label();
            gst_buffer_unref(buffer);
            return GST_FLOW_OK;
        }
        // For each vehicle, get the license plate detection
        lp_detections = hailo_common::get_hailo_detections(vehicle_detection);
        for(NewHailoDetectionPtr &lp_detection : lp_detections)
        {
            HailoBBox license_plate_box = hailo_common::create_flattened_bbox(lp_detection->get_bbox(), lp_detection->get_scaling_bbox());
            // For each license plate detection, check the classifications
            classifications = hailo_common::get_hailo_classifications(lp_detection);
            if (classifications.size() != 1)
                continue;
            HailoClassificationPtr classification = classifications[0];
            if (OCR_LABEL_TYPE == classification->get_classification_type())
            {
                confidence = classification->get_confidence(); 
                license_plate_ocr_label = classification->get_label();
                if (confidence >= OCR_SCORE_THRESHOLD && license_plate_ocr_label.size() > 6)
                {
                    GstCaps *caps;
                    cv::Mat raw_mat;
                    cv::Mat rgb_image;
                    cv::Mat cropped_image;
                    cv::Mat resized_image;
                    cv::Mat padded_image;
                    // GstBuffer *buffer_copy = gst_buffer_copy_deep(buffer);
                    GstEvent *ocr_event = gst_event_new_custom(GST_EVENT_CUSTOM_UPSTREAM,
                                                                gst_structure_new("OCR_LABEL",
                                                                                    "track_id", G_TYPE_INT, unique_ids[0]->get_id(),
                                                                                    "label", G_TYPE_STRING, license_plate_ocr_label.c_str(),
                                                                                    "confidence", G_TYPE_FLOAT, confidence, NULL));
                    gst_pad_send_event(tracker_src_pad, ocr_event); // Send the ocr label to the tracker
                    if (std::find(seen_ocr_track_ids.begin(), seen_ocr_track_ids.end(), unique_ids[0]->get_id()) != seen_ocr_track_ids.end())
                    {
                        // this track id was already updated
                        continue;
                    } else {
                        seen_ocr_track_ids.emplace_back(unique_ids[0]->get_id());
                    }
                    caps = gst_sample_get_caps(sample);
                    raw_mat = get_image(buffer, caps, GST_MAP_READWRITE);
                    cv::cvtColor(raw_mat, rgb_image, cv::COLOR_BGR2RGB);

                    cv::Rect rect;
                    rect.x = CLAMP(license_plate_box.xmin() * rgb_image.cols, 0, rgb_image.cols);
                    rect.y = CLAMP(license_plate_box.ymin() * rgb_image.rows, 0, rgb_image.rows);
                    rect.width  = CLAMP(license_plate_box.width()  * rgb_image.cols, 0, rgb_image.cols - rect.x);
                    rect.height = CLAMP(license_plate_box.height() * rgb_image.rows, 0, rgb_image.rows - rect.y);
                    cropped_image = rgb_image(rect);
                    cv::resize(cropped_image, resized_image, cv::Size(300, 75), cv::INTER_AREA);

                    // Add padding top, bottom, left, right, borderType
                    static const cv::Scalar color(255, 255, 255); // White color
                    cv::copyMakeBorder(resized_image, padded_image, 30, 0, 0, 0, cv::BORDER_CONSTANT, color);

                    auto text_position = cv::Point(10, 25);
                    std::string text = license_plate_ocr_label + " " + std::to_string((int)(confidence * 100)) + "%";
                    cv::putText(padded_image, text, text_position, cv::FONT_HERSHEY_SIMPLEX, 1,
                                cv::Scalar(0, 0, 255), 2);

                    std::string filename = std::string(TAPPAS_WORKSPACE) + "/lp_" + std::to_string(file_suffix % FILE_LIMIT) + ".jpg";
                    cv::imwrite(filename, padded_image);
                    file_suffix++;
                    raw_mat.release();
                    rgb_image.release();
                    cropped_image.release();
                    resized_image.release();
                    padded_image.release();
                }
            }
        }
    }

    gst_buffer_unref(buffer);
    return GST_FLOW_OK;
}

/**
 * @brief Extract the elements from the pipeline. From those extract the pads, 
 *        then set the probe callbacks and signal callbacks on those elements.
 *
 * @param pipeline  -  GstElement*
 *        The pipeline to set probes for.
 *
 * @param print_fps  -  gboolean
 *        If true, then print fps.
 */
void set_probe_callbacks(GstElement *pipeline, gboolean print_fps)
{
    // Extract hailonet elements from the pipeline
    GstElement *context_0_detection = gst_bin_get_by_name(GST_BIN(pipeline), CONTEXT_0_DETECTION_NET);
    GstElement *context_1_detection = gst_bin_get_by_name(GST_BIN(pipeline), CONTEXT_1_DETECTION_NET);
    GstElement *context_1_ocr = gst_bin_get_by_name(GST_BIN(pipeline), CONTEXT_1_OCR_NET);
    GstElement *license_plates_q = gst_bin_get_by_name(GST_BIN(pipeline), "license_plates_q");

    // Connect the queue signals
    g_signal_connect(license_plates_q, "overrun", G_CALLBACK(overrun_cb), pipeline);
    g_signal_connect(license_plates_q, "underrun", G_CALLBACK(underrun_cb), pipeline);

    // Extract pads from the hailonets
    GstPad *pad_c0_detection = gst_element_get_static_pad(context_0_detection, "sink");
    GstPad *pad_c1_detection = gst_element_get_static_pad(context_1_detection, "sink");
    GstPad *pad_c1_ocr = gst_element_get_static_pad(context_1_ocr, "sink");

    // Attach the callbacks to the probes
    gst_pad_add_probe(pad_c0_detection, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)cb_hailonet_0_sink, pipeline, NULL);
    gst_pad_add_probe(pad_c1_detection, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)cb_hailonet_1_sink, pipeline, NULL);
    gst_pad_add_probe(pad_c1_ocr, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)cb_hailonet_2_sink, pipeline, NULL);

    // hailonet src pad frame counting
    GstPad *src_pad_c0_detection = gst_element_get_static_pad(context_0_detection, "src");
    GstPad *src_pad_c1_detection = gst_element_get_static_pad(context_1_detection, "src");
    GstPad *src_pad_c1_ocr = gst_element_get_static_pad(context_1_ocr, "src");
    gst_pad_add_probe(src_pad_c0_detection, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)src_counter_h0, pipeline, NULL);
    gst_pad_add_probe(src_pad_c1_detection, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)src_counter_h1, pipeline, NULL);
    gst_pad_add_probe(src_pad_c1_ocr, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)src_counter_h2, pipeline, NULL);

    // Get the hailotracker source pad to send upstream events to
    GstElement *hailo_tracker = gst_bin_get_by_name(GST_BIN(pipeline), "hailo_tracker");
    GstPad *src_pad_hailo_tracker = gst_element_get_static_pad(hailo_tracker, "src");

    // Connect context 1 appsink callback
    GstElement *context_1_app_sink = gst_bin_get_by_name(GST_BIN(pipeline), CONTEXT_1_APP_SINK);
    g_signal_connect(context_1_app_sink, "new-sample", G_CALLBACK(context_1_ocr_results_callback), src_pad_hailo_tracker);

    // set fps-measurements signal callback to print the measurements
    if (print_fps)
    {
        GstElement *display_0 = gst_bin_get_by_name(GST_BIN(pipeline), CONTEXT_0_DISPLAY);
        g_signal_connect(display_0, "fps-measurements", G_CALLBACK(fps_measurements_callback), NULL);
    }
}

/**
 * @brief Handle ending messages from the pipeline (EOS, errors, etc...)
 * 
 * @param bus  -  GstBus*
 *        The GstBus to handle.
 *
 * @param msg  -  GstMessage*
 *        The GstMessage to handle.
 */
void handle_pipeline_end_messages(GstBus *bus, GstMessage *msg)
{
    if (msg != NULL)
    {
        GError *err;
        gchar *debug_info;
        switch (GST_MESSAGE_TYPE(msg))
        {
        case GST_MESSAGE_ERROR:
        {
            gst_message_parse_error(msg, &err, &debug_info);
            std::cerr << "Error received from element " << GST_OBJECT_NAME(msg->src) << ": " << err->message << std::endl;
            std::cerr << "Debugging information: ";
            if (debug_info)
            {
                std::cerr << debug_info << std::endl;
            }
            else
            {
                std::cerr << "none" << std::endl;
            }
            g_clear_error(&err);
            g_free(debug_info);
            break;
        }
        case GST_MESSAGE_EOS:
        {
            std::cout << "End-Of-Stream reached." << std::endl;
            break;
        }
        default:
        {
            // We should not reach here because we only asked for ERRORs and EOS
            std::cout << "Unexpected message received." << std::endl;
            break;
        }
        }
        gst_message_unref(msg);
    }
}

//******************************************************************
// MAIN
//******************************************************************
/**
 * @brief Build command line arguments.
 * 
 * @return cxxopts::Options 
 *         The available user arguments.
 */
cxxopts::Options build_arg_parser()
{
    cxxopts::Options options("License Plate Recognition");
    options.allow_unrecognised_options();
    options.add_options()
    ("h,help", "Show this help")
    ("i, input", "Set the input source (default $input_source)", cxxopts::value<std::string>()->default_value(std::string(TAPPAS_WORKSPACE) + std::string(DEFAULT_VIDEO_SRC)))
    ("show-fps", "Print fps");
    return options;
}

int main(int argc, char *argv[])
{
    // Set the working directory
    TAPPAS_WORKSPACE = std::getenv("TAPPAS_WORKSPACE");

    // Parse user arguments
    cxxopts::Options options = build_arg_parser();
    auto result = options.parse(argc, argv);
    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        exit(0);
    }
    PRINT_FPS = (gboolean)result.count("show-fps");

    // Prepare pipeline components
    GstBus *bus;
    GstMessage *msg;
    GMainLoop *loop;
    std::string src_pipeline_string;
    gst_init(&argc, &argv);  // Initialize Gstreamer
    loop = g_main_loop_new (NULL, FALSE);

    // Create the pipeline
    std::string pipeline_string = create_pipeline_string(result["input"].as<std::string>());
    GstElement *pipeline = gst_parse_launch(pipeline_string.c_str(), NULL);
    set_probe_callbacks(pipeline, PRINT_FPS);            // Set probe callbacks
    gst_element_set_state(pipeline, GST_STATE_PLAYING);  // Set the pipeline state to PLAYING
    g_main_loop_run(loop);

    // Extract closing messages
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                     (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    handle_pipeline_end_messages(bus, msg);

    // Free resources
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_deinit();
    gst_object_unref(pipeline);
    gst_object_unref(bus);
    gst_message_unref(msg);
    g_main_loop_unref (loop);

    return 0;
}