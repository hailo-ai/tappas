/**
* Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
* Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
**/
#pragma once

#include <gst/base/gstbasetransform.h>
#include "hailo_objects.hpp"
#include "export/encode_json.hpp"
#include <cstdio>

G_BEGIN_DECLS

#define GST_TYPE_HAILO_EXPORT_FILE (gst_hailoexportfile_get_type())
#define GST_HAILO_EXPORT_FILE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_EXPORT_FILE, GstHailoExportFile))
#define GST_HAILO_EXPORT_FILE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_EXPORT_FILE, GstHailoExportFileClass))
#define GST_IS_HAILO_EXPORT_FILE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_EXPORT_FILE))
#define GST_IS_HAILO_EXPORT_FILE_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_EXPORT_FILE))

typedef struct _GstHailoExportFile GstHailoExportFile;
typedef struct _GstHailoExportFileClass GstHailoExportFileClass;

struct _GstHailoExportFile
{
    GstBaseTransform base_hailoexportfile;
    gchar *file_path;
    FILE* json_file;
    uint buffer_offset;
};

struct _GstHailoExportFileClass
{
    GstBaseTransformClass base_hailoexportfile_class;
};

GType gst_hailoexportfile_get_type(void);

G_END_DECLS