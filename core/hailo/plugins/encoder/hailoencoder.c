#include "hailoencoder.h"

void SetDefaultParameters(EncoderParams *enc_params, bool codecH264)
{
  enc_params->frameRateNumer      = DEFAULT_UNCHANGED;
  enc_params->frameRateDenom      = DEFAULT_UNCHANGED;

  enc_params->width      = DEFAULT_UNCHANGED;
  enc_params->height     = DEFAULT_UNCHANGED;

  enc_params->codecH264 = codecH264;
  enc_params->inputFormat = DEFAULT_INPUT_FORMAT;
  enc_params->profile =  enc_params->codecH264 ? DEFAULT_H264_PROFILE : DEFAULT_HEVC_PROFILE;
  enc_params->level =  enc_params->codecH264 ? DEFAULT_H264_LEVEL : DEFAULT_HEVC_LEVEL;
  enc_params->streamType = VCENC_BYTE_STREAM;
  enc_params->gopSize = DEFAULT_GOP_SIZE;
  enc_params->gopLength = DEFAULT_GOP_LENGTH;
  enc_params->intraPicRate = DEFAULT_INTRA_PIC_RATE;
  enc_params->qphdr = DEFAULT_QPHDR;
  enc_params->qpmin = DEFAULT_QPMIN;
  enc_params->qpmax = DEFAULT_QPMAX;
  enc_params->intra_qp_delta = DEFAULT_INTRA_QP_DELTA;
  enc_params->fixed_intra_qp = DEFAULT_FIXED_INTRA_QP;
  enc_params->bitrate = DEFAULT_BITRATE;
  enc_params->tolMovingBitRate = DEFAULT_TOL_MOVING_BITRATE;
  enc_params->bitVarRangeI = DEFAULT_BITVAR_RANGE_I;
  enc_params->bitVarRangeP = DEFAULT_BITVAR_RANGE_P;
  enc_params->bitVarRangeB = DEFAULT_BITVAR_RANGE_B;
  enc_params->pictureRc = 1;
  enc_params->ctbRc = 0;
  enc_params->blockRcSize = 0;
  enc_params->pictureSkip = 0;
  enc_params->hrd = 0;
  enc_params->hrdCpbSize = DEFAULT_HRD_CPB_SIZE;
  enc_params->monitorFrames = 0;
  enc_params->roiArea1 = NULL;
  enc_params->roiArea2 = NULL;
  enc_params->compressor = 3;

  if (enc_params->codecH264)
  {
    enc_params->max_cu_size  = 16;
    enc_params->min_cu_size  = 8;
    enc_params->max_tr_size  = 16;
    enc_params->min_tr_size  = 4;
    enc_params->tr_depth_intra = 1;
    enc_params->tr_depth_inter = 2;
  }
  else
  {
    enc_params->max_cu_size  = 64;
    enc_params->min_cu_size  = 8;
    enc_params->max_tr_size  = 16;
    enc_params->min_tr_size  = 4;
    enc_params->tr_depth_intra = 2;
    enc_params->tr_depth_inter = 4;
  }
  enc_params->outBufSizeMax = 12;
  enc_params->roiMapDeltaQpBlockUnit = 0;

}

