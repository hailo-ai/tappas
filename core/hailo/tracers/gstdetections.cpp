/**
 * SECTION:gstdetections
 * @short_description: A tracing module that prints detections info at every sink pad.
 *
 */

#include "gstdetections.hpp"
#include "gstctf.hpp"
#include "gst_hailo_meta.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_detections_debug);
#define GST_CAT_DEFAULT gst_detections_debug

struct _GstDetectionsTracer
{
    GstSharkTracer parent;
};

#define _do_init \
    GST_DEBUG_CATEGORY_INIT(gst_detections_debug, "detections", 0, "detections tracer");

G_DEFINE_TYPE_WITH_CODE(GstDetectionsTracer, gst_detections_tracer,
                        GST_SHARK_TYPE_TRACER, _do_init);

static void gst_detections_buffer_pre(GObject *self, GstClockTime ts,
                                      GstPad *pad, GstBuffer *buffer);

static GstTracerRecord *tr_detections;

static void
gst_detections_buffer_pre(GObject *self, GstClockTime ts, GstPad *pad,
                          GstBuffer *buffer)
{
    HailoROIPtr hailo_roi;
    gchar *pad_name;
    guint64 offset;

    if (NULL == buffer)
    {
        return;
    }
    hailo_roi = get_hailo_main_roi(buffer, false);
    if (NULL == hailo_roi)
    {
        return;
    }
    
    pad_name = g_strdup_printf("%s:%s", GST_DEBUG_PAD_NAME(pad));
    offset = GST_BUFFER_OFFSET(buffer);

    for (auto obj : hailo_roi->get_objects())
    {
        if (obj->get_type() == HAILO_DETECTION)
        {
            HailoDetectionPtr detection = std::dynamic_pointer_cast<HailoDetection>(obj);
            auto detection_bbox = detection->get_bbox();
            gst_tracer_record_log(tr_detections,
                                  detection->get_label().c_str(),
                                  pad_name,
                                  offset,
                                  detection_bbox.xmin(),
                                  detection_bbox.ymin(),
                                  detection_bbox.xmax(),
                                  detection_bbox.ymax());
        }
    }
}

/* tracer class */
static void
gst_detections_tracer_class_init(GstDetectionsTracerClass *klass)
{

    tr_detections = gst_tracer_record_new("detections.class",
                                          "label",
                                          GST_TYPE_STRUCTURE, gst_structure_new("value", "type", G_TYPE_GTYPE, G_TYPE_STRING, "description", G_TYPE_STRING, "The detection's label", NULL),
                                          "pad",
                                          GST_TYPE_STRUCTURE, gst_structure_new("value", "type", G_TYPE_GTYPE, G_TYPE_STRING, "description", G_TYPE_STRING, "The pad which the buffer is going through", NULL),
                                          "offset",
                                          GST_TYPE_STRUCTURE, gst_structure_new("value", "type", G_TYPE_GTYPE, G_TYPE_UINT64, "description", G_TYPE_STRING, "Offset", "min", G_TYPE_UINT64, G_GUINT64_CONSTANT(0), "max", G_TYPE_UINT64, G_MAXUINT64, NULL),
                                          "xmin",
                                          GST_TYPE_STRUCTURE, gst_structure_new("value", "type", G_TYPE_GTYPE, G_TYPE_FLOAT, "description", G_TYPE_STRING, "the minimum x value of the bounding box", NULL),
                                          "ymin",
                                          GST_TYPE_STRUCTURE, gst_structure_new("value", "type", G_TYPE_GTYPE, G_TYPE_FLOAT, "description", G_TYPE_STRING, "the minimum y value of the bounding box", NULL),
                                          "xmax",
                                          GST_TYPE_STRUCTURE, gst_structure_new("value", "type", G_TYPE_GTYPE, G_TYPE_FLOAT, "description", G_TYPE_STRING, "the maximum x value of the bounding box", NULL),
                                          "ymax",
                                          GST_TYPE_STRUCTURE, gst_structure_new("value", "type", G_TYPE_GTYPE, G_TYPE_FLOAT, "description", G_TYPE_STRING, "the maximum y value of the bounding box", NULL),
                                          NULL);
}

static void
gst_detections_tracer_init(GstDetectionsTracer *self)
{
    GstSharkTracer *tracer = GST_SHARK_TRACER(self);
    gst_shark_tracer_register_hook(tracer, "pad-push-pre",
                                   G_CALLBACK(gst_detections_buffer_pre));
}
