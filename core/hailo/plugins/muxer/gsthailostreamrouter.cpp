/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
/*
 * Gsthailostreamrouter.hpp: Simple STREAM_ROUTER
 */

#include "gsthailostreamrouter.hpp"
#include "gst_hailo_meta.hpp"
#include "gst_hailo_stream_meta.hpp"
#include <iostream>
#include <gst/gst.h>

GST_DEBUG_CATEGORY_STATIC(gst_hailo_stream_router_debug);
#define GST_CAT_DEFAULT gst_hailo_stream_router_debug

GType gst_hailo_stream_router_pad_get_type(void);
// Define HailoStreamRouterPad type
#define GST_TYPE_HAILO_STREAM_ROUTER_PAD \
    (gst_hailo_stream_router_pad_get_type())
#define GST_HAILO_STREAM_ROUTER_PAD(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_STREAM_ROUTER_PAD, GstHailoStreamRouterPad))
#define GST_HAILO_STREAM_ROUTER_PAD_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_STREAM_ROUTER_PAD, GstHailoStreamRouterPadClass))
#define GST_IS_HAILO_STREAM_ROUTER_PAD(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_STREAM_ROUTER_PAD))
#define GST_IS_HAILO_STREAM_ROUTER_PAD_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_STREAM_ROUTER_PAD))
#define GST_HAILO_STREAM_ROUTER_PAD_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), GST_TYPE_HAILO_STREAM_ROUTER_PAD, GstHailoStreamRouterPadClass))
#define GST_HAILO_STREAM_ROUTER_PAD_CAST(obj) \
    ((GstHailoStreamRouterPad *)(obj))

#define GST_HAILO_STREAM_ROUTER_MAX_INPUT_PADS 40

typedef struct _GstHailoStreamRouterPad GstHailoStreamRouterPad;
typedef struct _GstHailoStreamRouterPadClass GstHailoStreamRouterPadClass;

struct _GstHailoStreamRouterPad
{
    GstPad parent;

    /* properties */
    const gchar *input_streams[GST_HAILO_STREAM_ROUTER_MAX_INPUT_PADS];
    /* < private > */
    guint num_input_streams;
    GMutex lock;
};

struct _GstHailoStreamRouterPadClass
{
    GstPadClass parent_class;
};

G_DEFINE_TYPE(GstHailoStreamRouterPad, gst_hailo_stream_router_pad, GST_TYPE_PAD);

enum
{
    PROP_PAD_0,
    PROP_PAD_INPUT_STREAMS,
};

// Pad Templates
static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE("sink",
                                                                    GST_PAD_SINK,
                                                                    GST_PAD_ALWAYS,
                                                                    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE("src_%u",
                                                                   GST_PAD_SRC,
                                                                   GST_PAD_REQUEST,
                                                                   GST_STATIC_CAPS_ANY);

static GstPad *gst_hailo_stream_router_request_new_pad(GstElement *element, GstPadTemplate *templ,
                                                       const gchar *re_name, const GstCaps *caps);
static void gst_hailo_stream_router_release_pad(GstElement *element, GstPad *pad);

static GstStateChangeReturn gst_hailo_stream_router_change_state(GstElement *element, GstStateChange transition);
static GstFlowReturn gst_hailo_stream_router_sink_chain(GstPad *pad, GstObject *parent, GstBuffer *buffer);

static void gst_hailo_stream_router_pad_set_property(GObject *object, guint prop_id,
                                                     const GValue *value, GParamSpec *pspec);

static void gst_hailo_stream_router_pad_get_property(GObject *object, guint prop_id,
                                                     GValue *value, GParamSpec *pspec);

static void gst_hailo_stream_router_dispose(GObject *object);
static void gst_hailo_stream_router_reset(GstHailoStreamRouter *hailo_stream_router);

static gboolean gst_hailo_stream_router_sink_event(GstPad *pad, GstObject *parent, GstEvent *event);

static void gst_hailo_stream_router_finalize(GObject *object);

static void gst_hailo_stream_router_release_srcpad(const GValue *item, GstHailoStreamRouter *hailo_stream_router);

static void gst_hailo_stream_router_child_proxy_init(gpointer g_iface, gpointer iface_data);

#define gst_hailo_stream_router_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstHailoStreamRouter, gst_hailo_stream_router,
                        GST_TYPE_ELEMENT,
                        GST_DEBUG_CATEGORY_INIT(gst_hailo_stream_router_debug, "hailostreamrouter", 0, "Hailo Stream Router");
                        G_IMPLEMENT_INTERFACE(GST_TYPE_CHILD_PROXY, gst_hailo_stream_router_child_proxy_init));

