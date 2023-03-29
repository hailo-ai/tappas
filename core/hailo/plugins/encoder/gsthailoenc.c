#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <assert.h>
#include <string.h>
/* for stats file handling */
#include <stdio.h>
// #include <glib/gstdio.h>
#include <errno.h>
#include "gsthailoenc.h"

GST_DEBUG_CATEGORY_STATIC(gst_hailoencoder_debug);
#define GST_CAT_DEFAULT gst_hailoencoder_debug

enum    
{
  NUM_OF_PROPS,
};

static void gst_hailoencoder_class_init (GstHailoEncClass * klass);
static void gst_hailoencoder_init (GstHailoEnc * hailoenc);
static void gst_hailoencoder_finalize (GObject * object);

static gboolean gst_hailoencoder_start (GstVideoEncoder * encoder);
static gboolean gst_hailoencoder_stop (GstVideoEncoder * encoder);
static GstFlowReturn gst_hailoencoder_finish (GstVideoEncoder * encoder);
static gboolean gst_hailoencoder_set_format (GstVideoEncoder * encoder,
    GstVideoCodecState * state);
static gboolean gst_hailoencoder_propose_allocation (GstVideoEncoder * encoder,
    GstQuery * query);
static gboolean gst_hailoencoder_flush (GstVideoEncoder * encoder);

static GstFlowReturn gst_hailoencoder_handle_frame (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame);

static void gst_hailoencoder_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_hailoencoder_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);


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
    GST_STATIC_CAPS("video/x-h265, "
        // "framerate = (fraction) [0/1, MAX], "
        // "width = (int) [ 16, MAX ], " "height = (int) [ 16, MAX ], "
        "stream-format = (string) byte-stream, "
        "alignment = (string) au, "
        "profile = (string) { main, main-still-picture, "
        "main-intra, main-10, main-10-intra }")
    );

#define _do_init \
    GST_DEBUG_CATEGORY_INIT(gst_hailoencoder_debug, "hailoencoder", 0, "hailoencoder element");
#define gst_hailoencoder_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstHailoEnc, gst_hailoencoder, GST_TYPE_VIDEO_ENCODER, _do_init);

static void
gst_hailoencoder_class_init (GstHailoEncClass * klass)
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
                                        "H265 Encoder",
                                        "Encoder/Video",
                                        "Encodes raw video into H265 format",
                                        "hailo.ai <contact@hailo.ai>");

  gobject_class->set_property = gst_hailoencoder_set_property;
  gobject_class->get_property = gst_hailoencoder_get_property;

  venc_class->start = gst_hailoencoder_start;
  venc_class->stop = gst_hailoencoder_stop;
  venc_class->finish = gst_hailoencoder_finish;
  venc_class->handle_frame = gst_hailoencoder_handle_frame;
  venc_class->set_format = gst_hailoencoder_set_format;
  venc_class->propose_allocation = gst_hailoencoder_propose_allocation;
  venc_class->flush = gst_hailoencoder_flush;
  
  gobject_class->finalize = gst_hailoencoder_finalize;

}

static void
gst_hailoencoder_init (GstHailoEnc * hailoenc)
{
  //GstHailoEncClass *klass =
  //    (GstHailoEncClass *) G_OBJECT_GET_CLASS (hailoenc);
  EncoderParams *enc_params = &(hailoenc->enc_params);
  GST_PAD_SET_ACCEPT_TEMPLATE (GST_VIDEO_ENCODER_SINK_PAD (hailoenc));
  hailoenc->apiVer = VCEncGetApiVersion();
  hailoenc->encBuild = VCEncGetBuild();
  memset(enc_params, 0, sizeof(EncoderParams));
  SetDefaultParameters(enc_params);
  if(HW_ID_MAJOR_NUMBER(hailoenc->encBuild.hwBuild)<= 1)
  {
    //H2V1 only support 1.
    enc_params->gopSize = 1;
  }
  memset(hailoenc->gopPicCfg, 0, sizeof(hailoenc->gopPicCfg));
  enc_params->encIn.gopConfig.pGopPicCfg = hailoenc->gopPicCfg;
  hailoenc->opened = FALSE;
  hailoenc->frameCntTotal = 0;
}

