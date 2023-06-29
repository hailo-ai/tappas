#include <assert.h>
#include <string.h>
/* for stats file handling */
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include "gsthailoh264enc.h"


/*******************
Property Definitions
*******************/
enum    
{
  PROP_0,
  PROP_PROFILE,
  PROP_LEVEL,
  PROP_INTRA_PIC_RATE,
  PROP_GOP_SIZE,
  PROP_GOP_LENGTH,
  PROP_QPHDR,
  PROP_QPMIN,
  PROP_QPMAX,
  PROP_INTRA_QP_DELTA,
  PROP_FIXED_INTRA_QP,
  PROP_BFRAME_QP_DELTA,
  PROP_BITRATE,
  PROP_TOL_MOVING_BITRATE,
  PROP_BITRATE_VAR_RANGE_I,
  PROP_BITRATE_VAR_RANGE_P,
  PROP_BITRATE_VAR_RANGE_B,
  PROP_PICTURE_RC,
  PROP_CTB_RC,
  PROP_PICTURE_SKIP,
  PROP_HRD,
  PROP_MONITOR_FRAMES,
  PROP_ROI_AREA_1,
  PROP_ROI_AREA_2,
  PROP_COMPRESSOR,
  PROP_BLOCK_RC_SIZE,
  PROP_HRD_CPB_SIZE,
  NUM_OF_PROPS,
};

#define GST_TYPE_HAILOH264ENC_PROFILE (gst_hailoh264enc_profile_get_type())
static GType
gst_hailoh264enc_profile_get_type(void)
{
    static GType hailoh264enc_profile_type = 0;
    static const GEnumValue hailoh264enc_profiles[] = {
        {VCENC_H264_BASE_PROFILE, "Base Profile", "base"},
        {VCENC_H264_MAIN_PROFILE, "Main Profile", "main"},
        {VCENC_H264_HIGH_PROFILE, "High Profile", "high"},
        {VCENC_H264_HIGH_10_PROFILE, "High 10 Profile", "high-10"},
        {0, NULL, NULL},
    };
    if (!hailoh264enc_profile_type)
    {
        hailoh264enc_profile_type =
            g_enum_register_static("GstHailoH264EncProfile", hailoh264enc_profiles);
    }
    return hailoh264enc_profile_type;
}

#define GST_TYPE_HAILOH264ENC_LEVEL (gst_hailoh264enc_level_get_type())
static GType
gst_hailoh264enc_level_get_type(void)
{
    static GType hailoh264enc_level_type = 0;
    static const GEnumValue hailoh264enc_levels[] = {
        {VCENC_H264_LEVEL_1, "Level 1", "level-1"},
        {VCENC_H264_LEVEL_1_b, "Level 1b", "level-1-b"},
        {VCENC_H264_LEVEL_1_1, "Level 1.1", "level-1-1"},
        {VCENC_H264_LEVEL_1_2, "Level 1.2", "level-1-2"},
        {VCENC_H264_LEVEL_1_3, "Level 1.3", "level-1-3"},
        {VCENC_H264_LEVEL_2, "Level 2", "level-2"},
        {VCENC_H264_LEVEL_2_1, "Level 2.1", "level-2-1"},
        {VCENC_H264_LEVEL_2_2, "Level 2.2", "level-2-2"},
        {VCENC_H264_LEVEL_3, "Level 3", "level-3"},
        {VCENC_H264_LEVEL_3_1, "Level 3.1", "level-3-1"},
        {VCENC_H264_LEVEL_3_2, "Level 3.2", "level-3-2"},
        {VCENC_H264_LEVEL_4, "Level 4", "level-4"},
        {VCENC_H264_LEVEL_4_1, "Level 4.1", "level-4-1"},
        {VCENC_H264_LEVEL_4_2, "Level 4.2", "level-4-2"},
        {VCENC_H264_LEVEL_5, "Level 5", "level-5"},
        {VCENC_H264_LEVEL_5_1, "Level 5.1", "level-5-1"},
        {VCENC_H264_LEVEL_5_2, "Level 5.2", "level-5-2"},
        {0, NULL, NULL},
    };
    if (!hailoh264enc_level_type)
    {
        hailoh264enc_level_type =
            g_enum_register_static("GstHailoH264EncLevel", hailoh264enc_levels);
    }
    return hailoh264enc_level_type;
}

#define GST_TYPE_HAILOH264ENC_COMPRESSOR (gst_hailoh264enc_compressor_get_type())
static GType
gst_hailoh264enc_compressor_get_type(void)
{
    /*Enable/Disable Embedded Compression
      0 = Disable Compression
      1 = Only Enable Luma Compression
      2 = Only Enable Chroma Compression
      3 = Enable Both Luma and Chroma Compression*/
    static GType hailoh264enc_compressor_type = 0;
    static const GEnumValue hailoh264enc_compressors[] = {
        {0, "Disable Compression", "disable"},
        {1, "Only Enable Luma Compression", "enable-luma"},
        {2, "Only Enable Chroma Compression", "enable-chroma"},
        {3, "Enable Both Luma and Chroma Compression", "enable-both"},
        {0, NULL, NULL},
    };
    if (!hailoh264enc_compressor_type)
    {
        hailoh264enc_compressor_type =
            g_enum_register_static("GstHailoH264EncCompressor", hailoh264enc_compressors);
    }
    return hailoh264enc_compressor_type;
}

#define GST_TYPE_HAILOH264ENC_BLOCK_RC_SIZE (gst_hailoh264enc_block_rc_size_get_type())
static GType
gst_hailoh264enc_block_rc_size_get_type(void)
{
    static GType hailoh264enc_block_rc_size_type = 0;
    static const GEnumValue hailoh264enc_block_rc_size_types[] = {
        {0, "64X64", "64x64"},
        {1, "32X32", "32x32"},
        {2, "16X16", "16x16"},
        {0, NULL, NULL},
    };
    if (!hailoh264enc_block_rc_size_type)
    {
        hailoh264enc_block_rc_size_type =
            g_enum_register_static("GstHailoH264EncBlockRcSize", hailoh264enc_block_rc_size_types);
    }
    return hailoh264enc_block_rc_size_type;
}

/*******************
Function Definitions
*******************/

