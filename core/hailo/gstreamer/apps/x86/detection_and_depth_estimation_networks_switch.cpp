/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#include <condition_variable>
#include <gst/gst.h>
#include <iostream>
#include <mutex>
#include <cxxopts.hpp>
#include <chrono>

#define NETWORK_SWITCH_FRAMES_COUNT (1)

// globals decleration
const gchar *tappas_workspace = "";
int net1_inferred = 0;
int net2_inferred = 0;
std::mutex mutex;
std::condition_variable condVar;

// flags
bool mutual_flag = false;
bool flag_hailonet_1_need_switch = false;
bool flag_hailonet_2_need_switch = false;
bool flag_identity1_got_eos = false;
bool flag_identity2_got_eos = false;
bool check_close_window;

// string consts
const gchar *HAILONET1_NAME = "hailonet_1";
const gchar *HAILONET2_NAME = "hailonet_2";
const gchar *IDENTITY_1_NAME = "identity_1";
const gchar *IDENTITY_2_NAME = "identity_2";

const std::string X86_DIR = "/apps/gstreamer/x86";
const std::string POSTPROCESS_DIR = X86_DIR + "/libs/";
const std::string RESOURCES_DIR = X86_DIR + "/network_switch/resources/";
const std::string DEFAULT_VIDEO_SRC = X86_DIR + "/network_switch/resources/instance_segmentation.mp4";

const std::string HAILONET_1_HEF = RESOURCES_DIR + "yolov5s.hef";
const std::string HAILONET_1_POST = POSTPROCESS_DIR + "libnew_yolo_post.so";
const std::string POST_1_FUNC_NAME = "yolov5";
const std::string HAILONET_2_HEF = RESOURCES_DIR + "fast_depth.hef";
const std::string HAILONET_2_POST = POSTPROCESS_DIR + "libdepth_estimation.so";


/**
 * @brief   This function deactivates one hailonet and activate the other. For that it has to flush all the buffers that are in the active hailonet before it changes anything.
  When the flush call returns, the active hailonet is empty from buffers, and since the probe callbacks are blocking, no buffers are allowed inside the hailonet at this point so switching is safe.
 * @param from hailonet element to deactivate
 * @param to hailonet element to activate
 */
void switch_activeness(GstElement *source, GstElement *destination)
{
    g_signal_emit_by_name(source, "flush"); // this signal is handeled in hailonet elements
    g_object_set(source, "is-active", false, NULL);
    g_object_set(destination, "is-active", true, NULL);
}

/**
 * @brief A pad callback on the sink pad of hailonet_1 element. This function is called with every buffer that passes this probe and handles deactivating hailonet_1 and activating hailonet_2 when it's time.
 *
 * @param pad the pad that the probe is set on
 * @param info extra info
 * @param user_data user data
 * @return GstPadProbeReturn OK
 */
static GstPadProbeReturn cb_hailonet_1(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    int net1_is_active;
    int net2_is_active;
    GstElement *pipeline = GST_ELEMENT(user_data);
    GstElement *hailonet_1 = gst_bin_get_by_name(GST_BIN(pipeline), HAILONET1_NAME);
    GstElement *hailonet_2 = gst_bin_get_by_name(GST_BIN(pipeline), HAILONET2_NAME);
    if (flag_hailonet_1_need_switch)
    {
        switch_activeness(hailonet_1, hailonet_2);
        flag_hailonet_1_need_switch = false;
        mutual_flag = false;
        condVar.notify_one();
    }

    if (flag_identity1_got_eos) // this is the last frame of the video so there shouldn't be waiting for the next buffer
    {
        return GST_PAD_PROBE_OK;
    }
    if (check_close_window)
    {
        return GST_PAD_PROBE_OK;
    }

    std::unique_lock<std::mutex> mlock(mutex);
    condVar.wait(mlock, []{ return mutual_flag; });
    // here hailonet1 is waiting for the mutual flag to turn to false. hailonet2 will change it to false when it is time to switch from hailonet2 to hailonet1.
    g_object_get(G_OBJECT(hailonet_2), "is-active", &net2_is_active, NULL);
    if ((net1_inferred + 1 - net2_inferred == NETWORK_SWITCH_FRAMES_COUNT) && (net2_is_active == 0))
    {
        flag_hailonet_1_need_switch = true; // turn on hailonet2
        net2_inferred++;
        return GST_PAD_PROBE_OK;
    }

    g_object_get(G_OBJECT(hailonet_1), "is-active", &net1_is_active, NULL);
    if (net1_is_active)
    {
        net1_inferred++;
    }

    mutual_flag = false;
    condVar.notify_one();

    return GST_PAD_PROBE_OK;
}

/**
 * @brief A pad callback on the sink pad of hailonet_2 element. This function is called with every buffer that passes this probe and handles deactivating hailonet_2 and activating hailonet_1 when it's time.
 *
 * @param pad the pad that the probe is set on
 * @param info extra info
 * @param user_data user data
 * @return GstPadProbeReturn OK
 */