static void
gst_hailo_stream_router_class_init(GstHailoStreamRouterClass *klass)
{
    GObjectClass *gobject_class = (GObjectClass *)klass;
    GstElementClass *gstelement_class = (GstElementClass *)klass;

    gobject_class->finalize = GST_DEBUG_FUNCPTR(gst_hailo_stream_router_finalize);
    gobject_class->dispose = GST_DEBUG_FUNCPTR(gst_hailo_stream_router_dispose);

    // Request and release pads
    gstelement_class->request_new_pad = GST_DEBUG_FUNCPTR(gst_hailo_stream_router_request_new_pad);
    gstelement_class->release_pad = GST_DEBUG_FUNCPTR(gst_hailo_stream_router_release_pad);

    gstelement_class->change_state = GST_DEBUG_FUNCPTR(gst_hailo_stream_router_change_state);

    // Pad templates
    gst_element_class_add_static_pad_template_with_gtype(gstelement_class, &src_template, GST_TYPE_HAILO_STREAM_ROUTER_PAD);

    gst_element_class_add_static_pad_template(gstelement_class, &sink_template);

    gst_element_class_set_static_metadata(gstelement_class, "Hailo Stream Router", "Generic",
                                          "Hailo Stream Router", "Hailo");

    g_type_class_ref(GST_TYPE_HAILO_STREAM_ROUTER_PAD);
}

static void
gst_hailo_stream_router_init(GstHailoStreamRouter *hailo_stream_router)
{
    // Configure sink pad
    hailo_stream_router->sinkpad = gst_pad_new_from_static_template(&sink_template, "sink");
    // Initialize hash table (of key -> value: input stream name -> target src_pads)
    hailo_stream_router->targets_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)g_array_unref);

    // Initialize element mutex
    g_mutex_init(&hailo_stream_router->lock);

    gst_pad_set_chain_function(hailo_stream_router->sinkpad, GST_DEBUG_FUNCPTR(gst_hailo_stream_router_sink_chain));
    gst_pad_set_event_function(hailo_stream_router->sinkpad, GST_DEBUG_FUNCPTR(gst_hailo_stream_router_sink_event));
    // gst_pad_set_query_function(hailo_stream_router->sinkpad, GST_DEBUG_FUNCPTR(gst_hailo_stream_router_sink_query));
    GST_PAD_SET_PROXY_CAPS(hailo_stream_router->sinkpad);
    gst_element_add_pad(GST_ELEMENT(hailo_stream_router), hailo_stream_router->sinkpad);
}

