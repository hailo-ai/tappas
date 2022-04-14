/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#ifndef _GST_HAILO_FILTER2_HPP_
#define _GST_HAILO_FILTER2_HPP_

#include <gst/base/gstbasetransform.h>
#include <map>
#include <vector>
#include "hailo_objects.hpp"

G_BEGIN_DECLS

#define GST_TYPE_HAILO_FILTER2 (gst_hailofilter2_get_type())
#define GST_HAILO_FILTER2(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_FILTER2, GstHailoFilter2))
#define GST_HAILO_FILTER2_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_FILTER2, GstHailoFilter2Class))
#define GST_IS_HAILO_FILTER2(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_FILTER2))
#define GST_IS_HAILO_FILTER2_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_FILTER2))

typedef struct _GstHailoFilter2 GstHailoFilter2;
typedef struct _GstHailoFilter2Class GstHailoFilter2Class;

struct _GstHailoFilter2
{
    GstBaseTransform base_hailofilter2;
    gchar *default_function_name;
    gchar *lib_path;
    std::vector<gchar *> function_name;
    void *loaded_lib;
    std::vector<void (*)(HailoROIPtr)> handler;
};

struct _GstHailoFilter2Class
{
    GstBaseTransformClass base_hailofilter2_class;
};

GType gst_hailofilter2_get_type(void);

G_END_DECLS

#endif // _GST_HAILO_FILTER2_HPP_