static GstPadProbeReturn cb_hailonet_2(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    int net1_is_active;
    int net2_is_active;
    GstElement *pipeline = GST_ELEMENT(user_data);
    GstElement *hailonet_1 = gst_bin_get_by_name(GST_BIN(pipeline), HAILONET1_NAME);
    GstElement *hailonet_2 = gst_bin_get_by_name(GST_BIN(pipeline), HAILONET2_NAME);
    if (flag_hailonet_2_need_switch)
    {
        switch_activeness(hailonet_2, hailonet_1);

        flag_hailonet_2_need_switch = false;
        mutual_flag = true;
        condVar.notify_one();
    }

    if (flag_identity2_got_eos) // last frame of the video so don't wait for the next buffer
    {
        return GST_PAD_PROBE_OK;
    }
    if (check_close_window)
    {
        return GST_PAD_PROBE_OK;
    }

    std::unique_lock<std::mutex> mlock(mutex);
    condVar.wait(mlock, []{ return !mutual_flag; });
    // here hailonet2 is waiting for the mutual flag to turn to false. hailonet1 will change it to false when it is time to switch from hailonet1 to hailonet2.

    g_object_get(G_OBJECT(hailonet_1), "is-active", &net1_is_active, NULL);

    if ((net1_inferred + 1 == net2_inferred) && (net1_is_active == 0))
    {
        flag_hailonet_2_need_switch = true;
        net1_inferred++;
        return GST_PAD_PROBE_OK;
    }
    g_object_get(G_OBJECT(hailonet_2), "is-active", &net2_is_active, NULL);
    if (net2_is_active)
    {
        net2_inferred++;
    }

    mutual_flag = true;
    condVar.notify_one();
    return GST_PAD_PROBE_OK;
}

/**
 * @brief A pad callback on the sink pad of identity_1 element which is right before hailonet_1 element.
 * This function is called with every event that passes this probe. the function sets flag to true if an EOS event is detected.
 * Listens to events that passes through identity1 element and looks for EOS
 * @param pad the pad that the probe is set on
 * @param info extra info
 * @param user_data user data
 * @return GstPadProbeReturn OK
 */
static GstPadProbeReturn cb_event_identity(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    GstEvent *event = GST_EVENT(info->data);
    if (GST_EVENT_TYPE(event) == GST_EVENT_EOS)
    {
        int *id = (int *)user_data;
        if (*id == 1)
        {
            flag_identity1_got_eos = true;
        }
        else
        {
            flag_identity2_got_eos = true;
        }
    }
    return GST_PAD_PROBE_OK;
}

/**
 * @brief Create a pipeline string object
 *
 * @param input_src std::string represents a video file path or usb camera name (/dev/video*)
 * @return std::string the full pipeline string
 */
std::string create_pipeline_string(std::string input_src)
{
    std::string src_pipeline_string, pipeline_branch_1, pipeline_branch_2;
    if (input_src.rfind("/dev/video", 0) == 0)
    {
        src_pipeline_string = "v4l2src device=" + input_src + " ! "
                                                              "video/x-raw,width=640,height=480,pixel-aspect-ratio=1/1 ! "
                                                              "videoconvert qos=false ! "
                                                              "queue leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! tee name=t ";
    }
    else
    {
        src_pipeline_string = "filesrc name=src_0 location=" + input_src + " ! " +
                              "video/x-raw qos=false ! decodebin qos=false ! " +
                              "videoconvert qos=false ! " +
                              "queue leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! tee name=t ";
    }

    pipeline_branch_1 = "t. ! "
                        "videoscale qos=false ! "
                        "queue leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! "
                        "identity name=" +
                        std::string(IDENTITY_1_NAME) + " qos=false ! "
                                                       "queue leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! "
                                                       "hailonet hef-path=" +
                        tappas_workspace + HAILONET_1_HEF +
                        " debug=False is-active=false qos=false batch-size=1 device-count=1 vdevice-key=1 name=" +
                        std::string(HAILONET1_NAME) + " ! "
                                                      "queue leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! "
                                                      "hailofilter2 function-name=" +
                        POST_1_FUNC_NAME +
                        " so-path=" +
                        tappas_workspace + HAILONET_1_POST + " qos=false debug=False ! "
                                                             "queue leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! "
                                                             "hailooverlay qos=false ! "
                                                             "queue leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! videoconvert qos=false ! "
                                                             "queue leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! "
                                                             "fpsdisplaysink name=hailo_display_1 video-sink=ximagesink signal-fps-measurements=true text-overlay=false sync=false qos=false ";
    // this is one branch (a split) of the pipeline that include all elements to run a network

    pipeline_branch_2 = "t. ! "
                        "aspectratiocrop aspect-ratio=1/1 ! "
                        "queue leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! "
                        "videoscale qos=false ! "
                        "queue leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! "
                        "identity name=" +
                        std::string(IDENTITY_2_NAME) + " qos=false ! "
                                                       "queue leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! "
                                                       "hailonet hef-path=" +
                        tappas_workspace + HAILONET_2_HEF +
                        " debug=False is-active=true qos=false batch-size=1 device-count=1 vdevice-key=1 name=" +
                        std::string(HAILONET2_NAME) + " ! "
                                                      "queue leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! "
                                                      "hailofilter2 so-path=" +
                        tappas_workspace + HAILONET_2_POST + " qos=false ! "
                                                             "queue leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! "
                                                             "videoconvert qos=false ! "
                                                             "queue leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! "
                                                             "hailooverlay qos=false ! "
                                                             "queue name=hailo_display_q_1 leaky=no max-size-buffers=1 max-size-bytes=0 max-size-time=0 ! videoconvert ! "
                                                             "fpsdisplaysink name=hailo_display_2 video-sink=ximagesink signal-fps-measurements=true text-overlay=false sync=false qos=false ";
    // this is one branch (a split) of the pipeline that include all elements to run a network

    // building the pipeline:
    return (src_pipeline_string + pipeline_branch_1 + pipeline_branch_2);
}

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

