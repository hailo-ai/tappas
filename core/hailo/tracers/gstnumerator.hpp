/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
#pragma once 

#include "gstsharktracer.hpp"

G_BEGIN_DECLS

#define GST_TYPE_NUMERATOR_TRACER (gst_numerator_tracer_get_type ())
G_DECLARE_FINAL_TYPE (GstNumeratorTracer, gst_numerator_tracer, GST, NUMERATOR_TRACER, GstSharkTracer)

G_END_DECLS
