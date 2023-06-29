#pragma once

#include <gst/gst.h>
#include <gst/video/video.h>
#include "hailoencoder.h"

G_BEGIN_DECLS

typedef struct _GstHailoH265Enc GstHailoH265Enc;
typedef struct _GstHailoH265EncClass GstHailoH265EncClass;
#define GST_TYPE_HAILO_H265_ENC (gst_hailoh265enc_get_type())
#define GST_HAILO_H265_ENC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_HAILO_H265_ENC, GstHailoH265Enc))
#define GST_HAILO_H265_ENC_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_HAILO_H265_ENC, GstHailoH265EncClass))
#define GST_IS_HAILO_H265_ENC(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_HAILO_H265_ENC))
#define GST_IS_HAILO_H265_ENC_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_HAILO_H265_ENC))
#define GST_HAILO_H265_ENC_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_HAILO_H265_ENC, GstHailoH265EncClass))

struct _GstHailoH265Enc
{
  GstVideoEncoder parent;
  GstVideoCodecState *input_state;
  GstBuffer *header_buffer;
  VCEncApiVersion apiVer;
  VCEncBuild encBuild;
  VCEncGopPicConfig gopPicCfg[MAX_GOP_PIC_CONFIG_NUM];
  EncoderParams enc_params;
  VCEncInst hevc_encoder;
  gboolean stream_restart;
  gboolean hard_restart;
  gboolean update_config;
  gboolean update_gop_size;
};

struct _GstHailoH265EncClass
{
  GstVideoEncoderClass parent_class;
};

G_GNUC_INTERNAL GType gst_hailoh265enc_get_type(void);

G_END_DECLS
