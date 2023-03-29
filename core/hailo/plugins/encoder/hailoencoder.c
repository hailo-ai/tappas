#include "hailoencoder.h"
#include "error.h"
#define DEFAULT -255

static void AdaptiveGOPSizeDecision(EncoderParams *enc_params, 
                                 VCEncInst encoder,
                                 VCEncIn *pEncIn);
static void GetAlignedPicSizebyFormat(VCEncPictureType type, u32 width, u32 height, u32 alignment,
                                       u32 *luma_Size, u32 *chroma_Size, u32 *picture_Size);

void SetDefaultParameters(EncoderParams *enc_params)
{
  enc_params->frameRateNumer      = DEFAULT;
  enc_params->frameRateDenom      = DEFAULT;

  enc_params->width      = DEFAULT;
  enc_params->height     = DEFAULT;
  enc_params->inputFormat = VCENC_YUV420_SEMIPLANAR;
  enc_params->profile = VCENC_HEVC_MAIN_10_PROFILE;
  enc_params->level = VCENC_HEVC_LEVEL_5_2;
  enc_params->streamType = VCENC_BYTE_STREAM;

  enc_params->max_cu_size  = 64;
  enc_params->min_cu_size  = 8;
  enc_params->max_tr_size  = 16;
  enc_params->min_tr_size  = 4;
  enc_params->tr_depth_intra = 2;  //mfu =>0
  enc_params->tr_depth_inter = (enc_params->max_cu_size == 64) ? 4 : 3;
  enc_params->intraPicRate   = 0;  // only first is IDR.
  enc_params->codecH264 = 0;
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

  cfg.codecH264 = 0;
  cfg.profile = enc_params->profile;
  cfg.level = enc_params->level;

  cfg.bitDepthLuma = DEFAULT_BIT_DEPTH;
  cfg.bitDepthChroma = DEFAULT_BIT_DEPTH;

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
  cfg.compressor = 3;
  cfg.enableOutputCuInfo = 0;
  cfg.exp_of_alignment = 0;
  cfg.refAlignmentExp = 0;
  cfg.AXIAlignment = 0;
  cfg.AXIreadOutstandingNum = ENCH2_ASIC_AXI_READ_OUTSTANDING_NUM;
  cfg.AXIwriteOutstandingNum = ENCH2_ASIC_AXI_WRITE_OUTSTANDING_NUM;
  if ((ret = VCEncInit(&cfg, pEnc)) != VCENC_OK)
  {
    //PrintErrorValue("VCEncInit() failed.", ret);
    return (int)ret;
  }
  return 0;
}


int InitEncoderCodingConfig(VCEncInst *pEnc)
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
  codingCfg.roi1Area.enable = 0;
  codingCfg.roi1Area.top = codingCfg.roi1Area.left = 
                            codingCfg.roi1Area.bottom = 
                            codingCfg.roi1Area.right = 0;
  codingCfg.roi2Area.enable = 0;
  codingCfg.roi2Area.top = codingCfg.roi2Area.left = 
                            codingCfg.roi2Area.bottom = 
                            codingCfg.roi2Area.right = 0;
  codingCfg.roi1DeltaQp = 0;
  codingCfg.roi2DeltaQp = 0;
  codingCfg.codecH264 = 0;
            
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

  rcCfg.qpHdr = 26;
  rcCfg.qpMin = 0;
  rcCfg.qpMax = 51;
  rcCfg.pictureSkip = 0;
  rcCfg.pictureRc =0;
  rcCfg.ctbRc = 0;

  rcCfg.blockRCSize = 0;
  rcCfg.bitPerSecond = 60000000;
  rcCfg.bitVarRangeI = 2000;
  rcCfg.bitVarRangeP = 2000;
  rcCfg.bitVarRangeB = 2000;
  rcCfg.tolMovingBitRate = 2000;
  
  rcCfg.monitorFrames = (enc_params->frameRateNumer+enc_params->frameRateDenom-1) / enc_params->frameRateDenom;
  rcCfg.hrd = 0;
  rcCfg.hrdCpbSize = 10000000;

  // rcCfg.gopLen = 0; // default is good.
  rcCfg.intraQpDelta = -5;
  rcCfg.fixedIntraQp = 0;

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
  if (InitEncoderCodingConfig(encoder) != 0)
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