static void
gst_hailoencoder_finalize (GObject * object)
{
  GstHailoEnc *hailoenc = (GstHailoEnc *) object;
  GST_DEBUG_OBJECT(hailoenc, "hailoencoder finalize callback");

  /* clean up remaining allocated data */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void update_params(GstHailoEnc *hailoenc, 
                          GstVideoInfo * info)
{
  EncoderParams *enc_params = &(hailoenc->enc_params);
  enc_params->width = GST_VIDEO_INFO_WIDTH (info);
  enc_params->height = GST_VIDEO_INFO_HEIGHT (info);

  enc_params->frameRateNumer = GST_VIDEO_INFO_FPS_N (info);
  enc_params->frameRateDenom = GST_VIDEO_INFO_FPS_D (info);

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
}

static gboolean
gst_hailoencoder_set_format (GstVideoEncoder * encoder,
    GstVideoCodecState * state)
{
  // GstCaps *other_caps;
  GstCaps *allowed_caps;
  GstCaps *icaps;
  GstVideoCodecState *output_format;
  GstHailoEnc *hailoenc = (GstHailoEnc *) encoder;
  EncoderParams *enc_params = &(hailoenc->enc_params);
  VCEncIn * pEncIn = &(enc_params->encIn);

  hailoenc->need_reopen = FALSE;
  update_params(hailoenc, &(state->info));
  if (hailoenc->hevc_encoder == NULL)
  {
    /* Encoder initialization */
    if (OpenEncoder(&(hailoenc->hevc_encoder), enc_params) != 0)
    {
      return FALSE;
    }

    /* Allocate input and output buffers */
    if (AllocRes(hailoenc->hevc_encoder, enc_params) != 0)
    {
      FreeRes(hailoenc->hevc_encoder, enc_params);
      CloseEncoder(hailoenc->hevc_encoder);
      // return -VCENC_MEMORY_ERROR;
      return FALSE;
    }
  }

  pEncIn->timeIncrement = 0;
  pEncIn->busOutBuf = enc_params->outbufMem.busAddress;
  pEncIn->outBufSize = enc_params->outbufMem.size;
  pEncIn->pOutBuf = enc_params->outbufMem.virtualAddress;
  

  /* close old session */
  if (hailoenc->opened) {
  }

  /* additional codec settings */

  // GST_DEBUG_OBJECT (hailoenc, "Extracting common video information");
  /* fetch pix_fmt, fps, par, width, height... */

  /* sanitize time base */
  //if (hailoenc->context->time_base.num <= 0
  //    || hailoenc->context->time_base.den <= 0)
  //  goto insane_timebase;


  //pix_fmt = hailoenc->context->pix_fmt;

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

  /* open codec */

  /* is the colourspace correct? */
  //if (pix_fmt != hailoenc->context->pix_fmt) {
  //  gst_caps_unref (allowed_caps);
  //  goto pix_fmt_err;
  //}

  // other_caps = gst_pad_get_allowed_caps (GST_VIDEO_ENCODER_SINK_PAD (encoder));
  /* try to set this caps on the other side */
  // if (!other_caps) {
  //   gst_caps_unref (allowed_caps);
  //   goto unsupported_codec;
  // }
  // GST_WARNING_OBJECT (hailoenc, "Other caps %" GST_PTR_FORMAT, allowed_caps);

  // icaps = gst_caps_intersect (allowed_caps, other_caps);
  // gst_caps_unref (allowed_caps);
  // gst_caps_unref (other_caps);
  // if (gst_caps_is_empty (icaps)) {
  //   gst_caps_unref (icaps);
  //   goto unsupported_codec;
  // }
  icaps = gst_caps_fixate (allowed_caps);
  

  //GST_DEBUG_OBJECT (hailoenc, "codec flags 0x%08x", hailoenc->context->flags);

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
    //const gchar *codec;

    //gst_tag_list_add (tags, GST_TAG_MERGE_REPLACE, GST_TAG_NOMINAL_BITRATE,
    //    (guint) hailoenc->context->bit_rate, NULL);

    gst_video_encoder_merge_tags (encoder, tags, GST_TAG_MERGE_REPLACE);
    gst_tag_list_unref (tags);
  }
  gint max_delayed_frames = 5;
  GstClockTime latency;
  latency = gst_util_uint64_scale_ceil (GST_SECOND * 1, max_delayed_frames, 25);
  gst_video_encoder_set_latency (GST_VIDEO_ENCODER (encoder), latency, latency);

  /* success! */
  hailoenc->opened = TRUE;

  return TRUE;

  /* ERRORS */
// open_file_err:
//   {
//     GST_ELEMENT_ERROR (hailoenc, RESOURCE, OPEN_WRITE,
//         (("Could not open file \"%s\" for writing."), hailoenc->filename),
//         GST_ERROR_SYSTEM);
//     return FALSE;
//   }
// file_read_err:
//   {
//     GST_ELEMENT_ERROR (hailoenc, RESOURCE, READ,
//         (("Could not get contents of file \"%s\"."), hailoenc->filename),
//         GST_ERROR_SYSTEM);
//     return FALSE;
//   }

//insane_timebase:
//  {
//    GST_ERROR_OBJECT (hailoenc, "Rejecting time base %d/%d",
//        hailoenc->context->time_base.den, hailoenc->context->time_base.num);
//    goto cleanup_stats_in;
//  }
// unsupported_codec:
//   {
//     GST_ERROR ("Unsupported codec - no caps found");
//     return TRUE;
//   }
// open_codec_fail:
//   {
//     //GST_DEBUG_OBJECT (hailoenc, "avenc_%s: Failed to open libav codec",
//     //    oclass->in_plugin->name);
//     goto close_codec;
//   }

// pix_fmt_err:
//   {
//     //GST_DEBUG_OBJECT (hailoenc,
//     //    "avenc_%s: AV wants different colourspace (%d given, %d wanted)",
//     //    oclass->in_plugin->name, pix_fmt, hailoenc->context->pix_fmt);
//     goto close_codec;
//   }

// bad_input_fmt:
//   {
//     //GST_DEBUG_OBJECT (hailoenc, "avenc_%s: Failed to determine input format",
//     //    oclass->in_plugin->name);
//     goto close_codec;
//   }
// close_codec:
//   {
//     goto cleanup_stats_in;
//   }
// cleanup_stats_in:
//   {
//     //g_free (hailoenc->context->stats_in);
//     return FALSE;
//   }
}


