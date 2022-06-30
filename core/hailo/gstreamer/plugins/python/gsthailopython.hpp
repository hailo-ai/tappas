/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once

#include <gst/video/gstvideofilter.h>
#include <gst/video/video.h>

G_BEGIN_DECLS

#define GST_TYPE_HAILO_PYTHON (gst_hailopython_get_type())
#define GST_HAILO_PYTHON(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_PYTHON, GstHailoPython))
#define GST_HAILO_PYTHON_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_PYTHON, GstHailoPythonClass))
#define GST_IS_HAILO_PYTHON(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_PYTHON))
#define GST_IS_HAILO_PYTHON_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_PYTHON))

typedef struct _GstHailoPython GstHailoPython;
typedef struct _GstHailoPythonClass GstHailoPythonClass;

struct PythonCallback;

struct _GstHailoPython
{
    GstVideoFilter base_hailopython;
    struct PythonCallback *python_callback;
    struct PythonCallback *python_finalize_callback;
    gchar *module_name;
    gchar *function_name;
    gchar *finalize_function_name;
};

struct _GstHailoPythonClass
{
    GstVideoFilterClass base_hailopython_class;
};

GType gst_hailopython_get_type(void);

G_END_DECLS