int InitEncoderConfig(EncoderParams *enc_params, VCEncInst *pEnc)
{
  VCEncRet ret;
  VCEncConfig cfg;

  cfg.width = enc_params->width;
  cfg.height = enc_params->height;
  cfg.frameRateNum = enc_params->frameRateNumer;
  cfg.frameRateDenom = enc_params->frameRateDenom;

  /* intra tools in sps and pps */
  cfg.strongIntraSmoothing = 1;
  cfg.streamType = enc_params->streamType;

  cfg.codecH264 = enc_params->codecH264;
  cfg.profile = enc_params->profile;
  cfg.level = enc_params->level;

  if (cfg.profile == VCENC_HEVC_MAIN_10_PROFILE || cfg.profile == VCENC_H264_HIGH_10_PROFILE)
  {
    cfg.bitDepthLuma = 10;
    cfg.bitDepthChroma = 10;
  }
  else
  {
    cfg.bitDepthLuma = 8;
    cfg.bitDepthChroma = 8;
  }

  //default maxTLayer
  cfg.maxTLayers = 1;

  cfg.interlacedFrame = 0;
  /* Find the max number of reference frame */
  u32 maxRefPics = 0;
  u32 maxTemporalId = 0;
  int idx;
  for (idx = 0; idx < enc_params->encIn.gopConfig.size; idx ++)
  {
    VCEncGopPicConfig *cfg = &(enc_params->encIn.gopConfig.pGopPicCfg[idx]);
    if (cfg->codingType != VCENC_INTRA_FRAME)
    {
      if (maxRefPics < cfg->numRefPics)
        maxRefPics = cfg->numRefPics;

      if (maxTemporalId < cfg->temporalId)
        maxTemporalId = cfg->temporalId;
    }
  }
  cfg.refFrameAmount = maxRefPics + cfg.interlacedFrame + (enc_params->encIn.gopConfig.ltrInterval > 0);
  cfg.maxTLayers = maxTemporalId +1;
  cfg.compressor = enc_params->compressor;
  cfg.enableOutputCuInfo = 0;
  cfg.exp_of_alignment = 0;
  cfg.refAlignmentExp = 0;
  cfg.AXIAlignment = 0;
  cfg.AXIreadOutstandingNum = 64;   //ENCH2_ASIC_AXI_READ_OUTSTANDING_NUM;
  cfg.AXIwriteOutstandingNum = 64;  //ENCH2_ASIC_AXI_WRITE_OUTSTANDING_NUM;
  if ((ret = VCEncInit(&cfg, pEnc)) != VCENC_OK)
  {
    //PrintErrorValue("VCEncInit() failed.", ret);
    return (int)ret;
  }
  return 0;
}

static void UpdateROIArea(EncoderParams *enc_params, VCEncCodingCtrl codingCfg)
{
  if (enc_params->roiArea1)
  {
    codingCfg.roi1Area.enable = 1;
    sscanf(enc_params->roiArea1, 
    "%u:%u:%u:%u:%u", 
    &codingCfg.roi1Area.left,
    &codingCfg.roi1Area.top,
    &codingCfg.roi1Area.right,
    &codingCfg.roi1Area.bottom,
    &codingCfg.roi1DeltaQp);
  }
  if (enc_params->roiArea2)
  {
    codingCfg.roi2Area.enable = 1;
    sscanf(enc_params->roiArea2, 
    "%u:%u:%u:%u:%u", 
    &codingCfg.roi2Area.left,
    &codingCfg.roi2Area.top,
    &codingCfg.roi2Area.right,
    &codingCfg.roi2Area.bottom,
    &codingCfg.roi2DeltaQp);
  }
}

int UpdateEncoderROIArea(EncoderParams *enc_params, VCEncInst *pEnc)
{
  VCEncInst encoder;
  VCEncRet ret;
  VCEncCodingCtrl codingCfg;

  encoder = *pEnc;

  /* Encoder setup: coding control */
  if ((ret = VCEncGetCodingCtrl(encoder, &codingCfg)) != VCENC_OK)
  {
    CloseEncoder(encoder);
    return -1;
  }

  UpdateROIArea(enc_params, codingCfg);

  if ((ret = VCEncSetCodingCtrl(encoder, &codingCfg)) != VCENC_OK)
  {
    CloseEncoder(encoder);
    return -1;
  }
  return 0;
}