static gboolean
gst_hailoencoder_propose_allocation (GstVideoEncoder * encoder,
    GstQuery * query)
{
  GstHailoEnc *hailoenc = (GstHailoEnc *) encoder;
  GST_DEBUG_OBJECT(hailoenc, "hailoencoder propose allocation callback");

  gst_query_add_allocation_meta (query, GST_VIDEO_META_API_TYPE, NULL);

  return GST_VIDEO_ENCODER_CLASS (parent_class)->propose_allocation (encoder,
      query);
}

static GstFlowReturn
gst_hailoencoder_receive_packet (GstHailoEnc * hailoenc, u32 *address, u32 size, gboolean send)
{
  GstBuffer *outbuf;
  GstVideoCodecFrame *frame;

  /* Get oldest frame */
  frame = gst_video_encoder_get_oldest_frame (GST_VIDEO_ENCODER(hailoenc));
  if (send) {
    outbuf = gst_buffer_new_memdup(address, size);
    frame->output_buffer = outbuf;
  
//     if (pkt->flags & AV_PKT_FLAG_KEY)
//       GST_VIDEO_CODEC_FRAME_SET_SYNC_POINT (frame);
//     else
//       GST_VIDEO_CODEC_FRAME_UNSET_SYNC_POINT (frame);
  }

// //   frame->dts =
// //       gst_ffmpeg_time_ff_to_gst (pkt->dts, hailoenc->context->time_base);
// //   /* This will lose some precision compared to setting the PTS from the input
// //    * buffer directly, but that way we're sure PTS and DTS are consistent, in
// //    * particular DTS should always be <= PTS
// //    */
// //   frame->pts =
// //       gst_ffmpeg_time_ff_to_gst (pkt->pts, hailoenc->context->time_base);

  return gst_video_encoder_finish_frame (GST_VIDEO_ENCODER (hailoenc), frame);
}

