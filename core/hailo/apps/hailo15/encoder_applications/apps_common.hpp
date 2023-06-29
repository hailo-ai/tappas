#pragma once
#include <gst/gst.h>

GstFlowReturn wait_for_end_of_pipeline(GstElement *pipeline);