static void
gst_hailo_stream_router_dispose(GObject *object)
{
    GstHailoStreamRouter *hailo_stream_router = GST_HAILO_STREAM_ROUTER(object);
    gst_hailo_stream_router_reset(hailo_stream_router);
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static gpointer
gst_pads_lookup(GstHailoStreamRouter *hailo_stream_router, const gchar *input_pad_name)
{
    g_mutex_lock(&hailo_stream_router->lock);
    gpointer pads_ptr;
    pads_ptr = g_hash_table_lookup(hailo_stream_router->targets_table, (gpointer)input_pad_name);
    g_mutex_unlock(&hailo_stream_router->lock);
    return pads_ptr;
}

static void
insert_new_srcpad_to_target_pads_table(GstHailoStreamRouter *hailo_stream_router, GstHailoStreamRouterPad *router_srcpad)
{
    // Iterate over the input stream names configured in the pad's properties
    guint i;
    for (i = 0; i < router_srcpad->num_input_streams; i++)
    {
        // Get the input stream name
        const char *input_pad_name = router_srcpad->input_streams[i];
        // Get the array of pads from the hash table
        gpointer pads_ptr = gst_pads_lookup(hailo_stream_router, input_pad_name);
        if (pads_ptr)
        {
            g_mutex_lock(&hailo_stream_router->lock);
            // Add router_srcpad to the pads GArray
            GArray *pads = (GArray *)pads_ptr;
            g_array_append_val(pads, router_srcpad);

            // Replace the pads GArray in the hash table with the new one
            g_hash_table_replace(hailo_stream_router->targets_table, (gpointer)input_pad_name, (gpointer)pads);
            g_mutex_unlock(&hailo_stream_router->lock);
        }
        else
        {
            // Create a new GArray of pads and add router_srcpad to it
            GArray *pads = g_array_sized_new(FALSE, FALSE, sizeof(GstHailoStreamRouterPad *), router_srcpad->num_input_streams);
            g_mutex_lock(&hailo_stream_router->lock);
            g_array_append_val(pads, router_srcpad);
            // Add input_pad_name as key and GArray of pads as value to the hash table if both doesn't already exist
            g_hash_table_insert(hailo_stream_router->targets_table, (gpointer)input_pad_name, (gpointer)pads);
            g_mutex_unlock(&hailo_stream_router->lock);
        }
    }
}

static void
gst_hailo_stream_router_release_srcpad(const GValue *item, GstHailoStreamRouter *hailo_stream_router)
{
    GstPad *pad = GST_PAD(g_value_get_object(item));
    if (pad != NULL)
    {
        gst_pad_set_active(pad, FALSE);
        gst_element_remove_pad(GST_ELEMENT_CAST(hailo_stream_router), pad);
    }
}

static void
gst_hailo_stream_router_reset(GstHailoStreamRouter *hailo_stream_router)
{
    g_mutex_lock(&hailo_stream_router->lock);
    GST_OBJECT_LOCK(hailo_stream_router);
    if (hailo_stream_router->sinkpad != NULL)
    {
        hailo_stream_router->sinkpad = NULL;
    }

    if (hailo_stream_router->targets_table != NULL)
    {
        g_hash_table_remove_all(hailo_stream_router->targets_table);
        g_hash_table_unref(hailo_stream_router->targets_table);
        hailo_stream_router->targets_table = NULL;
    }
    GST_OBJECT_UNLOCK(hailo_stream_router);

    GstIterator *it = NULL;
    GstIteratorResult itret = GST_ITERATOR_OK;
    it = gst_element_iterate_src_pads(GST_ELEMENT_CAST(hailo_stream_router));
    while (itret == GST_ITERATOR_OK || itret == GST_ITERATOR_RESYNC)
    {
        itret =
            gst_iterator_foreach(it,
                                 (GstIteratorForeachFunction)gst_hailo_stream_router_release_srcpad, hailo_stream_router);
        if (itret == GST_ITERATOR_RESYNC)
            gst_iterator_resync(it);
    }
    gst_iterator_free(it);
    g_mutex_unlock(&hailo_stream_router->lock);
}

static gboolean
gst_hailo_stream_router_sink_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
    gboolean res = TRUE;
    if (GST_EVENT_TYPE(event) == GST_EVENT_FLUSH_START || GST_EVENT_TYPE(event) == GST_EVENT_FLUSH_STOP || GST_EVENT_TYPE(event) == GST_EVENT_EOS)
    {
        res = gst_pad_event_default(pad, parent, event);
    }
    else
    {
        gst_event_unref(event);
    }

    return res;
}

static GstStateChangeReturn
gst_hailo_stream_router_change_state(GstElement *element, GstStateChange transition)
{
    GstStateChangeReturn result = GST_STATE_CHANGE_SUCCESS;
    GstHailoStreamRouter *hailo_stream_router = GST_HAILO_STREAM_ROUTER(element);
    result = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);

    switch (transition)
    {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
    {
        // Prepare hash table of pads (key -> value: input stream name -> target src_pads)
        GList *item;
        // Iterate over the src_pads of the element
        for (item = element->srcpads; item; item = item->next)
        {
            GstHailoStreamRouterPad *router_srcpad = GST_HAILO_STREAM_ROUTER_PAD(item->data);
            insert_new_srcpad_to_target_pads_table(hailo_stream_router, router_srcpad);
        }
        break;
    }
    case GST_STATE_CHANGE_PAUSED_TO_READY:
    {
        gst_hailo_stream_router_reset(hailo_stream_router);
        break;
    }
    default:
        break;
    }

    return result;
}