/**
 * @brief extract the elements from the pipeline, extract the pads, set the probe callbacks and signal callbacks
 *
 * @param pipeline the pipeline
 */
void set_callbacks(GstElement *pipeline, gboolean print_fps)
{
    // extract elements from pipeline
    GstElement *hailonet_1 = gst_bin_get_by_name(GST_BIN(pipeline), HAILONET1_NAME);
    GstElement *hailonet_2 = gst_bin_get_by_name(GST_BIN(pipeline), HAILONET2_NAME);
    GstElement *identity1 = gst_bin_get_by_name(GST_BIN(pipeline), IDENTITY_1_NAME);
    GstElement *identity2 = gst_bin_get_by_name(GST_BIN(pipeline), IDENTITY_2_NAME);
    GstElement *display_1 = gst_bin_get_by_name(GST_BIN(pipeline), "hailo_display_1");
    GstElement *display_2 = gst_bin_get_by_name(GST_BIN(pipeline), "hailo_display_2");

    // extract pads from elements
    GstPad *pad_hailonet_1 = gst_element_get_static_pad(hailonet_1, "sink");
    GstPad *pad_hailonet_2 = gst_element_get_static_pad(hailonet_2, "sink");
    GstPad *identity_pad1 = gst_element_get_static_pad(identity1, "sink");
    GstPad *identity_pad2 = gst_element_get_static_pad(identity2, "sink");

    // set probes
    int id1 = 1;
    int id2 = 2;
    gst_pad_add_probe(pad_hailonet_1, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)cb_hailonet_1, pipeline, NULL);
    gst_pad_add_probe(pad_hailonet_2, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback)cb_hailonet_2, pipeline, NULL);
    gst_pad_add_probe(identity_pad1, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, (GstPadProbeCallback)cb_event_identity, &id1, NULL);
    gst_pad_add_probe(identity_pad2, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, (GstPadProbeCallback)cb_event_identity, &id2, NULL);

    if (print_fps)
    {
        // set fps-measurements signal callback to print the measurements
        g_signal_connect(display_1, "fps-measurements", G_CALLBACK(fps_measurements_callback), NULL);
        g_signal_connect(display_2, "fps-measurements", G_CALLBACK(fps_measurements_callback), NULL);
    }
}

cxxopts::Options build_arg_parser()
{
    cxxopts::Options options("Detection and Fast Depth - networks switch");
    options.allow_unrecognised_options();
    options.add_options()
     ("h,help", "Show this help")
     ("i, input", "Set the input source (default $input_source)", cxxopts::value<std::string>()->default_value(std::string(tappas_workspace) + std::string(DEFAULT_VIDEO_SRC)))
     ("show-fps", "Print fps");
     return options;
}

int main(int argc, char *argv[])
{

    tappas_workspace = std::getenv("TAPPAS_WORKSPACE");
    GstBus *bus;
    GstMessage *msg;
    std::string src_pipeline_string;
    cxxopts::Options options = build_arg_parser();
    auto result = options.parse(argc, argv);

    if (result.count("help"))
    {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    gboolean print_fps = (gboolean)result.count("show-fps");

    gst_init(&argc, &argv);

    std::string pipeline_string = create_pipeline_string(result["input"].as<std::string>());
    GstElement *pipeline = gst_parse_launch(pipeline_string.c_str(), NULL);
    set_callbacks(pipeline, print_fps);
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    bus = gst_element_get_bus(pipeline);
    msg =
        gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
                                   (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    if (msg != NULL)
    {
        GError *err;
        gchar *debug_info;
        switch (GST_MESSAGE_TYPE(msg))
        {
        case GST_MESSAGE_ERROR:
        {
            gst_message_parse_error(msg, &err, &debug_info);
            if (std::string(err->message) == "Output window was closed")
            {
                check_close_window = true;
            }
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
    // Free resources
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_deinit();
    gst_object_unref(pipeline);
    gst_object_unref(bus);

    return 0;
}