int InitEncoderCodingConfig(EncoderParams *enc_params, VCEncInst *pEnc)
{
  VCEncInst encoder;
  VCEncRet ret;
  VCEncCodingCtrl codingCfg;

  encoder = *pEnc;

  /* Encoder setup: coding control */
  if ((ret = VCEncGetCodingCtrl(encoder, &codingCfg)) != VCENC_OK)
  {
    //PrintErrorValue("VCEncGetCodingCtrl() failed.", ret);
    CloseEncoder(encoder);
    return -1;
  }

  codingCfg.sliceSize = 0;
  codingCfg.disableDeblockingFilter = 0;
  codingCfg.tc_Offset = -2;
  codingCfg.beta_Offset = 5;
  codingCfg.enableSao = 1;
  codingCfg.enableDeblockOverride = 0;
  codingCfg.deblockOverride = 0;
  codingCfg.enableCabac = 1;
  codingCfg.cabacInitFlag = 0;
  codingCfg.videoFullRange = 0;
  
  /* Disabled */
  codingCfg.seiMessages = 0;
  codingCfg.gdrDuration = 0;
  codingCfg.fieldOrder = 0;

  codingCfg.cirStart = 0;
  codingCfg.cirInterval = 0;
  
  codingCfg.intraArea.enable = 0;
  codingCfg.pcm_loop_filter_disabled_flag = 0;
  
  codingCfg.intraArea.enable = 0;
  codingCfg.intraArea.top = codingCfg.intraArea.left = 
                            codingCfg.intraArea.bottom = 
                            codingCfg.intraArea.right = -1;
  codingCfg.ipcm1Area.enable = 0;
  codingCfg.ipcm1Area.top = codingCfg.ipcm1Area.left = 
                            codingCfg.ipcm1Area.bottom = 
                            codingCfg.ipcm1Area.right = -1;
  codingCfg.ipcm2Area.enable = 0;
  codingCfg.ipcm2Area.top = codingCfg.ipcm2Area.left = 
                            codingCfg.ipcm2Area.bottom = 
                            codingCfg.ipcm2Area.right = -1;

  codingCfg.ipcmMapEnable = 0;
  codingCfg.pcm_enabled_flag = 0;

  UpdateROIArea(enc_params, codingCfg);

  codingCfg.codecH264 = enc_params->codecH264;
            
  codingCfg.roiMapDeltaQpEnable = 0;
  codingCfg.roiMapDeltaQpBlockUnit = 0;

  codingCfg.enableScalingList = 0;
  codingCfg.chroma_qp_offset = 0;

  /* low latency */
  codingCfg.inputLineBufEn = 0;
  codingCfg.inputLineBufLoopBackEn = 0;
  codingCfg.inputLineBufDepth = 0;
  codingCfg.inputLineBufHwModeEn = 0;
  codingCfg.inputLineBufCbFunc = VCEncInputLineBufDone;
  codingCfg.inputLineBufCbData = NULL;

  /* denoise */
  codingCfg.noiseReductionEnable = 0;
  codingCfg.noiseLow = 10;        
  codingCfg.firstFrameSigma = 11;

  if ((ret = VCEncSetCodingCtrl(encoder, &codingCfg)) != VCENC_OK)
  {
    //PrintErrorValue("VCEncSetCodingCtrl() failed.", ret);
    CloseEncoder(encoder);
    return -1;
  }
  return 0;
}

int InitEncoderRateConfig(EncoderParams *enc_params, VCEncInst *pEnc)
{
  VCEncInst encoder;
  VCEncRet ret;
  VCEncRateCtrl rcCfg;

  encoder = *pEnc;

  /* Encoder setup: rate control */
  if ((ret = VCEncGetRateCtrl(encoder, &rcCfg)) != VCENC_OK)
  {
    //PrintErrorValue("VCEncGetRateCtrl() failed.", ret);
    CloseEncoder(encoder);
    return -1;
  }

  rcCfg.qpHdr = enc_params->qphdr;
  rcCfg.qpMin = enc_params->qpmin;
  rcCfg.qpMax = enc_params->qpmax;
  rcCfg.pictureSkip = enc_params->pictureSkip;
  rcCfg.pictureRc = enc_params->pictureRc;
  rcCfg.ctbRc = enc_params->ctbRc;

  rcCfg.blockRCSize = enc_params->blockRcSize;

  rcCfg.bitPerSecond = enc_params->bitrate;
  rcCfg.bitVarRangeI = enc_params->bitVarRangeI;
  rcCfg.bitVarRangeP = enc_params->bitVarRangeP;
  rcCfg.bitVarRangeB = enc_params->bitVarRangeB;
  rcCfg.tolMovingBitRate = enc_params->tolMovingBitRate;
  
  if (enc_params->monitorFrames != 0)
    rcCfg.monitorFrames = enc_params->monitorFrames;
  else
    rcCfg.monitorFrames = (enc_params->frameRateNumer+enc_params->frameRateDenom-1) / enc_params->frameRateDenom;

  if(rcCfg.monitorFrames>MAX_MONITOR_FRAMES)
    rcCfg.monitorFrames=MAX_MONITOR_FRAMES;
  if(rcCfg.monitorFrames<MIN_MONITOR_FRAMES)
    rcCfg.monitorFrames=MIN_MONITOR_FRAMES;

  rcCfg.hrd = enc_params->hrd;
  rcCfg.hrdCpbSize = enc_params->hrdCpbSize;

  rcCfg.gopLen = enc_params->gopLength;
  rcCfg.intraQpDelta = enc_params->intra_qp_delta;
  rcCfg.fixedIntraQp = enc_params->fixed_intra_qp;

  if ((ret = VCEncSetRateCtrl(encoder, &rcCfg)) != VCENC_OK)
  {
    //PrintErrorValue("VCEncSetRateCtrl() failed.", ret);
    CloseEncoder(encoder);
    return -1;
  }
  return 0;
}

