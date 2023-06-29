#pragma once
#include "enc_common.h"
#include "gopconfig.h"

void SetDefaultParameters(EncoderParams *enc_params, bool codecH264);
int InitEncoderRateConfig(EncoderParams *enc_params, VCEncInst *pEnc);
int UpdateEncoderROIArea(EncoderParams *enc_params, VCEncInst *pEnc);
int OpenEncoder(VCEncInst *encoder, EncoderParams *enc_params);
int UpdateEncoderConfig(VCEncInst *encoder, EncoderParams *enc_params);
void CloseEncoder(VCEncInst encoder);
int AllocRes(EncoderParams *enc_params);
void FreeRes(EncoderParams *enc_params);
u32 SetupInputBuffer(EncoderParams *enc_params, VCEncIn *pEncIn);
void UpdateEncoderGOP(EncoderParams *enc_params, 
                    VCEncInst encoder);
VCEncRet EncodeFrame(EncoderParams *enc_params, VCEncInst encoder,
                      VCEncSliceReadyCallBackFunc sliceReadyCbFunc,
                      void *pAppData);
void ForceKeyframe(EncoderParams *enc_params, VCEncInst encoder);