static GstFlowReturn
gst_hailoencoder_first(GstVideoEncoder * encoder)
{
  GstHailoEnc *hailoenc = (GstHailoEnc *) encoder;
  GstVideoCodecFrame *frame;
  EncoderParams *enc_params = &(hailoenc->enc_params);
  VCEncIn *pEncIn = &(enc_params->encIn);
  VCEncOut *pEncOut = &(enc_params->encOut);
  GstBuffer *outbuf;

  if (VCEncStrmStart(hailoenc->hevc_encoder, pEncIn, pEncOut) != VCENC_OK)
  {
    return GST_FLOW_ERROR;
  }
  frame = gst_video_encoder_get_oldest_frame (GST_VIDEO_ENCODER(hailoenc));
  outbuf = gst_buffer_new_memdup(enc_params->outbufMem.virtualAddress, pEncOut->streamSize);

  enc_params->total_bits += pEncOut->streamSize * 8;
  enc_params->streamSize += pEncOut->streamSize;

  pEncIn->poc = 0;

  //default gop size as IPPP
  pEncIn->gopSize =  enc_params->nextGopSize = ((enc_params->gopSize == 0) ? 1 : enc_params->gopSize);
  pEncIn->codingType = VCENC_INTRA_FRAME;
  frame->output_buffer = outbuf;

  return gst_video_encoder_finish_subframe(encoder, frame);
}

/*    Callback function called by the encoder SW after "slice ready"
    interrupt from HW. Note that this function is not necessarily called
    after every slice i.e. it is possible that two or more slices are
    completed between callbacks. 
------------------------------------------------------------------------------*/
void gst_hailoencoder_slice_ready(VCEncSliceReady *slice)
{
  u32 i;
  u32 streamSize;
  u8 *strmPtr;
  GstHailoEnc *hailoenc = (GstHailoEnc *) slice->pAppData;
  EncoderParams *enc_params = &(hailoenc->enc_params);
  /* Here is possible to implement low-latency streaming by
   * sending the complete slices before the whole frame is completed. */
   
  if(enc_params->multislice_encoding&&(ENCH2_SLICE_READY_INTERRUPT))
  {
    if (slice->slicesReadyPrev == 0)    /* New frame */
    {
        strmPtr = (u8 *)slice->pOutBuf; /* Pointer to beginning of frame */
        streamSize=0;
        for(i=0;i<slice->nalUnitInfoNum+slice->slicesReady;i++)
        {
          streamSize+=*(slice->sliceSizes+i);
        }
        gst_hailoencoder_receive_packet(hailoenc, (u32 *)strmPtr, streamSize, TRUE);
    }
    else
    {
      strmPtr = (u8 *)enc_params->strmPtr; /* Here we store the slice pointer */
      streamSize=0;
      for(i=(slice->nalUnitInfoNum+slice->slicesReadyPrev);i<slice->slicesReady+slice->nalUnitInfoNum;i++)
      {
        streamSize+=*(slice->sliceSizes+i);
      }
      gst_hailoencoder_receive_packet(hailoenc, (u32 *)strmPtr, streamSize, TRUE);
    }
    strmPtr+=streamSize;
    /* Store the slice pointer for next callback */
    enc_params->strmPtr = strmPtr;
  }
}

static GstFlowReturn
gst_hailoencoder_handle_frame (GstVideoEncoder * encoder,
    GstVideoCodecFrame * frame)
{
  GstHailoEnc *hailoenc = (GstHailoEnc *) encoder;
  GstFlowReturn ret = GST_FLOW_ERROR;
  VCEncRet enc_ret;
  EncoderParams *enc_params = &(hailoenc->enc_params);
  VCEncIn * pEncIn = &(enc_params->encIn);
  VCEncOut * pEncOut = &(enc_params->encOut);
  GstVideoFrame vframe;

  if (hailoenc->first)
  {
    hailoenc->first = false;
    ret = gst_hailoencoder_first(encoder);
    if (ret != GST_FLOW_OK)
    {
      GST_ERROR_OBJECT(hailoenc, "First buffer failed %d\n", ret);
      return ret;
    }
    if (SetupInputBuffer(enc_params, pEncIn) == 0)
    {
      GST_ERROR_OBJECT(hailoenc, "Setup input buffer failed\n");
      return GST_FLOW_ERROR;
    }
  }
  hailoenc->frameCntTotal++;

  enc_params->multislice_encoding=0;
  enc_params->strmPtr = NULL;
  gst_video_frame_map (&vframe, &(hailoenc->input_state->info), frame->input_buffer, GST_MAP_READ);
  guint8 * luma = GST_VIDEO_FRAME_PLANE_DATA (&vframe, 0);
  guint8 * chroma = GST_VIDEO_FRAME_PLANE_DATA (&vframe, 1);
  guint src_offset, dst_offset;
  for (int i = 0; i < enc_params->height; i++)
  {
    src_offset = i * GST_VIDEO_FRAME_PLANE_STRIDE(&vframe, 0);
    dst_offset = i * enc_params->width;
    memcpy(enc_params->lum + dst_offset, luma + src_offset, enc_params->width);
  }
  for (int i = 0; i < enc_params->height/2; i++)
  {
    src_offset = i * GST_VIDEO_FRAME_PLANE_STRIDE(&vframe, 1);
    dst_offset = i * enc_params->width;
    memcpy(enc_params->cb + dst_offset, chroma + src_offset, enc_params->width);
  }

  enc_ret = EncodeFrame(enc_params, hailoenc->hevc_encoder, &gst_hailoencoder_slice_ready, hailoenc);
  switch (enc_ret)
  {
    case VCENC_FRAME_READY:
      enc_params->picture_enc_cnt++;
      if (enc_params->encOut.streamSize == 0)
      {
        enc_params->picture_cnt++;
        break;
      }
      else
      {
        if((enc_params->multislice_encoding==0)||(ENCH2_SLICE_READY_INTERRUPT==0))
        {
          gst_hailoencoder_receive_packet (hailoenc, enc_params->outbufMem.virtualAddress, 
                                           pEncOut->streamSize, TRUE);
        }
        UpdateEncoder(enc_params, hailoenc->hevc_encoder);
      }
      break;
    case VCENC_OUTPUT_BUFFER_OVERFLOW:
      enc_params->picture_cnt++;
      break;
    default:
      return ret;
      break;
  }
  ret = GST_FLOW_OK;
  return ret;
}