int InitEncoderPreProcConfig(EncoderParams *enc_params, VCEncInst *pEnc)
{
  VCEncInst encoder;
  VCEncRet ret;
  VCEncPreProcessingCfg preProcCfg;

  encoder = *pEnc;

  /* PreP setup */
  if ((ret = VCEncGetPreProcessing(encoder, &preProcCfg)) != VCENC_OK)
  {
    //PrintErrorValue("VCEncGetPreProcessing() failed.", ret);
    CloseEncoder(encoder);
    return -1;
  }
  preProcCfg.inputType = (VCEncPictureType)enc_params->inputFormat;
  // No Rotation
  preProcCfg.rotation = (VCEncPictureRotation)0;

  // TODO change width and height
  preProcCfg.origWidth = enc_params->width;
  preProcCfg.origHeight = enc_params->height;

  preProcCfg.xOffset = 0;
  preProcCfg.yOffset = 0;
  preProcCfg.colorConversion.type = (VCEncColorConversionType)0;
  
  // For the future RGB to YUV
  if (preProcCfg.colorConversion.type == VCENC_RGBTOYUV_USER_DEFINED)
  {
    preProcCfg.colorConversion.coeffA = 20000;
    preProcCfg.colorConversion.coeffB = 44000;
    preProcCfg.colorConversion.coeffC = 5000;
    preProcCfg.colorConversion.coeffE = 35000;
    preProcCfg.colorConversion.coeffF = 38000;
  }

  preProcCfg.scaledWidth = 0;
  preProcCfg.scaledHeight = 0;

  preProcCfg.busAddressScaledBuff = 0;
  preProcCfg.virtualAddressScaledBuff = NULL;
  preProcCfg.sizeScaledBuff = 0;
  preProcCfg.alignment = 0;

  /* Set overlay area*/
  for(int i = 0; i < MAX_OVERLAY_NUM; i++)
  {
    preProcCfg.overlayArea[i].xoffset = 0;
    preProcCfg.overlayArea[i].cropXoffset = 0;
    preProcCfg.overlayArea[i].yoffset = 0;
    preProcCfg.overlayArea[i].cropYoffset = 0;
    preProcCfg.overlayArea[i].width = 0;
    preProcCfg.overlayArea[i].cropWidth = 0;
    preProcCfg.overlayArea[i].height =0;
    preProcCfg.overlayArea[i].cropHeight = 0;
    preProcCfg.overlayArea[i].format = 0;
    preProcCfg.overlayArea[i].alpha = 0;
    preProcCfg.overlayArea[i].enable = 0;
    preProcCfg.overlayArea[i].Ystride = 0;
    preProcCfg.overlayArea[i].UVstride = 0;
    preProcCfg.overlayArea[i].bitmapY = 0;
    preProcCfg.overlayArea[i].bitmapU = 0;
    preProcCfg.overlayArea[i].bitmapV = 0;
  }

  if ((ret = VCEncSetPreProcessing(encoder, &preProcCfg)) != VCENC_OK)
  {
    //PrintErrorValue("VCEncSetPreProcessing() failed.", ret);
    CloseEncoder(encoder);
    return -1;
  }
  return 0;
}


/*------------------------------------------------------------------------------

    OpenEncoder
        Create and configure an encoder instance.

    Params:
        encoder    - place where to save the new encoder instance
        enc_params         - Arguments
    Return:
        0   - for success
        -1  - error

------------------------------------------------------------------------------*/
int OpenEncoder(VCEncInst *encoder, EncoderParams *enc_params)
{
  if (InitEncoderConfig(enc_params, encoder) != 0)
    return -1;
  
  return UpdateEncoderConfig(encoder, enc_params);
}

