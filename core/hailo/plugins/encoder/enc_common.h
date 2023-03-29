
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "video_encoder/base_type.h"
#include "video_encoder/hevcencapi.h"
#include "video_encoder/encasiccontroller.h"
#include "video_encoder/instance.h"
#include "video_encoder/encinputlinebuffer.h"

typedef struct {
  i32 width;
  i32 height;
  VCEncPictureType inputFormat;
  VCEncProfile profile;
  VCEncLevel level;
  VCEncStreamType streamType;
  i32 frameRateNumer;      /* Output frame rate numerator */
  i32 frameRateDenom;      /* Output frame rate denominator */
  i32 picture_cnt;
  i32 picture_enc_cnt;
  i32 idr_interval;
  i32 last_idr_picture_cnt;
  u8 *lum;
  u8 *cb;
  u8 *cr;
  u32 validencodedframenumber;
  u32 alignment;

  i32 max_cu_size;    /* Max coding unit size in pixels */
  i32 min_cu_size;    /* Min coding unit size in pixels */
  i32 max_tr_size;    /* Max transform size in pixels */
  i32 min_tr_size;    /* Min transform size in pixels */
  i32 tr_depth_intra;   /* Max transform hierarchy depth */
  i32 tr_depth_inter;   /* Max transform hierarchy depth */
  i32 intraPicRate;   /* IDR interval */
  u32 outBufSizeMax; /* Max buf size in MB */
  u32 roiMapDeltaQpBlockUnit;
  u32 total_bits;

  /* Moved from global space */

  /* SW/HW shared memories for input/output buffers */
  EWLLinearMem_t pictureMem;
  EWLLinearMem_t outbufMem;

  float sumsquareoferror;
  float averagesquareoferror;
  i32 maxerrorovertarget;
  i32 maxerrorundertarget;
  long numbersquareoferror;

  u32 gopSize;
  VCEncIn encIn;
  VCEncOut encOut;
  bool codecH264;
  u8 gopCfgOffset[MAX_GOP_SIZE + 1];

  inputLineBufferCfg inputCtbLineBuf;

  // Slice data
  u8 *strmPtr;
  u32 multislice_encoding;

  // Adaptive Gop variables
  int gop_frm_num;
  double sum_intra_vs_interskip;
  double sum_skip_vs_interskip;
  double sum_intra_vs_interskipP;
  double sum_intra_vs_interskipB;
  int sum_costP;
  int sum_costB;
  int last_gopsize;
  i32 nextGopSize;
  u32 streamSize;
  VCEncPictureCodingType nextCodingType;

} EncoderParams;