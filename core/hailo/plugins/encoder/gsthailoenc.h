#pragma once

#include <gst/gst.h>
#include <gst/video/video.h>
#include "hailoencoder.h"

G_BEGIN_DECLS

typedef struct _GstHailoEnc GstHailoEnc;
typedef struct _GstHailoEncClass GstHailoEncClass;
#define GST_TYPE_HAILO_ENCODER (gst_hailoencoder_get_type())
#define GST_HAILO_ENCODER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_ENCODER, GstHailoEnc))
#define GST_HAILO_ENCODER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_ENCODER, GstHailoEncClass))
#define GST_IS_HAILO_ENCODER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_ENCODER))
#define GST_IS_HAILO_ENCODER_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_ENCODER))
#define GST_HAILO_ENCODER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_HAILO_ENCODER, GstHailoEncClass))

struct _GstHailoEnc
{
  GstVideoEncoder parent;
  gchar* filename;
  GstVideoCodecState *input_state;
  VCEncApiVersion apiVer;
  VCEncBuild encBuild;
  VCEncGopPicConfig gopPicCfg[MAX_GOP_PIC_CONFIG_NUM];
  guint frameCntTotal;
  EncoderParams enc_params;
  VCEncInst hevc_encoder;
  gboolean opened;
  gboolean first;
  gboolean need_reopen;

};

struct _GstHailoEncClass
{
  GstVideoEncoderClass parent_class;
};

G_GNUC_INTERNAL GType gst_hailoencoder_get_type(void);

G_END_DECLS