static void
gst_hailoencoder_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstHailoEnc *hailoenc = (GstHailoEnc *) (object);

  if (hailoenc->opened) {
    GST_WARNING_OBJECT (hailoenc,
        "Can't change properties once decoder is setup !");
    return;
  }

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_hailoencoder_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  // GstHailoEnc *hailoenc = (GstHailoEnc *) (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_hailoencoder_flush (GstVideoEncoder * encoder)
{
  return TRUE;
}

static gboolean
gst_hailoencoder_start (GstVideoEncoder * encoder)
{
  GstHailoEnc *hailoenc = (GstHailoEnc *) encoder;
  EncoderParams *enc_params = &(hailoenc->enc_params);

  if (VCEncInitGopConfigs (enc_params->gopSize, NULL, &(enc_params->encIn.gopConfig), enc_params->gopCfgOffset) != 0)
  {
    return FALSE;
  }

  hailoenc->first = TRUE;

  gst_video_encoder_set_min_pts (encoder, GST_SECOND * 60 * 60 * 1000);

  return TRUE;
}

static gboolean
gst_hailoencoder_stop (GstVideoEncoder * encoder)
{
  GstHailoEnc *hailoenc = (GstHailoEnc *) encoder;
  EncoderParams *enc_params = &(hailoenc->enc_params);
  FreeRes(hailoenc->hevc_encoder, enc_params);
  CloseEncoder(hailoenc->hevc_encoder);

  return TRUE;
}

static GstFlowReturn
gst_hailoencoder_finish (GstVideoEncoder * encoder)
{
  GstFlowReturn ret = GST_FLOW_OK;
  GstHailoEnc *hailoenc = (GstHailoEnc *) encoder;
  EncoderParams *enc_params = &(hailoenc->enc_params);
  VCEncOut *pEncOut = &(enc_params->encOut);
  VCEncIn *pEncIn = &(enc_params->encIn);
  VCEncRet enc_ret;
  GstBuffer *outbuf;

  /* End stream */
  enc_ret = VCEncStrmEnd(hailoenc->hevc_encoder, pEncIn, pEncOut);
  if (enc_ret == VCENC_OK)
  {
    outbuf = gst_buffer_new_memdup(enc_params->outbufMem.virtualAddress, pEncOut->streamSize);
    gst_pad_push(encoder->srcpad, outbuf);
    enc_params->streamSize += pEncOut->streamSize;
  }

  return ret;
}
static gboolean
plugin_init(GstPlugin *plugin)
{
    gst_element_register(plugin, "hailoencoder", GST_RANK_PRIMARY, GST_TYPE_HAILO_ENCODER);
    return TRUE;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR, hailoencoder, "hailo encoder plugin", plugin_init,
                  VERSION, "unknown", PACKAGE, "https://hailo.ai/")
