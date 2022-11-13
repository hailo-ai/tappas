/**
 * Copyright (c) 2021-2022 Hailo Technologies Ltd. All rights reserved.
 * Distributed under the LGPL license (https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt)
 **/
/*
 * GStreamer RoundRobin element
 *
 * gsthailoroundrobin.cpp: Simple Input Round Robin funnel (N->1) element, waits on all sinks, passes the first one, with all metadata included.
 */

#include "gsthailoroundrobin.hpp"
#include "gst_hailo_meta.hpp"
#include "gst_hailo_stream_meta.hpp"

GST_DEBUG_CATEGORY_STATIC(gst_hailo_round_robin_debug);
#define GST_CAT_DEFAULT gst_hailo_round_robin_debug

GType gst_hailo_round_robin_pad_get_type(void);
#define GST_TYPE_HAILO_ROUND_ROBIN_PAD \
    (gst_hailo_round_robin_pad_get_type())
#define GST_HAILO_ROUND_ROBIN_PAD(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_ROUND_ROBIN_PAD, GstHailoRoundRobinPad))
#define GST_HAILO_ROUND_ROBIN_PAD_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_ROUND_ROBIN_PAD, GstHailoRoundRobinPadClass))
#define GST_IS_ROUND_ROBIN_PAD(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_ROUND_ROBIN_PAD))
#define GST_IS_ROUND_ROBIN_PAD_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_ROUND_ROBIN_PAD))
#define GST_HAILO_ROUND_ROBIN_PAD_CAST(obj) \
    ((GstHailoRoundRobinPad *)(obj))

typedef struct _GstHailoRoundRobinPad GstHailoRoundRobinPad;
typedef struct _GstHailoRoundRobinPadClass GstHailoRoundRobinPadClass;

struct _GstHailoRoundRobinPad
{
    GstPad parent;
    gboolean got_eos;
};

struct _GstHailoRoundRobinPadClass
{
    GstPadClass parent;
};

static size_t get_pad_num(GstPad *pad)
{
    gchar *name = gst_pad_get_name(pad);
    std::string pad_name_str(name);
    std::string pad_num_str = pad_name_str.substr(pad_name_str.find("_") + 1);
    size_t pad_num = stoi(pad_num_str);
    g_free(name);
    return pad_num;
}

G_DEFINE_TYPE(GstHailoRoundRobinPad, gst_hailo_round_robin_pad, GST_TYPE_PAD);

#define DEFAULT_FORWARD_STICKY_EVENTS TRUE

enum
{
    PROP_0,
    PROP_FUNNEL_MODE,
};

static void
gst_hailo_round_robin_pad_class_init(GstHailoRoundRobinPadClass *klass)
{
}

