
#include <iostream>
#include "apps_common.hpp"

GstFlowReturn wait_for_end_of_pipeline(GstElement *pipeline)
{
    GstBus *bus;
    GstMessage *msg;
    GstFlowReturn ret = GST_FLOW_ERROR;
    bus = gst_element_get_bus(pipeline);
    // This function blocks until an error or EOS message is received.
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

    if (msg != NULL)
    {
        GError *err;
        gchar *debug_info;
        switch (GST_MESSAGE_TYPE(msg))
        {
            case GST_MESSAGE_ERROR:
            {
                gst_message_parse_error(msg, &err, &debug_info);
                GST_ERROR("Error received from element %s: %s", GST_OBJECT_NAME(msg->src), err->message);

                std::string dinfo = debug_info ? std::string(debug_info) : "none";
                GST_ERROR("Debugging information : %s", dinfo.c_str());

                g_clear_error(&err);
                g_free(debug_info);
                ret = GST_FLOW_ERROR;
                break;
            }
            case GST_MESSAGE_EOS:
            {
                GST_INFO("End-Of-Stream reached");
                ret = GST_FLOW_OK;
                break;
            }
            default:
            {
                // We should not reach here because we only asked for ERRORs and EOS
                GST_WARNING("Unexpected message received %d", GST_MESSAGE_TYPE(msg));
                ret = GST_FLOW_ERROR;
                break;
            }
        }
        gst_message_unref(msg);
    }
    gst_object_unref(bus);
    return ret;
}