u32 SetupInputBuffer(EncoderParams *enc_params, VCEncIn *pEncIn)
{
  u32 src_img_size;
  u32 bitsPerPixel = VCEncGetBitsPerPixel(enc_params->inputFormat);
  ASSERT(bitsPerPixel != 0);
  
  u32 size_lum = 0;
  u32 size_ch  = 0;

  GetAlignedPicSizebyFormat(enc_params->inputFormat, enc_params->width, enc_params->height, enc_params->alignment, &size_lum, &size_ch, NULL);

  pEncIn->busLuma = enc_params->pictureMem.busAddress;
  enc_params->lum = (u8 *)enc_params->pictureMem.virtualAddress;
  pEncIn->busChromaU = pEncIn->busLuma + size_lum;
  enc_params->cb = enc_params->lum + (u32)size_lum;
  pEncIn->busChromaV = pEncIn->busChromaU + (u32)size_ch/2;
  enc_params->cr = enc_params->cb + (u32)size_ch/2;
  src_img_size = enc_params->width * enc_params->height * bitsPerPixel / 8;
  
  return src_img_size;
}

static u32 get_aligned_width(int width, i32 input_format)
{
  return (width + 15) & (~15);
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
int AllocRes(VCEncInst enc, EncoderParams *enc_params)
{
  i32 ret;
  u32 pictureSize;
  u32 outbufSize;
  u32 lumaSize,chromaSize;

  if (enc_params->alignment == 0)
    pictureSize = get_aligned_width(enc_params->width, enc_params->inputFormat) * enc_params->height *
                  VCEncGetBitsPerPixel(enc_params->inputFormat) / 8;
  else
  {
    if (enc_params->inputFormat==VCENC_YUV420_SEMIPLANAR||enc_params->inputFormat==VCENC_YUV420_SEMIPLANAR_VU)
    {
      lumaSize = STRIDE(enc_params->width,enc_params->alignment)* enc_params->height;
      chromaSize = STRIDE(enc_params->width,enc_params->alignment)* enc_params->height/2;
      pictureSize = lumaSize + chromaSize;
    }
    else if (enc_params->inputFormat==VCENC_YUV422_INTERLEAVED_YUYV||enc_params->inputFormat==VCENC_YUV422_INTERLEAVED_UYVY)
    {
      pictureSize = STRIDE(enc_params->width*2,enc_params->alignment)* enc_params->height;
    }else{
      pictureSize = 0; //This situation won't happen
    }
  }

  GetAlignedPicSizebyFormat(enc_params->inputFormat,enc_params->width,enc_params->height,
                             enc_params->alignment,&lumaSize,&chromaSize,&pictureSize);
  
  enc_params->pictureMem.virtualAddress = NULL;
  enc_params->outbufMem.virtualAddress = NULL;

  /* Here we use the EWL instance directly from the encoder
   * because it is the easiest way to allocate the linear memories */
  ret = EWLMallocLinear(((struct vcenc_instance *)enc)->asic.ewl, pictureSize,enc_params->alignment,
                        &enc_params->pictureMem);
  if (ret != EWL_OK)
  {
    enc_params->pictureMem.virtualAddress = NULL;
    return 1;
  }

  /* Limited amount of memory on some test environment */
  outbufSize = 4 * enc_params->pictureMem.size < ((u32)enc_params->outBufSizeMax * 1024 * 1024) ?
               4 * enc_params->pictureMem.size : ((u32)enc_params->outBufSizeMax * 1024 * 1024);

  ret = EWLMallocLinear(((struct vcenc_instance *)enc)->asic.ewl, outbufSize,enc_params->alignment,
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
void FreeRes(VCEncInst enc, EncoderParams *enc_params)
{
  if (enc_params->pictureMem.virtualAddress != NULL)
    EWLFreeLinear(((struct vcenc_instance *)enc)->asic.ewl, &enc_params->pictureMem);
  if (enc_params->outbufMem.virtualAddress != NULL)
    EWLFreeLinear(((struct vcenc_instance *)enc)->asic.ewl, &enc_params->outbufMem);
}


VCEncRet EncodeFrame(EncoderParams *enc_params, VCEncInst encoder,
                      VCEncSliceReadyCallBackFunc sliceReadyCbFunc,
                      void *pAppData)
{
  VCEncIn *pEncIn = &(enc_params->encIn);
  VCEncOut *pEncOut = &(enc_params->encOut);

  pEncIn->codingType = (pEncIn->poc == 0) ? VCENC_INTRA_FRAME : enc_params->nextCodingType;
  enc_params->last_idr_picture_cnt = enc_params->picture_cnt;

  return VCEncStrmEncode(encoder, pEncIn, pEncOut, sliceReadyCbFunc, pAppData);    
}

void UpdateEncoder(EncoderParams *enc_params, 
                    VCEncInst encoder)
{
  VCEncIn *pEncIn = &(enc_params->encIn);
  VCEncOut *pEncOut = &(enc_params->encOut);
  pEncIn->timeIncrement = enc_params->frameRateDenom;
  enc_params->total_bits += pEncOut->streamSize * 8;
  enc_params->streamSize += pEncOut->streamSize;
  enc_params->validencodedframenumber++;
  //Adaptive GOP size decision
  if ((enc_params->gopSize == 0) && pEncIn->codingType != VCENC_INTRA_FRAME)
  {
    AdaptiveGOPSizeDecision(enc_params, encoder, pEncIn);
  }
  enc_params->nextCodingType = find_next_pic(enc_params);
}

/*------------------------------------------------------------------------------

    Static Functions

------------------------------------------------------------------------------*/

static void AdaptiveGOPSizeDecision(EncoderParams *enc_params, 
                                 VCEncInst encoder,
                                 VCEncIn *pEncIn)
{
  struct vcenc_instance *vcenc_instance = (struct vcenc_instance *)encoder;
  unsigned int uiIntraCu8Num = vcenc_instance->asic.regs.intraCu8Num;
  unsigned int uiSkipCu8Num = vcenc_instance->asic.regs.skipCu8Num;
  unsigned int uiPBFrameCost = vcenc_instance->asic.regs.PBFrame4NRdCost;
  double dIntraVsInterskip = (double)uiIntraCu8Num/(double)((enc_params->width/8) * (enc_params->height/8));
  double dSkipVsInterskip = (double)uiSkipCu8Num/(double)((enc_params->width/8) * (enc_params->height/8));          

  enc_params->gop_frm_num++;
  enc_params->sum_intra_vs_interskip += dIntraVsInterskip;
  enc_params->sum_skip_vs_interskip += dSkipVsInterskip;
  enc_params->sum_costP += (pEncIn->codingType == VCENC_PREDICTED_FRAME)? uiPBFrameCost:0;
  enc_params->sum_costB += (pEncIn->codingType == VCENC_BIDIR_PREDICTED_FRAME)? uiPBFrameCost:0;
  enc_params->sum_intra_vs_interskipP += (pEncIn->codingType == VCENC_PREDICTED_FRAME)? dIntraVsInterskip:0;
  enc_params->sum_intra_vs_interskipB += (pEncIn->codingType == VCENC_BIDIR_PREDICTED_FRAME)? dIntraVsInterskip:0; 
  if(pEncIn->gopPicIdx == pEncIn->gopSize-1)//last frame of the current gop. decide the gopsize of next gop.
  {            
    dIntraVsInterskip = enc_params->sum_intra_vs_interskip/enc_params->gop_frm_num;
    dSkipVsInterskip = enc_params->sum_skip_vs_interskip/enc_params->gop_frm_num;
    enc_params->sum_costB = (enc_params->gop_frm_num>1)?(enc_params->sum_costB/(enc_params->gop_frm_num-1)):0xFFFFFFF;
    enc_params->sum_intra_vs_interskipB = (enc_params->gop_frm_num>1)?(enc_params->sum_intra_vs_interskipB/(enc_params->gop_frm_num-1)):0xFFFFFFF;
    //Enabled adaptive GOP size for large resolution
    if (((enc_params->width * enc_params->height) >= (1280 * 720)) || ((MAX_ADAPTIVE_GOP_SIZE >3)&&((enc_params->width * enc_params->height) >= (416 * 240))))
    {
        if ((((double)enc_params->sum_costP/(double)enc_params->sum_costB)<1.1)&&(dSkipVsInterskip >= 0.95))
        {
            enc_params->last_gopsize = enc_params->nextGopSize = 1;
        }
        else if (((double)enc_params->sum_costP/(double)enc_params->sum_costB)>5)
        {
            enc_params->nextGopSize = enc_params->last_gopsize;
        }
        else
        {
            if( ((enc_params->sum_intra_vs_interskipP > 0.40) && (enc_params->sum_intra_vs_interskipP < 0.70)&& (enc_params->sum_intra_vs_interskipB < 0.10)) )
            {
                enc_params->last_gopsize++;
                if(enc_params->last_gopsize==5 || enc_params->last_gopsize==7)  
                {
                    enc_params->last_gopsize++;
                }
                enc_params->last_gopsize = MIN(enc_params->last_gopsize, MAX_ADAPTIVE_GOP_SIZE);
                enc_params->nextGopSize = enc_params->last_gopsize; //
            }
            else if (dIntraVsInterskip >= 0.30)
            {
                enc_params->last_gopsize = enc_params->nextGopSize = 1; //No B
            }
            else if (dIntraVsInterskip >= 0.20)
            {
                enc_params->last_gopsize = enc_params->nextGopSize = 2; //One B
            }
            else if (dIntraVsInterskip >= 0.10)
            {
                enc_params->last_gopsize--;
                if(enc_params->last_gopsize == 5 || enc_params->last_gopsize==7) 
                {
                    enc_params->last_gopsize--;
                }
                enc_params->last_gopsize = MAX(enc_params->last_gopsize, 3);
                enc_params->nextGopSize = enc_params->last_gopsize; //
            }
            else
            {
                enc_params->last_gopsize++;
                if(enc_params->last_gopsize==5 || enc_params->last_gopsize==7)  
                {
                    enc_params->last_gopsize++;
                }
                enc_params->last_gopsize = MIN(enc_params->last_gopsize, MAX_ADAPTIVE_GOP_SIZE);
                enc_params->nextGopSize = enc_params->last_gopsize; //
            }
        }
    }
    else
    {
      enc_params->nextGopSize = 3;
    }
    enc_params->gop_frm_num = 0;
    enc_params->sum_intra_vs_interskip = 0;
    enc_params->sum_skip_vs_interskip = 0;
    enc_params->sum_costP = 0;
    enc_params->sum_costB = 0;
    enc_params->sum_intra_vs_interskipP = 0;
    enc_params->sum_intra_vs_interskipB = 0;

    enc_params->nextGopSize = MIN(enc_params->nextGopSize, MAX_ADAPTIVE_GOP_SIZE);
  }
}

static void GetAlignedPicSizebyFormat(VCEncPictureType type, u32 width, u32 height, u32 alignment,
                                       u32 *luma_Size, u32 *chroma_Size, u32 *picture_Size)
{
  u32 luma_stride = 0, chroma_stride = 0;
  u32 lumaSize = 0, chromaSize = 0, pictureSize = 0;

  VCEncGetAlignedStride(width, type, &luma_stride, &chroma_stride, alignment);
  switch(type)
  {
    case VCENC_YUV420_PLANAR:
     lumaSize = luma_stride * height;
     chromaSize = chroma_stride * height/2*2;
     break;
    case VCENC_YUV420_SEMIPLANAR:
    case VCENC_YUV420_SEMIPLANAR_VU:
     lumaSize = luma_stride * height;
     chromaSize = chroma_stride * height/2;
     break;
    case VCENC_YUV422_INTERLEAVED_YUYV:
    case VCENC_YUV422_INTERLEAVED_UYVY:
    case VCENC_RGB565:
    case VCENC_BGR565:
    case VCENC_RGB555:
    case VCENC_BGR555:
    case VCENC_RGB444:
    case VCENC_BGR444:
    case VCENC_RGB888:
    case VCENC_BGR888:
    case VCENC_RGB101010:
    case VCENC_BGR101010:
     lumaSize = luma_stride * height;
     chromaSize = 0;
     break;
    case VCENC_YUV420_PLANAR_10BIT_I010:
     lumaSize = luma_stride * height;
     chromaSize = chroma_stride * height/2*2;
     break;
    case VCENC_YUV420_PLANAR_10BIT_P010:
     lumaSize = luma_stride * height;
     chromaSize = chroma_stride * height/2;
     break;
    case VCENC_YUV420_PLANAR_10BIT_PACKED_PLANAR:
     lumaSize = luma_stride *10/8 * height;
     chromaSize = chroma_stride *10/8* height/2*2;
     break;
    case VCENC_YUV420_10BIT_PACKED_Y0L2:
     lumaSize = luma_stride *2*2* height/2;
     chromaSize = 0;
     break;
    default:
     printf("not support this format\n");
     chromaSize = lumaSize = 0;
     break;
  }

  pictureSize = lumaSize + chromaSize;
  if (luma_Size != NULL)
    *luma_Size = lumaSize;
  if (chroma_Size != NULL)
    *chroma_Size = chromaSize;
  if (picture_Size != NULL)
    *picture_Size = pictureSize;
}

