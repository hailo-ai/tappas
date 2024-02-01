#include "gsthailonvalve.hpp"

#include <string.h>

GST_DEBUG_CATEGORY_STATIC(hailonvalve_debug);
#define GST_CAT_DEFAULT (hailonvalve_debug)

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE("sink",
                                                                   GST_PAD_SINK,
                                                                   GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE("src",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_ALWAYS,
                                                                  GST_STATIC_CAPS_ANY);

enum
{
    PROP_0,
    PROP_N_FRAMES
};

#define DEFAULT_NFRAMES 100

static void gst_hailonvalve_set_property(GObject *object,
                                         guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_hailonvalve_get_property(GObject *object,
                                         guint prop_id, GValue *value, GParamSpec *pspec);

static GstFlowReturn gst_hailonvalve_chain(GstPad *pad, GstObject *parent,
                                           GstBuffer *buffer);
static gboolean gst_hailonvalve_sink_event(GstPad *pad, GstObject *parent,
                                           GstEvent *event);
static gboolean gst_hailonvalve_query(GstPad *pad, GstObject *parent,
                                      GstQuery *query);

#define _do_init \
    GST_DEBUG_CATEGORY_INIT(hailonvalve_debug, "hailonvalve", 0, "HailoNValve");
#define gst_hailonvalve_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstHailoNValve, gst_hailonvalve, GST_TYPE_ELEMENT, _do_init);

static void
gst_hailonvalve_class_init(GstHailoNValveClass *klass)
{
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;

    gobject_class = (GObjectClass *)klass;
    gstelement_class = (GstElementClass *)(klass);

    gobject_class->set_property = gst_hailonvalve_set_property;
    gobject_class->get_property = gst_hailonvalve_get_property;

    g_object_class_install_property(gobject_class, PROP_N_FRAMES,
                                    g_param_spec_uint("nframes", "number of frames to drop",
                                                      "how many frames to drop before opening the valve",
                                                      0, G_MAXINT, DEFAULT_NFRAMES, (GParamFlags)(G_PARAM_READWRITE | GST_PARAM_MUTABLE_PLAYING | G_PARAM_STATIC_STRINGS)));

    gst_element_class_add_static_pad_template(gstelement_class, &srctemplate);
    gst_element_class_add_static_pad_template(gstelement_class, &sinktemplate);

    gst_element_class_set_static_metadata(gstelement_class, "HailoNValve element",
                                          "Filter", "Drops buffers and events or lets them through",
                                          "Olivier Crete <olivier.crete@collabora.co.uk>");
}

static void
gst_hailonvalve_init(GstHailoNValve *hailonvalve)
{
    hailonvalve->drop = TRUE;
    hailonvalve->discont = FALSE;
    hailonvalve->n_frames = DEFAULT_NFRAMES;

    hailonvalve->srcpad = gst_pad_new_from_static_template(&srctemplate, "src");
    gst_pad_set_query_function(hailonvalve->srcpad,
                               GST_DEBUG_FUNCPTR(gst_hailonvalve_query));
    GST_PAD_SET_PROXY_CAPS(hailonvalve->srcpad);
    gst_element_add_pad(GST_ELEMENT(hailonvalve), hailonvalve->srcpad);

    hailonvalve->sinkpad = gst_pad_new_from_static_template(&sinktemplate, "sink");
    gst_pad_set_chain_function(hailonvalve->sinkpad,
                               GST_DEBUG_FUNCPTR(gst_hailonvalve_chain));
    gst_pad_set_event_function(hailonvalve->sinkpad,
                               GST_DEBUG_FUNCPTR(gst_hailonvalve_sink_event));
    gst_pad_set_query_function(hailonvalve->sinkpad,
                               GST_DEBUG_FUNCPTR(gst_hailonvalve_query));
    GST_PAD_SET_PROXY_CAPS(hailonvalve->sinkpad);
    GST_PAD_SET_PROXY_ALLOCATION(hailonvalve->sinkpad);
    gst_element_add_pad(GST_ELEMENT(hailonvalve), hailonvalve->sinkpad);
}

static void
gst_hailonvalve_set_property(GObject *object,
                             guint prop_id, const GValue *value, GParamSpec *pspec)
{
    GstHailoNValve *hailonvalve = GST_HAILO_NVALVE(object);

    switch (prop_id)
    {
    case PROP_N_FRAMES:
        hailonvalve->n_frames = g_value_get_uint(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_hailonvalve_get_property(GObject *object,
                             guint prop_id, GValue *value, GParamSpec *pspec)
{
    GstHailoNValve *hailonvalve = GST_HAILO_NVALVE(object);

    switch (prop_id)
    {
    case PROP_N_FRAMES:
        g_value_set_uint(value, hailonvalve->n_frames);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static gboolean
forward_sticky_events(GstPad *pad, GstEvent **event, gpointer user_data)
{
    GstHailoNValve *hailonvalve = (GstHailoNValve *)user_data;

    if (!gst_pad_push_event(hailonvalve->srcpad, gst_event_ref(*event)))
        hailonvalve->need_repush_sticky = TRUE;

    return TRUE;
}

static void
gst_hailonvalve_repush_sticky(GstHailoNValve *hailonvalve)
{
    hailonvalve->need_repush_sticky = FALSE;
    gst_pad_sticky_events_foreach(hailonvalve->sinkpad, forward_sticky_events, hailonvalve);
}

static GstFlowReturn
gst_hailonvalve_chain(GstPad *pad, GstObject *parent, GstBuffer *buffer)
{
    GstHailoNValve *hailonvalve = GST_HAILO_NVALVE(parent);
    GstFlowReturn ret = GST_FLOW_OK;

    if (hailonvalve->drop)
    {
        hailonvalve->count += 1;
        if (hailonvalve->count >= hailonvalve->n_frames)
            hailonvalve->drop = FALSE;
    }
    GST_BUFFER_OFFSET(buffer) -= hailonvalve->count;

    if (g_atomic_int_get(&hailonvalve->drop))
    {
        gst_buffer_unref(buffer);
        hailonvalve->discont = TRUE;
    }
    else
    {
        if (hailonvalve->discont)
        {
            buffer = gst_buffer_make_writable(buffer);
            GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_DISCONT);
            hailonvalve->discont = FALSE;
        }

        if (hailonvalve->need_repush_sticky)
            gst_hailonvalve_repush_sticky(hailonvalve);

        ret = gst_pad_push(hailonvalve->srcpad, buffer);
    }

    /* Ignore errors if "drop" was changed while the thread was blocked
     * downwards
     */
    if (g_atomic_int_get(&hailonvalve->drop))
        ret = GST_FLOW_OK;

    return ret;
}

static gboolean
gst_hailonvalve_sink_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
    GstHailoNValve *hailonvalve;
    gboolean is_sticky = GST_EVENT_IS_STICKY(event);
    gboolean ret = TRUE;

    hailonvalve = GST_HAILO_NVALVE(parent);

    if (g_atomic_int_get(&hailonvalve->drop))
    {
        hailonvalve->need_repush_sticky |= is_sticky;
        gst_event_unref(event);
    }
    else
    {
        if (hailonvalve->need_repush_sticky)
            gst_hailonvalve_repush_sticky(hailonvalve);
        ret = gst_pad_event_default(pad, parent, event);
    }

    /* Ignore errors if "drop" was changed while the thread was blocked
     * downwards.
     */
    if (g_atomic_int_get(&hailonvalve->drop))
    {
        hailonvalve->need_repush_sticky |= is_sticky;
        ret = TRUE;
    }

    return ret;
}

static gboolean
gst_hailonvalve_query(GstPad *pad, GstObject *parent, GstQuery *query)
{
    GstHailoNValve *hailonvalve = GST_HAILO_NVALVE(parent);

    if (GST_QUERY_IS_SERIALIZED(query) && g_atomic_int_get(&hailonvalve->drop))
        return FALSE;

    return gst_pad_query_default(pad, parent, query);
}