static void gst_hailoh264enc_class_init(GstHailoH264EncClass * klass);
static void gst_hailoh264enc_init(GstHailoH264Enc * hailoenc);
static void gst_hailoh264enc_set_property(GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_hailoh264enc_get_property(GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_hailoh264enc_finalize(GObject * object);

static gboolean gst_hailoh264enc_set_format(GstVideoEncoder * encoder, GstVideoCodecState * state);
static gboolean gst_hailoh264enc_propose_allocation(GstVideoEncoder * encoder, GstQuery * query);
static gboolean gst_hailoh264enc_flush(GstVideoEncoder * encoder);
static gboolean gst_hailoh264enc_start(GstVideoEncoder * encoder);
static gboolean gst_hailoh264enc_stop(GstVideoEncoder * encoder);
static GstFlowReturn gst_hailoh264enc_finish(GstVideoEncoder * encoder);
static GstFlowReturn gst_hailoh264enc_handle_frame(GstVideoEncoder * encoder, GstVideoCodecFrame * frame);

/************
Pad Templates
************/

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/x-raw, "
        "format=NV12, "
        "width=(int)[16,MAX], "
        "height=(int)[16,MAX], "
        "framerate=(fraction)[0/1,MAX]"));

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/x-h264, "
        "stream-format = (string) byte-stream, "
        "alignment = (string) au, "
        "profile = (string) { base, main, high, high-10 }"));


/*************
Init Functions
*************/

GST_DEBUG_CATEGORY_STATIC(gst_hailoh264enc_debug);
#define GST_CAT_DEFAULT gst_hailoh264enc_debug
#define _do_init \
    GST_DEBUG_CATEGORY_INIT(gst_hailoh264enc_debug, "hailoh264enc", 0, "hailoh264enc element");
#define gst_hailoh264enc_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstHailoH264Enc, gst_hailoh264enc, GST_TYPE_VIDEO_ENCODER, _do_init);