/*------------------------------------------------------------------------------

    UpdateEncoderConfig
        Configure an encoder instance.

    Params:
        encoder    - place where to save the new encoder instance
        enc_params         - Arguments
    Return:
        0   - for success
        -1  - error

------------------------------------------------------------------------------*/
int UpdateEncoderConfig(VCEncInst *encoder, EncoderParams *enc_params)
{
  enc_params->idr_interval = enc_params->intraPicRate;

  if (InitEncoderCodingConfig(enc_params, encoder) != 0)
    return -1;
  if (InitEncoderRateConfig(enc_params, encoder) != 0)
    return -1;
  if ( InitEncoderPreProcConfig(enc_params, encoder) != 0)
    return -1;

  return 0;
}


/*------------------------------------------------------------------------------

    CloseEncoder
       Release an encoder insatnce.

   Params:
        encoder - the instance to be released
------------------------------------------------------------------------------*/
void CloseEncoder(VCEncInst encoder)
{
  VCEncRet ret;

  if ((ret = VCEncRelease(encoder)) != VCENC_OK)
  {
    //PrintErrorValue("VCEncRelease() failed.", ret);
  }
}



/*------------------------------------------------------------------------------

    AllocRes

    Allocation of the physical memories used by both SW and HW:
    the input pictures and the output stream buffer.

    NOTE! The implementation uses the EWL instance from the encoder
          for OS independence. This is not recommended in final environment
          because the encoder will release the EWL instance in case of error.
          Instead, the memories should be allocated from the OS the same way
          as inside EWLMallocLinear().

------------------------------------------------------------------------------*/
int AllocRes(EncoderParams *enc_params)
{
  i32 ret;
  u32 outbufSize;
  EWLInitParam_t ewl_params;
  ewl_params.clientType = EWL_CLIENT_TYPE_HEVC_ENC;
  enc_params->ewl = (void *)EWLInit(&ewl_params);
  if (NULL == enc_params->ewl)
  {
    return 1;
  }
 
  /* Limited amount of memory on some test environment */
  outbufSize =  ((u32)enc_params->outBufSizeMax * 1024 * 1024);

  ret = EWLMallocLinear((const void *)enc_params->ewl, outbufSize,enc_params->alignment,
                        &enc_params->outbufMem);
  if (ret != EWL_OK)
  {
    enc_params->outbufMem.virtualAddress = NULL;
    return 1;
  }
  
  return 0;
}

/*------------------------------------------------------------------------------

    FreeRes

    Release all resources allcoated byt AllocRes()

------------------------------------------------------------------------------*/
void FreeRes(EncoderParams *enc_params)
{
  if (enc_params->outbufMem.virtualAddress != NULL)
    EWLFreeLinear((const void *)enc_params->ewl, &enc_params->outbufMem);
  if (NULL != enc_params->ewl)
    (void)EWLRelease((const void *)enc_params->ewl);
}


VCEncRet EncodeFrame(EncoderParams *enc_params, VCEncInst encoder,
                      VCEncSliceReadyCallBackFunc sliceReadyCbFunc,
                      void *pAppData)
{
  VCEncIn *pEncIn = &(enc_params->encIn);
  VCEncOut *pEncOut = &(enc_params->encOut);

  pEncIn->codingType = (pEncIn->poc == 0) ? VCENC_INTRA_FRAME : enc_params->nextCodingType;
  if (pEncIn->codingType == VCENC_INTRA_FRAME)
  {
    pEncIn->poc = 0;
    enc_params->last_idr_picture_cnt = enc_params->picture_cnt;
  }

  return VCEncStrmEncode(encoder, pEncIn, pEncOut, sliceReadyCbFunc, pAppData);    
}

void ForceKeyframe(EncoderParams *enc_params, VCEncInst encoder)
{
  VCEncIn *pEncIn = &(enc_params->encIn);
  pEncIn->codingType = VCENC_INTRA_FRAME;
  pEncIn->poc = 0;
  enc_params->last_idr_picture_cnt = enc_params->picture_cnt;
}

void UpdateEncoderGOP(EncoderParams *enc_params, VCEncInst encoder)
{
  VCEncIn *pEncIn = &(enc_params->encIn);
  pEncIn->timeIncrement = enc_params->frameRateDenom;
  enc_params->validencodedframenumber++;
  enc_params->nextCodingType = find_next_pic(enc_params);
}