static void
gst_hailo_round_robin_pad_init(GstHailoRoundRobinPad *pad)
{
    pad->got_eos = FALSE;
}

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE("sink_%u",
                                                                    GST_PAD_SINK,
                                                                    GST_PAD_REQUEST,
                                                                    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE("src",
                                                                   GST_PAD_SRC,
                                                                   GST_PAD_ALWAYS,
                                                                   GST_STATIC_CAPS_ANY);

#define _do_init \
    GST_DEBUG_CATEGORY_INIT(gst_hailo_round_robin_debug, "hailo_round_robin", 0, "hailo_round_robin element");
#define gst_hailo_round_robin_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstHailoRoundRobin, gst_hailo_round_robin, GST_TYPE_ELEMENT, _do_init);

static GstStateChangeReturn gst_hailo_round_robin_change_state(GstElement *element,
                                                               GstStateChange transition);

static GstFlowReturn gst_hailo_round_robin_sink_chain(GstPad *pad,
                                                      GstObject *parent,
                                                      GstBuffer *buf);

static gboolean gst_hailo_round_robin_sink_event(GstPad *pad,
                                                 GstObject *parent,
                                                 GstEvent *event);

static GstPad *gst_hailo_round_robin_request_new_pad(GstElement *element,
                                                     GstPadTemplate *templ,
                                                     const gchar *name,
                                                     const GstCaps *caps);

static void gst_hailo_round_robin_release_pad(GstElement *element, GstPad *pad);
static void gst_hailo_round_robin_dispose(GObject *object);


static void
gst_hailo_round_robin_set_property(GObject *object, guint prop_id,
                                   const GValue *value, GParamSpec *pspec)
{
    switch (prop_id)
    {
    case PROP_FUNNEL_MODE:
        GST_HAILO_ROUND_ROBIN(object)->funnel_mode = g_value_get_boolean(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void
gst_hailo_round_robin_get_property(GObject *object, guint prop_id, GValue *value,
                                   GParamSpec *pspec)
{
    switch (prop_id)
    {
    case PROP_FUNNEL_MODE:
        g_value_set_boolean(value, GST_HAILO_ROUND_ROBIN(object)->funnel_mode);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}



static void
gst_hailo_round_robin_class_init(GstHailoRoundRobinClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);

    gobject_class->set_property = gst_hailo_round_robin_set_property;
    gobject_class->get_property = gst_hailo_round_robin_get_property;
    gobject_class->dispose = GST_DEBUG_FUNCPTR(gst_hailo_round_robin_dispose);

    gst_element_class_set_static_metadata(gstelement_class,
                                          "Input Round Robin element", "Generic", "multiple input into one output in roundrobin order",
                                          "hailo.ai <contact@hailo.ai>");

    gst_element_class_add_static_pad_template(gstelement_class, &sink_template);
    gst_element_class_add_static_pad_template(gstelement_class, &src_template);

    gstelement_class->request_new_pad =
        GST_DEBUG_FUNCPTR(gst_hailo_round_robin_request_new_pad);
    gstelement_class->release_pad = GST_DEBUG_FUNCPTR(gst_hailo_round_robin_release_pad);
    gstelement_class->change_state = GST_DEBUG_FUNCPTR(gst_hailo_round_robin_change_state);

    // install new property funnel_mode
    g_object_class_install_property(gobject_class,
                                    PROP_FUNNEL_MODE,
                                    g_param_spec_boolean("funnel-mode",
                                                         "Funnel mode",
                                                         "Disables the round robin logic and pushes all buffers when available",
                                                         false,
                                                         (GParamFlags)(GST_PARAM_CONTROLLABLE | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
}

static void
gst_hailo_round_robin_init(GstHailoRoundRobin *hailo_round_robin)
{
    hailo_round_robin->current_pad_num = 0;
    hailo_round_robin->mutexes.clear();
    hailo_round_robin->condition_vars.clear();
    hailo_round_robin->srcpad = gst_pad_new_from_static_template(&src_template, "src");
    hailo_round_robin->funnel_mode = false;
    gst_pad_use_fixed_caps(hailo_round_robin->srcpad);

    gst_element_add_pad(GST_ELEMENT(hailo_round_robin), hailo_round_robin->srcpad);
}

static void
gst_hailo_round_robin_dispose(GObject *object)
{
    GstHailoRoundRobin *hailo_round_robin = GST_HAILO_ROUND_ROBIN_CAST(object);
    hailo_round_robin->srcpad = NULL;
    hailo_round_robin->current_pad_num = 0;
    hailo_round_robin->mutexes.clear();
    hailo_round_robin->condition_vars.clear();
    G_OBJECT_CLASS(parent_class)->dispose(object);
}

static GstPad *
gst_hailo_round_robin_request_new_pad(GstElement *element, GstPadTemplate *templ,
                                      const gchar *name, const GstCaps *caps)
{
    /*
        * This function is called when a new sink pad is requested.
        * for each request we create a new pad and return it.
        * we add a new mutex and a new conditon variable.
    */

    GstPad *sinkpad;
    GstHailoRoundRobin *hailo_round_robin = GST_HAILO_ROUND_ROBIN_CAST(element);

    GST_DEBUG_OBJECT(element, "requesting pad");

    sinkpad = GST_PAD_CAST(g_object_new(GST_TYPE_HAILO_ROUND_ROBIN_PAD,
                                        "name", name, "direction", templ->direction, "template", templ,
                                        NULL));

    // Set sink_chain and sink_event funtions for the new pad
    gst_pad_set_chain_function(sinkpad,
                               GST_DEBUG_FUNCPTR(gst_hailo_round_robin_sink_chain));
    gst_pad_set_event_function(sinkpad,
                               GST_DEBUG_FUNCPTR(gst_hailo_round_robin_sink_event));

    GST_OBJECT_FLAG_SET(sinkpad, GST_PAD_FLAG_PROXY_CAPS);
    GST_OBJECT_FLAG_SET(sinkpad, GST_PAD_FLAG_PROXY_ALLOCATION);

    hailo_round_robin->mutexes.emplace_back(std::make_unique<std::mutex>());
    hailo_round_robin->condition_vars.emplace_back(std::make_unique<std::condition_variable>());

    gst_pad_set_active(sinkpad, TRUE);

    gst_element_add_pad(element, sinkpad);

    GST_DEBUG_OBJECT(element, "requested pad %s:%s",
                     GST_DEBUG_PAD_NAME(sinkpad));

    return sinkpad;
}

static gboolean
gst_hailo_round_robin_all_sinkpads_eos_unlocked(GstHailoRoundRobin *hailo_round_robin)
{
    GstElement *element = GST_ELEMENT_CAST(hailo_round_robin);
    GList *item;
    gboolean all_eos = FALSE;

    // When there are no sinkpads we return FALSE because there is no need to send EOS down the pipeline, pipeline is still active.
    if (element->numsinkpads == 0)
        goto done;

    // Iterate over all the sinkpads of the element, and check if they are in eos state
    for (item = element->sinkpads; item != NULL; item = g_list_next(item))
    {
        GstHailoRoundRobinPad *sinkpad = GST_HAILO_ROUND_ROBIN_PAD_CAST(item->data);

        if (!sinkpad->got_eos)
        {
            return FALSE;
        }
    }

    all_eos = TRUE;

done:
    return all_eos;
}

static void
gst_hailo_round_robin_release_pad(GstElement *element, GstPad *pad)
{
    GstHailoRoundRobin *hailo_round_robin = GST_HAILO_ROUND_ROBIN_CAST(element);
    GST_DEBUG_OBJECT(hailo_round_robin, "releasing pad %s:%s", GST_DEBUG_PAD_NAME(pad));
    gst_pad_set_active(pad, FALSE);

    if (hailo_round_robin->condition_vars[get_pad_num(pad)] != NULL)
        hailo_round_robin->condition_vars[get_pad_num(pad)]->notify_all();
    gst_element_remove_pad(GST_ELEMENT_CAST(hailo_round_robin), pad);
}

static gboolean
forward_events(GstPad *pad, GstEvent **event, gpointer user_data)
{
    // This function pushes forward all events that are not EOS.
    GstPad *srcpad = GST_PAD_CAST(user_data);

    if (GST_EVENT_TYPE(*event) != GST_EVENT_EOS)
        gst_pad_push_event(srcpad, gst_event_ref(*event));

    return TRUE;
}


static GstFlowReturn
gst_hailo_round_robin_sink_chain(GstPad *pad, GstObject *parent, GstBuffer *buf)
{
    GstFlowReturn ret = GST_FLOW_ERROR;
    GstHailoRoundRobin *hailo_round_robin = GST_HAILO_ROUND_ROBIN_CAST(parent);

    size_t pad_num = get_pad_num(pad);
    if (!hailo_round_robin->funnel_mode && hailo_round_robin->current_pad_num != pad_num)
    {
        // Wait for the turn of this pad.
        if (hailo_round_robin->condition_vars[pad_num] != NULL)
        {
            std::unique_lock lock(*hailo_round_robin->mutexes[pad_num].get());
            hailo_round_robin->condition_vars[pad_num]->wait(lock);
        }
        else {
            gst_buffer_unref(buf);
            return ret;
        }

        // If condition variable got notified but it is not this pad's turn, raise an error.
        if (hailo_round_robin->current_pad_num != pad_num)
        {
            GST_ERROR_OBJECT(hailo_round_robin,
                             "Tried to send buf on pad %zu while current pad should be %zu, dropping buffer!",
                             pad_num, hailo_round_robin->current_pad_num);
            gst_buffer_unref(buf);
            return ret;
        }
    }

    buf = gst_buffer_make_writable(buf);

    gchar *pad_name = gst_pad_get_name(pad);
    gchar *stream_id = gst_pad_get_stream_id(pad);

    // Add stream meta to the buffer including the pad name and stream id.
    gst_buffer_add_hailo_stream_meta(buf, pad_name, stream_id);

    // Forward sticky events.
    gst_pad_sticky_events_foreach(pad, forward_events, hailo_round_robin->srcpad);

    // Push out_buffer forward.
    ret = gst_pad_push(hailo_round_robin->srcpad, buf);

    g_free(pad_name);
    g_free(stream_id);

    if (!hailo_round_robin->funnel_mode)
    {
        // Update current pad num.
        hailo_round_robin->current_pad_num++;
        if (hailo_round_robin->current_pad_num == hailo_round_robin->mutexes.size())
            hailo_round_robin->current_pad_num = 0;

        while(hailo_round_robin->condition_vars[hailo_round_robin->current_pad_num] == NULL){
            hailo_round_robin->current_pad_num++;
            if (hailo_round_robin->current_pad_num == hailo_round_robin->mutexes.size())
                hailo_round_robin->current_pad_num = 0;
        }
        hailo_round_robin->condition_vars[hailo_round_robin->current_pad_num]->notify_one();
    }

    return ret;
}

static gboolean
gst_hailo_round_robin_sink_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
    // Handle sink event comming from the sink pad
    GstHailoRoundRobin *hailo_round_robin = GST_HAILO_ROUND_ROBIN_CAST(parent);
    GstHailoRoundRobinPad *fpad = GST_HAILO_ROUND_ROBIN_PAD_CAST(pad);
    gboolean forward = TRUE;
    gboolean res = TRUE;
    gboolean unlock = FALSE;
    size_t pad_num = get_pad_num(pad);

    GST_DEBUG_OBJECT(pad, "received event %" GST_PTR_FORMAT, event);

    if (GST_EVENT_IS_STICKY(event))
    {
        unlock = TRUE;
        GST_PAD_STREAM_LOCK(hailo_round_robin->srcpad);

        if (GST_EVENT_TYPE(event) == GST_EVENT_EOS)
        {
            // EOS event received, we need to forward it to all sink pads and set got_eos to TRUE
            GST_OBJECT_LOCK(hailo_round_robin);
            fpad->got_eos = TRUE;

            hailo_round_robin->condition_vars[pad_num]->notify_all();
            hailo_round_robin->condition_vars[pad_num] = NULL;
            forward = gst_hailo_round_robin_all_sinkpads_eos_unlocked(hailo_round_robin);
            GST_OBJECT_UNLOCK(hailo_round_robin);
        }
        else if (pad_num != hailo_round_robin->current_pad_num)
        {
            forward = FALSE;
        }
    }
    else if (GST_EVENT_TYPE(event) == GST_EVENT_FLUSH_STOP)
    {
        unlock = TRUE;
        GST_PAD_STREAM_LOCK(hailo_round_robin->srcpad);
        GST_OBJECT_LOCK(hailo_round_robin);
        fpad->got_eos = FALSE;
        GST_OBJECT_UNLOCK(hailo_round_robin);
    }

    if (forward && GST_EVENT_IS_SERIALIZED(event))
    {
        /* If no data is coming and we receive serialized event, need to forward all sticky events.
         * Otherwise downstream has an inconsistent set of sticky events when
         * handling the new event. */
        if (!unlock)
        {
            unlock = TRUE;
            GST_PAD_STREAM_LOCK(hailo_round_robin->srcpad);
        }

        if (pad_num != hailo_round_robin->current_pad_num)
        {
            gst_pad_sticky_events_foreach(pad, forward_events, hailo_round_robin->srcpad);
        }
    }

    if (forward)
        res = gst_pad_push_event(hailo_round_robin->srcpad, event);
    else
        gst_event_unref(event);

    if (unlock)
        GST_PAD_STREAM_UNLOCK(hailo_round_robin->srcpad);

    return res;
}

static GstStateChangeReturn
gst_hailo_round_robin_change_state(GstElement *element, GstStateChange transition)
{
    GstStateChangeReturn ret;
    GstHailoRoundRobin *hailo_round_robin = GST_HAILO_ROUND_ROBIN_CAST(element);

    switch (transition)
    {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
    {
        for (uint i=0; i < hailo_round_robin->condition_vars.size(); i++)
            if (hailo_round_robin->condition_vars[i] != NULL)
                hailo_round_robin->condition_vars[i]->notify_all();
        break;
    }
    default: 
        break;
    }
    ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);

    return ret;
}