void gst_hailo_stream_router_finalize(GObject *object)
{
    GstHailoStreamRouter *stream_router = GST_HAILO_STREAM_ROUTER(object);
    g_mutex_clear(&stream_router->lock);
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void
gst_hailo_stream_router_pad_class_init(GstHailoStreamRouterPadClass *klass)
{
    GObjectClass *gobject_class = (GObjectClass *)klass;

    gobject_class->set_property = gst_hailo_stream_router_pad_set_property;
    gobject_class->get_property = gst_hailo_stream_router_pad_get_property;

    // Install input-streams char* array property
    g_object_class_install_property(gobject_class, PROP_PAD_INPUT_STREAMS,
                                    gst_param_spec_array("input-streams", "Pad input streams",
                                                         "Input streams of a srcpad",
                                                         g_param_spec_string("input-stream-name", "Input stream name",
                                                                             "Input stream", "",
                                                                             (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)),
                                                         (GParamFlags)(G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS)));
}

static void
set_input_streams(GstHailoStreamRouterPad *current_pad, const GValue *value)
{
    // Insert the elements of Gvalue represents input streams into the char array 'input_streams'
    if (value == NULL)
    {
        GST_ERROR("initialization of pad property input_streams: value is NULL");
        return;
    }

    guint i;
    guint len = gst_value_array_get_size(value);
    if (len > 0 && len < GST_HAILO_STREAM_ROUTER_MAX_INPUT_PADS)
    {
        for (i = 0; i < len; i++)
        {
            const GValue *val = gst_value_array_get_value(value, i);
            current_pad->input_streams[i] = g_strdup(g_value_get_string(val));
        }
        current_pad->num_input_streams = len;
    }
    else
    {
        std::cerr << "initialization of element property input_streams: value is " << len << " and must be between 0 to " << GST_HAILO_STREAM_ROUTER_MAX_INPUT_PADS << std::endl;
    }
    return;
}

static void
get_input_streams(GstHailoStreamRouterPad *current_pad, GValue *value)
{
    // Insert the elements of Gvalue represents input streams into the char array 'input_streams'
    if (value == NULL)
    {
        GST_ERROR("initialization of pad property input_streams: value is NULL");
        return;
    }
    guint len = gst_value_array_get_size(value);
    if (len > 0)
    {
        GST_ERROR("initialization of pad property input_streams: value an not empty array");
        return;
    }

    guint i;
    for (i = 0; i < current_pad->num_input_streams; i++)
    {
        GValue val = G_VALUE_INIT;
        g_value_init(&val, G_TYPE_STRING);
        g_value_set_string(&val, current_pad->input_streams[i]);
        gst_value_array_append_and_take_value(value, &val);
    }

}

static void
gst_hailo_stream_router_pad_init(GstHailoStreamRouterPad *pad)
{
    // Initialize pad mutex
    g_mutex_init(&pad->lock);

    g_mutex_lock(&pad->lock);

    // Initialize input_streams array of char*
    memset(pad->input_streams, 0, sizeof(pad->input_streams));

    guint i;
    // In first initialization - set input streams to an empty array
    pad->num_input_streams = 0;
    for (i = 0; i < GST_HAILO_STREAM_ROUTER_MAX_INPUT_PADS; i++)
    {
        pad->input_streams[i] = "";
    }

    g_mutex_unlock(&pad->lock);
}

static gboolean
forward_events(GstPad *pad, GstEvent **event, gpointer user_data)
{
    // This function pushes forward all events that are not EOS.
    GstPad *srcpad = GST_PAD_CAST(user_data);

    gst_pad_push_event(srcpad, gst_event_ref(*event));

    return TRUE;
}

/**
 * Chain method of the sink pad
 * On incomming buffer, get the target src_pad from the hash table, and forward the buffer to it.
 *
 * @param pad  The sink pad
 * @param parent GstObject stream_router element
 * @param buffer Incomming buffer (GstBuffer)
 * @return GstFlowReturn
 */
static GstFlowReturn
gst_hailo_stream_router_sink_chain(GstPad *pad, GstObject *parent, GstBuffer *buffer)
{
    GstHailoStreamRouter *stream_router = GST_HAILO_STREAM_ROUTER(parent);

    GstFlowReturn result = GST_FLOW_OK;

    // Get the input stream name from the stream metadata on the buffer
    gchar *input_pad_name = gst_buffer_get_hailo_stream_meta(buffer)->pad_name;

    // Lookup for the input stream name key and get it's value (target pads array) from the hash table
    gpointer pads_ptr = gst_pads_lookup(stream_router, input_pad_name);
    if (pads_ptr)
    {
        guint i;
        GArray *src_pads = (GArray *)pads_ptr;
        GstHailoStreamRouterPad *src_pad;
        // Iterate over the target src_pads
        for (i = 0; i < src_pads->len; i++)
        {
            src_pad = (GstHailoStreamRouterPad *)g_array_index(src_pads, GstHailoStreamRouterPad *, i);
            if (GST_IS_PAD(src_pad))
            {
                // Forward sticky events.
                gst_pad_sticky_events_foreach(pad, forward_events, src_pad);

                GstBuffer *buffer_copy = gst_buffer_copy(buffer);

                // Push the buffer to the src_pad
                result = gst_pad_push(GST_PAD(src_pad), buffer_copy);
            }
        }
    }

    gst_buffer_unref(buffer);
    return result;
}

static void
gst_hailo_stream_router_pad_set_property(GObject *object, guint prop_id,
                                         const GValue *value, GParamSpec *pspec)
{
    GstHailoStreamRouterPad *pad = GST_HAILO_STREAM_ROUTER_PAD(object);
    g_return_if_fail(GST_IS_HAILO_STREAM_ROUTER_PAD(pad));

    switch (prop_id)
    {
    case PROP_PAD_INPUT_STREAMS:
        g_mutex_lock(&pad->lock);
        set_input_streams(pad, value);
        g_mutex_unlock(&pad->lock);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_hailo_stream_router_pad_get_property(GObject *object, guint prop_id,
                                         GValue *value, GParamSpec *pspec)
{
    GstHailoStreamRouterPad *pad = GST_HAILO_STREAM_ROUTER_PAD(object);

    switch (prop_id)
    {
    case PROP_PAD_INPUT_STREAMS:
        g_mutex_lock(&pad->lock);
        get_input_streams(pad, value);
        g_mutex_unlock(&pad->lock);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static GstPad *
gst_hailo_stream_router_request_new_pad(GstElement *element, GstPadTemplate *templ,
                                        const gchar *name, const GstCaps *caps)
{
    GstPad *newpad;

    GST_DEBUG_OBJECT(element, "requesting pad");

    newpad = GST_PAD_CAST(g_object_new(GST_TYPE_HAILO_STREAM_ROUTER_PAD,
                                       "name", name, "direction", templ->direction, "template", templ,
                                       NULL));

    if (newpad == NULL)
    {
        GST_DEBUG_OBJECT(element, "could not create/add pad");
        return NULL;
    }

    gst_pad_set_active(newpad, TRUE);

    gst_element_add_pad(element, newpad);

    GST_DEBUG_OBJECT(element, "requested pad %s:%s",
                     GST_DEBUG_PAD_NAME(newpad));

    gst_child_proxy_child_added(GST_CHILD_PROXY(element), G_OBJECT(newpad), name);

    return newpad;
}

static void
gst_hailo_stream_router_release_pad(GstElement *element, GstPad *pad)
{
    GstHailoStreamRouter *hailo_stream_router = GST_HAILO_STREAM_ROUTER(element);
    GST_DEBUG_OBJECT(hailo_stream_router, "releasing pad %s:%s", GST_DEBUG_PAD_NAME(pad));
    gst_pad_set_active(pad, FALSE);
    gst_element_remove_pad(GST_ELEMENT_CAST(hailo_stream_router), pad);
}

/* GstChildProxy implementation - for using pad properties */
static guint
gst_hailo_stream_router_child_proxy_get_children_count(GstChildProxy *child_proxy)
{
    guint count = 0;
    GstHailoStreamRouter *hailo_stream_router = GST_HAILO_STREAM_ROUTER(child_proxy);

    GST_OBJECT_LOCK(hailo_stream_router);
    count = GST_ELEMENT_CAST(hailo_stream_router)->numsrcpads;
    GST_OBJECT_UNLOCK(hailo_stream_router);
    GST_INFO_OBJECT(hailo_stream_router, "Children Count: %d", count);

    return count;
}

static GObject *
gst_hailo_stream_router_child_proxy_get_child_by_index(GstChildProxy *child_proxy,
                                                       guint index)
{
    GstHailoStreamRouter *hailo_stream_router = GST_HAILO_STREAM_ROUTER(child_proxy);
    GObject *obj = NULL;

    GST_OBJECT_LOCK(hailo_stream_router);
    obj = G_OBJECT(g_list_nth_data(GST_ELEMENT_CAST(hailo_stream_router)->srcpads, index));
    if (obj)
        gst_object_ref(obj);
    GST_OBJECT_UNLOCK(hailo_stream_router);

    return obj;
}

static void
gst_hailo_stream_router_child_proxy_init(gpointer g_iface, gpointer iface_data)
{
    GstChildProxyInterface *iface = (GstChildProxyInterface *)g_iface;

    iface->get_child_by_index = gst_hailo_stream_router_child_proxy_get_child_by_index;
    iface->get_children_count = gst_hailo_stream_router_child_proxy_get_children_count;
}