static void
gst_hailoh264enc_class_init(GstHailoH264EncClass * klass)
{
  GObjectClass *gobject_class;
  GstVideoEncoderClass *venc_class;
  GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

  gobject_class = (GObjectClass *) klass;
  venc_class = (GstVideoEncoderClass *) klass;

  //parent_class = g_type_class_peek_parent (klass);
  gst_element_class_add_pad_template (element_class, gst_static_pad_template_get(&sink_template));
  gst_element_class_add_pad_template (element_class, gst_static_pad_template_get(&src_template));
  gst_element_class_set_static_metadata(element_class,
                                        "H264 Encoder",
                                        "Encoder/Video",
                                        "Encodes raw video into H264 format",
                                        "hailo.ai <contact@hailo.ai>");

  gobject_class->set_property = gst_hailoh264enc_set_property;
  gobject_class->get_property = gst_hailoh264enc_get_property;


  g_object_class_install_property(gobject_class, PROP_PROFILE,
                                    g_param_spec_enum("profile", "encoder profile", "profile to encoder",
                                                      GST_TYPE_HAILOH264ENC_PROFILE, (gint)DEFAULT_H264_PROFILE,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property(gobject_class, PROP_LEVEL,
                                    g_param_spec_enum("level", "encoder level", "level to encoder",
                                                      GST_TYPE_HAILOH264ENC_LEVEL, (gint)DEFAULT_H264_LEVEL,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property(gobject_class, PROP_INTRA_PIC_RATE,
                                    g_param_spec_uint("intra-pic-rate", "IDR Interval", "I frames interval (0 - Dynamic IDR Interval)",
                                                      MIN_INTRA_PIC_RATE, MAX_INTRA_PIC_RATE, (guint)DEFAULT_INTRA_PIC_RATE,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_GOP_SIZE,
                                    g_param_spec_uint("gop-size", "GOP Size", "GOP Size (1 - No B Frames)",
                                                      MIN_GOP_SIZE, MAX_GOP_SIZE, (guint)DEFAULT_GOP_SIZE,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_GOP_LENGTH,
                                    g_param_spec_uint("gop-length", "GOP Length", "GOP Length",
                                                      MIN_GOP_LENGTH, MAX_GOP_LENGTH, (guint)DEFAULT_GOP_LENGTH,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_QPHDR,
                                    g_param_spec_int("qp-hdr", "Initial target QP", "Initial target QP, -1 = Encoder calculates initial QP",
                                                      MIN_QPHDR, MAX_QPHDR, (gint)DEFAULT_QPHDR,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_QPMIN,
                                    g_param_spec_uint("qp-min", "QP Min", "Minimum frame header QP",
                                                      MIN_QP_VALUE, MAX_QP_VALUE, (guint)DEFAULT_QPMIN,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_QPMAX,
                                    g_param_spec_uint("qp-max", "QP Max", "Maximum frame header QP",
                                                      MIN_QP_VALUE, MAX_QP_VALUE, (guint)DEFAULT_QPMAX,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_INTRA_QP_DELTA,
                                    g_param_spec_int("intra-qp-delta", "Intra QP delta", "QP difference between target QP and intra frame QP",
                                                      MIN_INTRA_QP_DELTA, MAX_INTRA_QP_DELTA, (gint)DEFAULT_INTRA_QP_DELTA,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_FIXED_INTRA_QP,
                                    g_param_spec_uint("fixed-intra-qp", "Fixed Intra QP", "Use fixed QP value for every intra frame in stream, 0 = disabled",
                                                      MIN_FIXED_INTRA_QP, MAX_FIXED_INTRA_QP, (guint)DEFAULT_FIXED_INTRA_QP,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_BFRAME_QP_DELTA,
                                    g_param_spec_int("bframe-qp-delta", "BFrame QP Delta", "QP difference between BFrame QP and target QP, -1 = Disabled",
                                                      MIN_BFRAME_QP_DELTA, MAX_BFRAME_QP_DELTA, (guint)DEFAULT_BFRAME_QP_DELTA,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_BITRATE,
                                    g_param_spec_uint("bitrate", "Target bitrate", "Target bitrate for rate control in bits/second",
                                                      MIN_BITRATE, MAX_BITRATE, (guint)DEFAULT_BITRATE,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_TOL_MOVING_BITRATE,
                                    g_param_spec_uint("tol-moving-bitrate", "Tolerance moving bitrate", "Percent tolerance over target bitrate of moving bit rate",
                                                      MIN_BITRATE_VARIABLE_RANGE, MAX_BITRATE_VARIABLE_RANGE, (guint)DEFAULT_TOL_MOVING_BITRATE,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_BITRATE_VAR_RANGE_I,
                                    g_param_spec_uint("bitvar-range-i", "Bitrate percent variation I frame", "Percent variations over average bits per frame for I frame",
                                                      MIN_BITRATE_VARIABLE_RANGE, MAX_BITRATE_VARIABLE_RANGE, (guint)DEFAULT_BITVAR_RANGE_I,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_BITRATE_VAR_RANGE_P,
                                    g_param_spec_uint("bitvar-range-p", "Bitrate percent variation P frame", "Percent variations over average bits per frame for P frame",
                                                      MIN_BITRATE_VARIABLE_RANGE, MAX_BITRATE_VARIABLE_RANGE, (guint)DEFAULT_BITVAR_RANGE_P,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_BITRATE_VAR_RANGE_B,
                                    g_param_spec_uint("bitvar-range-b", "Bitrate percent variation B frame", "Percent variations over average bits per frame for B frame",
                                                      MIN_BITRATE_VARIABLE_RANGE, MAX_BITRATE_VARIABLE_RANGE, (guint)DEFAULT_BITVAR_RANGE_B,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_PICTURE_RC,
                                    g_param_spec_boolean("picture-rc", "Picture Rate Control", "Adjust QP between pictures", true,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_CTB_RC,
                                    g_param_spec_boolean("ctb-rc", "Block Rate Control", "Adaptive adjustment of QP inside frame", false,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_PICTURE_SKIP,
                                    g_param_spec_boolean("picture-skip", "Picture Skip", "Allow rate control to skip pictures", false,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_HRD,
                                    g_param_spec_boolean("hrd", "Picture Rate Control", "Restricts the instantaneous bitrate and total bit amount of every coded picture.", false,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_MONITOR_FRAMES,
                                    g_param_spec_uint("monitor-frames", "Monitor Frames", "How many frames will be monitored for moving bit rate. Default is using framerate",
                                                      MIN_MONITOR_FRAMES, MAX_MONITOR_FRAMES, (gint)DEFAULT_MONITOR_FRAMES,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_ROI_AREA_1,
                                    g_param_spec_string("roi-area1", "ROI Area and QP Delta",
                                                        "Specifying rectangular area of CTBs as Region Of Interest with lower QP, left:top:right:bottom:delta_qp format ", NULL,
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_ROI_AREA_2,
                                    g_param_spec_string("roi-area2", "ROI Area and QP Delta",
                                                        "Specifying rectangular area of CTBs as Region Of Interest with lower QP, left:top:right:bottom:delta_qp format ", NULL,
                                                        (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_COMPRESSOR,
                                    g_param_spec_enum("compressor", "Compressor", "Enable/Disable Embedded Compression",
                                                      GST_TYPE_HAILOH264ENC_COMPRESSOR, (guint)3,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property(gobject_class, PROP_BLOCK_RC_SIZE,
                                    g_param_spec_enum("block-rc-size", "Block Rate Control Size", "Size of block rate control",
                                                      GST_TYPE_HAILOH264ENC_BLOCK_RC_SIZE, (guint)0,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
  g_object_class_install_property(gobject_class, PROP_HRD_CPB_SIZE,
                                    g_param_spec_uint("hrd-cpb-size", "HRD Coded Picture Buffer size", "Buffer size used by the HRD model in bits",
                                                      MIN_HRD_CPB_SIZE, MAX_HRD_CPB_SIZE, (guint)DEFAULT_HRD_CPB_SIZE,
                                                      (GParamFlags)(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_PLAYING)));
                              
  venc_class->start = gst_hailoh264enc_start;
  venc_class->stop = gst_hailoh264enc_stop;
  venc_class->finish = gst_hailoh264enc_finish;
  venc_class->handle_frame = gst_hailoh264enc_handle_frame;
  venc_class->set_format = gst_hailoh264enc_set_format;
  venc_class->propose_allocation = gst_hailoh264enc_propose_allocation;
  venc_class->flush = gst_hailoh264enc_flush;
  
  gobject_class->finalize = gst_hailoh264enc_finalize;

}

static void
gst_hailoh264enc_init(GstHailoH264Enc * hailoenc)
{
  //GstHailoH264EncClass *klass =
  //    (GstHailoH264EncClass *) G_OBJECT_GET_CLASS (hailoenc);
  EncoderParams *enc_params = &(hailoenc->enc_params);
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_VIDEO_ENCODER_SINK_PAD (hailoenc));
  hailoenc->apiVer = VCEncGetApiVersion();
  hailoenc->encBuild = VCEncGetBuild();
  hailoenc->stream_restart = FALSE;
  memset(enc_params, 0, sizeof(EncoderParams));
  SetDefaultParameters(enc_params, true);
  if(HW_ID_MAJOR_NUMBER(hailoenc->encBuild.hwBuild)<= 1)
  {
    //H2V1 only support 1.
    enc_params->gopSize = 1;
  }
  memset(hailoenc->gopPicCfg, 0, sizeof(hailoenc->gopPicCfg));
  hailoenc->h264_encoder = NULL;
  enc_params->encIn.gopConfig.pGopPicCfg = hailoenc->gopPicCfg;
}

/************************
GObject Virtual Functions
************************/

static void
gst_hailoh264enc_get_property(GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstHailoH264Enc *hailoenc = (GstHailoH264Enc *) (object);

  switch (prop_id) {
    case PROP_PROFILE:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_enum(value, (gint)hailoenc->enc_params.profile);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_LEVEL:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_enum(value, (gint)hailoenc->enc_params.level);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_INTRA_PIC_RATE:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_uint(value, (guint)hailoenc->enc_params.intraPicRate);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_GOP_SIZE:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_uint(value, (guint)hailoenc->enc_params.gopSize);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_GOP_LENGTH:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_uint(value, (guint)hailoenc->enc_params.gopLength);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_QPHDR:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_int(value, (gint)hailoenc->enc_params.qphdr);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_QPMIN:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_uint(value, (guint)hailoenc->enc_params.qpmin);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_QPMAX:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_uint(value, (guint)hailoenc->enc_params.qpmax);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_INTRA_QP_DELTA:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_int(value, (gint)hailoenc->enc_params.intra_qp_delta);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_FIXED_INTRA_QP:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_uint(value, (guint)hailoenc->enc_params.fixed_intra_qp);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_BFRAME_QP_DELTA:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_int(value, (gint)hailoenc->enc_params.bFrameQpDelta);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_BITRATE:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_uint(value, (guint)hailoenc->enc_params.bitrate);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_TOL_MOVING_BITRATE:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_uint(value, (guint)hailoenc->enc_params.tolMovingBitRate);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_BITRATE_VAR_RANGE_I:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_uint(value, (guint)hailoenc->enc_params.bitVarRangeI);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_BITRATE_VAR_RANGE_P:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_uint(value, (guint)hailoenc->enc_params.bitVarRangeP);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_BITRATE_VAR_RANGE_B:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_uint(value, (guint)hailoenc->enc_params.bitVarRangeB);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_MONITOR_FRAMES:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_uint(value, (guint)hailoenc->enc_params.monitorFrames);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_PICTURE_RC:
        GST_OBJECT_LOCK(hailoenc);
        gboolean picture_rc = hailoenc->enc_params.pictureRc == 1 ? TRUE : FALSE;
        g_value_set_boolean(value, picture_rc);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_CTB_RC:
        GST_OBJECT_LOCK(hailoenc);
        gboolean ctb_rc = hailoenc->enc_params.ctbRc == 1 ? TRUE : FALSE;
        g_value_set_boolean(value, ctb_rc);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_PICTURE_SKIP:
        GST_OBJECT_LOCK(hailoenc);
        gboolean picture_skip = hailoenc->enc_params.pictureSkip == 1 ? TRUE : FALSE;
        g_value_set_boolean(value, picture_skip);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_HRD:
        GST_OBJECT_LOCK(hailoenc);
        gboolean hrd = hailoenc->enc_params.hrd == 1 ? TRUE : FALSE;
        g_value_set_boolean(value, hrd);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_ROI_AREA_1:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_string(value, (const gchar *)hailoenc->enc_params.roiArea1);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_ROI_AREA_2:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_string(value, (const gchar *)hailoenc->enc_params.roiArea2);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_COMPRESSOR:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_enum(value, (gint)hailoenc->enc_params.compressor);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_BLOCK_RC_SIZE:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_enum(value, (gint)hailoenc->enc_params.blockRcSize);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_HRD_CPB_SIZE:
        GST_OBJECT_LOCK(hailoenc);
        g_value_set_uint(value, (guint)hailoenc->enc_params.hrdCpbSize);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_hailoh264enc_set_property(GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstHailoH264Enc *hailoenc = (GstHailoH264Enc *) (object);
  hailoenc->update_config=FALSE;

  switch (prop_id) {
    case PROP_PROFILE:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.profile = (VCEncProfile)g_value_get_enum(value);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_LEVEL:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.level = (VCEncLevel)g_value_get_enum(value);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_INTRA_PIC_RATE:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.intraPicRate = g_value_get_uint(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_GOP_SIZE:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.gopSize = g_value_get_uint(value);
        hailoenc->update_config = TRUE;
        hailoenc->update_gop_size = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_GOP_LENGTH:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.gopLength = g_value_get_uint(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_QPHDR:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.qphdr = g_value_get_int(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_QPMIN:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.qpmin = g_value_get_uint(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_QPMAX:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.qpmax = g_value_get_uint(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_INTRA_QP_DELTA:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.intra_qp_delta = g_value_get_int(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_FIXED_INTRA_QP:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.fixed_intra_qp = g_value_get_uint(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_BFRAME_QP_DELTA:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.bFrameQpDelta = g_value_get_int(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_BITRATE:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.bitrate = g_value_get_uint(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_TOL_MOVING_BITRATE:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.tolMovingBitRate = g_value_get_uint(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_BITRATE_VAR_RANGE_I:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.bitVarRangeI = g_value_get_uint(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_BITRATE_VAR_RANGE_P:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.bitVarRangeP = g_value_get_uint(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_BITRATE_VAR_RANGE_B:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.bitVarRangeB = g_value_get_uint(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_MONITOR_FRAMES:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.monitorFrames = g_value_get_uint(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_PICTURE_RC:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.pictureRc = g_value_get_boolean(value) ? 1 : 0;
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_CTB_RC:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.ctbRc = g_value_get_boolean(value) ? 1 : 0;
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_PICTURE_SKIP:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.pictureSkip = g_value_get_boolean(value) ? 1 : 0;
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_HRD:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.hrd = g_value_get_boolean(value) ? 1 : 0;
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_ROI_AREA_1:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.roiArea1 = (char *)g_value_get_string(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_ROI_AREA_2:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.roiArea2 = (char *)g_value_get_string(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_COMPRESSOR:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.compressor = (u32)g_value_get_enum(value);
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_BLOCK_RC_SIZE:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.blockRcSize = (u32)g_value_get_enum(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    case PROP_HRD_CPB_SIZE:
        GST_OBJECT_LOCK(hailoenc);
        hailoenc->enc_params.hrdCpbSize = g_value_get_uint(value);
        hailoenc->update_config = TRUE;
        GST_OBJECT_UNLOCK(hailoenc);
        break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_hailoh264enc_finalize(GObject * object)
{
  GstHailoH264Enc *hailoenc = (GstHailoH264Enc *) object;
  GST_DEBUG_OBJECT(hailoenc, "hailoh264enc finalize callback");

  /* clean up remaining allocated data */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/*****************
Internal Functions
*****************/


/**
 * Gets the time difference between 2 time specs in milliseconds.
 * @param[in] after     The second time spec.
 * @param[in] before    The first time spec.\
 * @returns The time differnece in milliseconds.
 */
int64_t gst_hailoh264enc_difftimespec_ms(const struct timespec after, const struct timespec before)
{
    return ((int64_t)after.tv_sec - (int64_t)before.tv_sec) * (int64_t)1000
         + ((int64_t)after.tv_nsec - (int64_t)before.tv_nsec) / 1000000;
}

/**
 * Updates the encoder with the input video info.
 *
 * @param[in] hailoenc     The GstHailoH264Enc object.
 * @param[in] info         A GstVideoInfo object containing the input video info for this pipeline.
 * @returns TRUE if the encoder parameters were updated, FALSE otherwise.
 * @note The updated data is the resolution, framerate and input format.
 */
static gboolean 
gst_hailoh264enc_update_params(GstHailoH264Enc *hailoenc, GstVideoInfo * info)
{
 gboolean updated_params = FALSE;
  EncoderParams *enc_params = &(hailoenc->enc_params);

  if (enc_params->width != GST_VIDEO_INFO_WIDTH (info) ||
      enc_params->height != GST_VIDEO_INFO_HEIGHT (info))
  {
    enc_params->width = GST_VIDEO_INFO_WIDTH (info);
    enc_params->height = GST_VIDEO_INFO_HEIGHT (info);
    updated_params = TRUE;
  }

  if (enc_params->frameRateNumer != GST_VIDEO_INFO_FPS_N (info) ||
      enc_params->frameRateDenom != GST_VIDEO_INFO_FPS_D (info))
  {
    enc_params->frameRateNumer = GST_VIDEO_INFO_FPS_N (info);
    enc_params->frameRateDenom = GST_VIDEO_INFO_FPS_D (info);
    updated_params = TRUE;
  }

  switch (GST_VIDEO_INFO_FORMAT(info))
  {
  case GST_VIDEO_FORMAT_NV12:
    enc_params->inputFormat = VCENC_YUV420_SEMIPLANAR;
    break;
  case GST_VIDEO_FORMAT_NV21:
    enc_params->inputFormat = VCENC_YUV420_SEMIPLANAR_VU;
    break;
  case GST_VIDEO_FORMAT_I420:
    enc_params->inputFormat = VCENC_YUV420_PLANAR;
    break;
  default:
    GST_ERROR_OBJECT(hailoenc, "Unsupported format %d", GST_VIDEO_INFO_FORMAT(info));
    break;
  }
  return updated_params;
}

/**
 * Updated the encoder parameters with the physical addresses of the current input buffer.
 *
 * @param[in] hailoenc     The GstHailoH264Enc object.
 * @param[in] frame        The GstVideoCodecFrame object with the input GstBuffer inside.
 * @return GST_FLOW_OK on success, GST_FLOW_ERROR on failure.
 * @note The function will fail when it cannot get the physical address or the memory is non-continous.
 */
static GstFlowReturn gst_hailoh264enc_update_input_buffer(GstHailoH264Enc * hailoenc,
    GstVideoCodecFrame * frame)
{
  EncoderParams *enc_params = &(hailoenc->enc_params);
  GstVideoFrame vframe;
  int ret;

  // Get the Virtal addresses of input buffer luma and chroma.
  gst_video_frame_map(&vframe, &(hailoenc->input_state->info), frame->input_buffer, GST_MAP_READ);
  guint8 * luma = GST_VIDEO_FRAME_PLANE_DATA (&vframe, 0);
  guint8 * chroma = GST_VIDEO_FRAME_PLANE_DATA (&vframe, 1);
  // Get the Size of input buffer luma and chroma.
  size_t luma_size = GST_VIDEO_FRAME_HEIGHT (&vframe) * GST_VIDEO_FRAME_PLANE_STRIDE (&vframe, 0);
  size_t chroma_size = GST_VIDEO_FRAME_HEIGHT (&vframe) * GST_VIDEO_FRAME_PLANE_STRIDE (&vframe, 1) / 2;
  gst_video_frame_unmap(&vframe);
  
  // Get the physical Addresses of input buffer luma and chroma.
  ret = EWLGetBusAddress(enc_params->ewl, (u32*)luma, (u32*)&(enc_params->encIn.busLuma), luma_size);
  if (ret != EWL_OK)
  {
    GST_ERROR_OBJECT(hailoenc, "Could not get physical address of input picture luma");
    return GST_FLOW_ERROR;
  }
  ret = EWLGetBusAddress(enc_params->ewl, (u32*)chroma, (u32*)&(enc_params->encIn.busChromaU), chroma_size);
  if (ret != EWL_OK)
  {
    GST_ERROR_OBJECT(hailoenc, "Could not get physical address of input picture chroma");
    return GST_FLOW_ERROR;
  }
  return GST_FLOW_OK;
}

/**
 * Creats a GstBuffer object with the encoded data as memory.
 *
 * @param[in] hailoenc     The GstHailoH264Enc object.
 * @return A GstBuffer object of the encoded data.
 * @note It also modifies parameters containing the total streamed bytes and bits.
 */
static GstBuffer *gst_hailoh264enc_get_encoded_buffer(GstHailoH264Enc * hailoenc)
{
  GstBuffer *outbuf;
  EncoderParams *enc_params = &(hailoenc->enc_params);
  outbuf = gst_buffer_new_memdup(enc_params->outbufMem.virtualAddress, 
                                              enc_params->encOut.streamSize);
  return outbuf;
}

/**
 * Set the headers of the encoded stream
 *
 * @param[in] encoder     The GstVideoEncoder object.
 */
static void
gst_hailoh264enc_set_headers(GstVideoEncoder * encoder)
{
  GstHailoH264Enc *hailoenc = (GstHailoH264Enc *) encoder;
  GList *headers = NULL;
  headers = g_list_prepend(headers, hailoenc->header_buffer);
  gst_video_encoder_set_headers(encoder, headers);
}

/**
 * Encode and set the header - Performed via VCEncStrmStart
 *
 * @param[in] encoder     The GstVideoEncoder object.
 * @return Upon success, returns VCENC_OK. Otherwise, returns another error value from VCEncRet.
 */
static VCEncRet
gst_hailoh264enc_encode_header(GstVideoEncoder * encoder)
{
  VCEncRet enc_ret;
  GstHailoH264Enc *hailoenc = (GstHailoH264Enc *) encoder;
  EncoderParams *enc_params = &(hailoenc->enc_params);
  VCEncIn *pEncIn = &(enc_params->encIn);
  VCEncOut *pEncOut = &(enc_params->encOut);
  pEncIn->gopSize = enc_params->gopSize;

  if (hailoenc->h264_encoder == NULL)
  {
    GST_ERROR_OBJECT(hailoenc, "Encoder not initialized");
    return VCENC_ERROR;
  }

  enc_ret = VCEncStrmStart(hailoenc->h264_encoder, pEncIn, pEncOut);
  if (enc_ret != VCENC_OK)
  {
    return enc_ret;
  }

  hailoenc->header_buffer = gst_hailoh264enc_get_encoded_buffer(hailoenc);
  gst_hailoh264enc_set_headers(encoder);
  
  // Default gop size as IPPP
  pEncIn->poc = 0;
  pEncIn->gopSize =  enc_params->nextGopSize = ((enc_params->gopSize == 0) ? 1 : enc_params->gopSize);
  pEncIn->codingType = VCENC_INTRA_FRAME;

  return enc_ret;
}


/**
 * Restart the encoder
 *
 * @param[in] encoder     The GstVideoEncoder object.
 * @param[in] frame        A GstVideoCodecFrame used for sending stream_end data.
 * @return Upon success, returns GST_FLOW_OK, GST_FLOW_ERROR on failure.
 */
static GstFlowReturn
gst_hailoh264enc_stream_restart(GstVideoEncoder * encoder, GstVideoCodecFrame * frame)
{
  VCEncRet enc_ret;
  GstHailoH264Enc *hailoenc = (GstHailoH264Enc *) encoder;
  EncoderParams *enc_params = &(hailoenc->enc_params);
  VCEncIn * pEncIn = &(enc_params->encIn);
  VCEncOut * pEncOut = &(enc_params->encOut);
  GST_WARNING_OBJECT(hailoenc, "Restarting encoder");

  if (hailoenc->h264_encoder == NULL)
  {
    GST_ERROR_OBJECT(hailoenc, "Encoder not initialized");
    return GST_FLOW_ERROR;
  }

  enc_ret = VCEncStrmEnd(hailoenc->h264_encoder, pEncIn, pEncOut);
  if (enc_ret != VCENC_OK)
  {
    GST_ERROR_OBJECT(hailoenc, "Encoder restart - Failed to end stream, returned %d", enc_ret);
    return GST_FLOW_ERROR;
  }

  if (enc_params->picture_cnt > 0)
  {
    frame->output_buffer = gst_hailoh264enc_get_encoded_buffer(hailoenc);
    gst_video_encoder_finish_subframe(GST_VIDEO_ENCODER(hailoenc), frame);
  }

  if (hailoenc->hard_restart)
  {
    CloseEncoder(hailoenc->h264_encoder);
  }

  if (hailoenc->update_gop_size)
  {
    GST_DEBUG_OBJECT(hailoenc, "Updating gop size to %u", enc_params->gopSize);
    memset(hailoenc->gopPicCfg, 0, sizeof(hailoenc->gopPicCfg));
    memset(enc_params->gopCfgOffset, 0, sizeof(enc_params->gopCfgOffset));
    memset(&(enc_params->encIn.gopConfig), 0, sizeof(enc_params->encIn.gopConfig));
    enc_params->encIn.gopConfig.pGopPicCfg = hailoenc->gopPicCfg;
    if (VCEncInitGopConfigs(enc_params->gopSize, NULL, &(enc_params->encIn.gopConfig), enc_params->gopCfgOffset, enc_params->bFrameQpDelta, enc_params->codecH264) != 0)
    {
      GST_ERROR_OBJECT(hailoenc, "Encoder restart - Failed to update gop size");
      return GST_FLOW_ERROR;
    }    
    hailoenc->update_gop_size = FALSE;
  }

  if (hailoenc->hard_restart)
  {
    GST_INFO_OBJECT(hailoenc, "Reopening encoder");
    if (OpenEncoder(&(hailoenc->h264_encoder), enc_params) != 0)
    {
      GST_ERROR_OBJECT(hailoenc, "Encoder restart - Failed to open encoder");
      return GST_FLOW_ERROR;
    }
    hailoenc->hard_restart = FALSE;
  }
  else
  {
    if (UpdateEncoderConfig(&(hailoenc->h264_encoder), enc_params) != 0)
    {
      GST_ERROR_OBJECT(hailoenc, "Encoder restart - Failed to update configuration");
      return GST_FLOW_ERROR;
    }
  }

  enc_ret = gst_hailoh264enc_encode_header(encoder);
  if (enc_ret != VCENC_OK)
  {
    GST_ERROR_OBJECT(hailoenc, "Encoder restart - Failed to encode headers, returned %d", enc_ret);
    return GST_FLOW_ERROR;
  }

  hailoenc->stream_restart = FALSE;
  return GST_FLOW_OK;
}

/*
 * Send a slice to the downstream element
 *
 * @param[in] hailoenc     The GstHailoH264Enc object.
 * @param[in] address      The address of the slice.
 * @param[in] size         The size of the slice.
 * @return Upon success, returns GST_FLOW_OK, GST_FLOW_ERROR on failure.
 */
static GstFlowReturn
gst_hailoh264enc_send_slice(GstHailoH264Enc * hailoenc, u32 *address, u32 size)
{
  GstBuffer *outbuf;
  GstVideoCodecFrame *frame;

  /* Get oldest frame */
  frame = gst_video_encoder_get_oldest_frame (GST_VIDEO_ENCODER(hailoenc));
  outbuf = gst_buffer_new_memdup(address, size);
  frame->output_buffer = outbuf;

  return gst_video_encoder_finish_subframe (GST_VIDEO_ENCODER (hailoenc), frame);
}

/*
 * Callback function for slice ready event
 *
 * @param[in] slice     The slice ready event.
 * @return void
 */
static void gst_hailoh264enc_slice_ready(VCEncSliceReady *slice)
{
  u32 i;
  u32 streamSize;
  u8 *strmPtr;
  GstHailoH264Enc *hailoenc = (GstHailoH264Enc *) slice->pAppData;
  EncoderParams *enc_params = &(hailoenc->enc_params);
  /* Here is possible to implement low-latency streaming by
   * sending the complete slices before the whole frame is completed. */
   
  if(enc_params->multislice_encoding)
  {
    if (slice->slicesReadyPrev == 0)    /* New frame */
    {
        strmPtr = (u8 *)slice->pOutBuf; /* Pointer to beginning of frame */
        streamSize=0;
        for(i=0;i<slice->nalUnitInfoNum+slice->slicesReady;i++)
        {
          streamSize+=*(slice->sliceSizes+i);
        }
        gst_hailoh264enc_send_slice(hailoenc, (u32 *)strmPtr, streamSize);
    }
    else
    {
      strmPtr = (u8 *)enc_params->strmPtr; /* Here we store the slice pointer */
      streamSize=0;
      for(i=(slice->nalUnitInfoNum+slice->slicesReadyPrev);i<slice->slicesReady+slice->nalUnitInfoNum;i++)
      {
        streamSize+=*(slice->sliceSizes+i);
      }
      gst_hailoh264enc_send_slice(hailoenc, (u32 *)strmPtr, streamSize);
    }
    strmPtr+=streamSize;
    /* Store the slice pointer for next callback */
    enc_params->strmPtr = strmPtr;
  }
}

/**
 * Encode a single frame
 *
 * @param[in] hailoenc     The GstHailoH264Enc object.
 * @param[in] frame        A GstVideoCodecFrame containing the input to encode.
 * @return Upon success, returns GST_FLOW_OK, GST_FLOW_ERROR on failure.
 */
static GstFlowReturn encode_single_frame(GstHailoH264Enc *hailoenc, GstVideoCodecFrame * frame)
{
  GstFlowReturn ret = GST_FLOW_ERROR;
  VCEncRet enc_ret;
  EncoderParams *enc_params = &(hailoenc->enc_params);
  struct timespec start_encode, end_encode;
  GST_DEBUG_OBJECT(hailoenc, "Encoding frame number %u in type %u", frame->system_frame_number, enc_params->nextCodingType);

  if (hailoenc->h264_encoder == NULL)
  {
    GST_ERROR_OBJECT(hailoenc, "Encoder not initialized");
    return GST_FLOW_ERROR;
  }

  ret = gst_hailoh264enc_update_input_buffer(hailoenc, frame);
  if (ret != GST_FLOW_OK)
  {
    GST_ERROR_OBJECT(hailoenc, "Could not update the input buffer");
    return ret;
  }

  clock_gettime(CLOCK_MONOTONIC,&start_encode);
  enc_ret = EncodeFrame(enc_params, hailoenc->h264_encoder, &gst_hailoh264enc_slice_ready, hailoenc);
  clock_gettime(CLOCK_MONOTONIC,&end_encode);
  GST_DEBUG_OBJECT(hailoenc, "Encode took %lu milliseconds", (long)gst_hailoh264enc_difftimespec_ms(end_encode,start_encode));

  switch (enc_ret)
  {
    case VCENC_FRAME_READY:
      enc_params->picture_enc_cnt++;
      if (enc_params->encOut.streamSize == 0)
      {
        enc_params->picture_cnt++;
        ret = GST_FLOW_OK;
        GST_WARNING_OBJECT(hailoenc, "Encoder didn't return any output for frame %d", enc_params->picture_cnt);
      }
      else
      {
        if(enc_params->multislice_encoding==0)
        {
          // Get the encoded output buffer.
          frame->output_buffer = gst_hailoh264enc_get_encoded_buffer(hailoenc);
          ret = gst_video_encoder_finish_frame(GST_VIDEO_ENCODER (hailoenc), frame);
          if (ret != GST_FLOW_OK)
          {
            GST_ERROR_OBJECT(hailoenc, "Could not send incoded buffer, reason %d", ret);
            return ret;
          }
        }
        UpdateEncoderGOP(enc_params, hailoenc->h264_encoder);
      }
      break;
    default:
      GST_ERROR_OBJECT(hailoenc, "Encoder failed with error %d", enc_ret);
      ret = GST_FLOW_ERROR;
      return ret;
      break;
  }
  return ret;
}

/**
 * Encode multiple frames - encode 1 frame or more according to the GOP config order.
 *
 * @param[in] encoder     The GstVideoEncoder object.
 * @return Upon success, returns GST_FLOW_OK, GST_FLOW_ERROR on failure.
 * @note All the frames that will be encoded are queued in the GstVideoEncoder object and retreived
 *       via the gst_video_encoder_get_frame function.
 */
static GstFlowReturn
gst_hailoh264enc_encode_frames(GstVideoEncoder * encoder)
{
  GstHailoH264Enc *hailoenc = (GstHailoH264Enc *) encoder;
  EncoderParams *enc_params = &(hailoenc->enc_params);
  GstVideoCodecFrame * current_frame;
  GstFlowReturn ret = GST_FLOW_ERROR;
  guint gop_size = enc_params->encIn.gopSize;
  GST_DEBUG_OBJECT(hailoenc, "Encoding %u frames", gop_size);

  if (hailoenc->h264_encoder == NULL)
  {
    GST_ERROR_OBJECT(hailoenc, "Encoder not initialized");
    return GST_FLOW_ERROR;
  }

  if (gop_size <= 0)
  {
    GST_ERROR_OBJECT(hailoenc, "Invalid current GOP size %d", gop_size);
    return GST_FLOW_ERROR;
  }

  // Assuming enc_params->encIn.gopSize is not 0.
  for (int i=0;i<gop_size;i++)
  {
    current_frame = gst_video_encoder_get_frame(encoder, enc_params->picture_cnt);
    if (!current_frame)
    {
      GST_ERROR_OBJECT(hailoenc, "frame %u is missing", enc_params->picture_cnt);
      break;
    }
    ret = encode_single_frame(hailoenc, current_frame);
    gst_video_codec_frame_unref(current_frame);
    if (ret != GST_FLOW_OK)
    {
      GST_ERROR_OBJECT(hailoenc, "Encoding frame %u failed.", enc_params->picture_cnt);
      break;
    }
  }

  if (hailoenc->update_config && enc_params->nextCodingType == VCENC_INTRA_FRAME)
  {
    GST_INFO_OBJECT(hailoenc, "Finished GOP, restarting encoder in order to update config");
    hailoenc->stream_restart = TRUE;
    hailoenc->update_config=FALSE;
  }

  return ret;
}


/********************************
GstVideoEncoder Virtual Functions
********************************/

static gboolean
gst_hailoh264enc_set_format(GstVideoEncoder * encoder, GstVideoCodecState * state)
{
  // GstCaps *other_caps;
  GstCaps *allowed_caps;
  GstCaps *icaps;
  GstVideoCodecState *output_format;
  GstHailoH264Enc *hailoenc = (GstHailoH264Enc *) encoder;
  EncoderParams *enc_params = &(hailoenc->enc_params);

  gboolean updated_caps = gst_hailoh264enc_update_params(hailoenc, &(state->info));
  if (hailoenc->h264_encoder != NULL && updated_caps)
  {
    GST_INFO_OBJECT(hailoenc, "Encoder parameters changed, restarting encoder");
    hailoenc->stream_restart = TRUE;
    hailoenc->hard_restart = TRUE;
  }
  else if (hailoenc->h264_encoder == NULL)
  {
    /* Encoder initialization */
    if (OpenEncoder(&(hailoenc->h264_encoder), enc_params) != 0)
    {
      return FALSE;
    }

    VCEncRet ret = gst_hailoh264enc_encode_header(encoder);
    if (ret != VCENC_OK)
    {
      GST_ERROR_OBJECT(hailoenc, "Failed to encode headers, returned %d", ret);
      return FALSE;
    }
  }

  /* some codecs support more than one format, first auto-choose one */
  GST_DEBUG_OBJECT (hailoenc, "picking an output format ...");
  allowed_caps = gst_pad_get_allowed_caps (GST_VIDEO_ENCODER_SRC_PAD (encoder));
  if (!allowed_caps) {
    GST_DEBUG_OBJECT (hailoenc, "... but no peer, using template caps");
    /* we need to copy because get_allowed_caps returns a ref, and
     * get_pad_template_caps doesn't */
    allowed_caps =
        gst_pad_get_pad_template_caps (GST_VIDEO_ENCODER_SRC_PAD (encoder));
  }
  GST_DEBUG_OBJECT (hailoenc, "chose caps %" GST_PTR_FORMAT, allowed_caps);

  icaps = gst_caps_fixate (allowed_caps);
  
  /* Store input state and set output state */
  if (hailoenc->input_state)
    gst_video_codec_state_unref (hailoenc->input_state);
  hailoenc->input_state = gst_video_codec_state_ref (state);
  GST_DEBUG_OBJECT (hailoenc, "Setting output caps state %" GST_PTR_FORMAT, icaps);

  output_format = gst_video_encoder_set_output_state (encoder, icaps, state);
  
  gst_video_codec_state_unref (output_format);

  /* Store some tags */
  {
    GstTagList *tags = gst_tag_list_new_empty ();
    gst_video_encoder_merge_tags (encoder, tags, GST_TAG_MERGE_REPLACE);
    gst_tag_list_unref (tags);
  }
  gint max_delayed_frames = 5;
  GstClockTime latency;
  latency = gst_util_uint64_scale_ceil (GST_SECOND * 1, max_delayed_frames, 25);
  gst_video_encoder_set_latency (GST_VIDEO_ENCODER (encoder), latency, latency);

  return TRUE;
}

static gboolean
gst_hailoh264enc_propose_allocation(GstVideoEncoder * encoder, GstQuery * query)
{
  GstHailoH264Enc *hailoenc = (GstHailoH264Enc *) encoder;
  GST_DEBUG_OBJECT(hailoenc, "hailoh264enc propose allocation callback");

  gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL);

  return GST_VIDEO_ENCODER_CLASS (parent_class)->propose_allocation (encoder,
      query);
}

static gboolean
gst_hailoh264enc_flush(GstVideoEncoder * encoder)
{
  return TRUE;
}

static gboolean
gst_hailoh264enc_start(GstVideoEncoder * encoder)
{
  GstHailoH264Enc *hailoenc = (GstHailoH264Enc *) encoder;
  EncoderParams *enc_params = &(hailoenc->enc_params);
  VCEncIn * pEncIn = &(enc_params->encIn);

  if (VCEncInitGopConfigs (enc_params->gopSize, NULL, &(enc_params->encIn.gopConfig), enc_params->gopCfgOffset, enc_params->bFrameQpDelta, enc_params->codecH264) != 0)
  {
    return FALSE;
  }

  /* Allocate input and output buffers */
  if (AllocRes(enc_params) != 0)
  {
    FreeRes(enc_params);
    return FALSE;
  }
  pEncIn->timeIncrement = 0;
  pEncIn->busOutBuf = enc_params->outbufMem.busAddress;
  pEncIn->outBufSize = enc_params->outbufMem.size;
  pEncIn->pOutBuf = enc_params->outbufMem.virtualAddress;

  gst_video_encoder_set_min_pts (encoder, GST_SECOND * 60 * 60 * 1000);

  return TRUE;
}

static gboolean
gst_hailoh264enc_stop(GstVideoEncoder * encoder)
{
  GstHailoH264Enc *hailoenc = (GstHailoH264Enc *) encoder;
  EncoderParams *enc_params = &(hailoenc->enc_params);
  CloseEncoder(hailoenc->h264_encoder);
  hailoenc->h264_encoder = NULL;
  FreeRes(enc_params);

  return TRUE;
}

static GstFlowReturn
gst_hailoh264enc_finish(GstVideoEncoder * encoder)
{
  GstHailoH264Enc *hailoenc = (GstHailoH264Enc *) encoder;
  VCEncOut *pEncOut = &(hailoenc->enc_params.encOut);
  VCEncIn *pEncIn = &(hailoenc->enc_params.encIn);
  VCEncRet enc_ret;

  /* End stream */
  enc_ret = VCEncStrmEnd(hailoenc->h264_encoder, pEncIn, pEncOut);
  if (enc_ret != VCENC_OK)
  {
    GST_ERROR_OBJECT(hailoenc, "Failed to end stream, returned %d", enc_ret);
    return GST_FLOW_ERROR;
  }
  return gst_pad_push(encoder->srcpad, gst_hailoh264enc_get_encoded_buffer(hailoenc));
}

static GstFlowReturn
gst_hailoh264enc_handle_frame(GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame)
{
  GstHailoH264Enc *hailoenc = (GstHailoH264Enc *) encoder;
  GstFlowReturn ret = GST_FLOW_ERROR;
  EncoderParams *enc_params = &(hailoenc->enc_params);
  GList *frames;
  guint delayed_frames;
  GstVideoCodecFrame * oldest_frame;
  struct timespec start_handle, end_handle;
  clock_gettime(CLOCK_MONOTONIC,&start_handle);
  GST_DEBUG_OBJECT(hailoenc, "Received frame number %u", frame->system_frame_number);

  if (hailoenc->stream_restart)
  {
    ret = gst_hailoh264enc_stream_restart(encoder, frame);
    if (ret != GST_FLOW_OK)
    {
      GST_ERROR_OBJECT(hailoenc, "Failed to restart encoder");
      return ret;
    }
  }

  // Update Slice Encoding parameters
  enc_params->multislice_encoding=0;
  enc_params->strmPtr = NULL;

  if (GST_VIDEO_CODEC_FRAME_IS_FORCE_KEYFRAME(frame))
  {
    GST_DEBUG_OBJECT(hailoenc, "Forcing keyframe");
    // Adding sync point in order to delete forced keyframe evnet from the queue.
    GST_VIDEO_CODEC_FRAME_SET_SYNC_POINT(frame);
    ForceKeyframe(enc_params, hailoenc->h264_encoder);
    oldest_frame = gst_video_encoder_get_oldest_frame(encoder);
    ret = encode_single_frame(hailoenc, oldest_frame);
    if (ret != GST_FLOW_OK)
    {
      GST_ERROR_OBJECT(hailoenc, "Failed to encode forced keyframe");
      return ret;
    }
    if (frame == oldest_frame)
    {
      gst_video_codec_frame_unref(oldest_frame);
      return ret;
    }
  }

  switch (enc_params->nextCodingType)
  {
    case VCENC_INTRA_FRAME:
      ret = encode_single_frame(hailoenc, frame);
      break;
    case VCENC_PREDICTED_FRAME:
      frames = gst_video_encoder_get_frames (encoder);
      delayed_frames = g_list_length (frames);
      g_list_free_full (frames, (GDestroyNotify) gst_video_codec_frame_unref);
      guint gop_size = enc_params->encIn.gopSize;
      if (delayed_frames == gop_size)
      {
        ret = gst_hailoh264enc_encode_frames(encoder);
      }
      else if (delayed_frames < gop_size)
      {
        ret = GST_FLOW_OK;
      }
      else
      {
        GST_ERROR_OBJECT(hailoenc, "Skipped too many frames");
      }
      break;
    case VCENC_BIDIR_PREDICTED_FRAME:
      GST_ERROR_OBJECT(hailoenc,"Got B frame without pending P frame");
      break;
    default:
      GST_ERROR_OBJECT(hailoenc,"Unknown coding type %d", (int)enc_params->nextCodingType);
      break;
  }
  clock_gettime(CLOCK_MONOTONIC,&end_handle);
  GST_DEBUG_OBJECT(hailoenc, "handle_frame took %lu milliseconds", (long)gst_hailoh264enc_difftimespec_ms(end_handle,start_handle));
  return ret;
}
