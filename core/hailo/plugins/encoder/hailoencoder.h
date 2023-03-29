#pragma once
#include "enc_common.h"
#include "gopconfig.h"

#define DEFAULT_BIT_DEPTH 10 // 10 bit depth

void SetDefaultParameters(EncoderParams *enc_params);
int OpenEncoder(VCEncInst *encoder, EncoderParams *enc_params);
void CloseEncoder(VCEncInst encoder);
int AllocRes(VCEncInst enc, EncoderParams *enc_params);
void FreeRes(VCEncInst enc, EncoderParams *enc_params);
u32 SetupInputBuffer(EncoderParams *enc_params, VCEncIn *pEncIn);
void UpdateEncoder(EncoderParams *enc_params, 
                    VCEncInst encoder);
VCEncRet EncodeFrame(EncoderParams *enc_params, VCEncInst encoder,
                      VCEncSliceReadyCallBackFunc sliceReadyCbFunc,
                      void *pAppData);