
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "video_encoder/base_type.h"
#include "video_encoder/hevcencapi.h"
#include "video_encoder/encinputlinebuffer.h"
#include "video_encoder/ewl.h"

#define MIN_QP_VALUE (0)
#define MAX_QP_VALUE (51)
#define MIN_BITRATE_VARIABLE_RANGE (0)
#define MAX_BITRATE_VARIABLE_RANGE (2000)

#define MIN_BITRATE (10000)
#define MAX_BITRATE (40000000)
#define MIN_HRD_CPB_SIZE (10000)
#define MAX_HRD_CPB_SIZE (40000000)
#define MIN_MONITOR_FRAMES (10)
#define MAX_MONITOR_FRAMES (120)
#define MIN_INTRA_PIC_RATE (0)
#define MAX_INTRA_PIC_RATE (300)
#define MIN_GOP_LENGTH (1)
#define MAX_GOP_LENGTH (300)
#define MIN_GOP_SIZE (1)
// #define MAX_GOP_SIZE (8) - Defined in hevcencapi.h
#define MIN_QPHDR (-1)
#define MAX_QPHDR (MAX_QP_VALUE)
#define MIN_INTRA_QP_DELTA (-MAX_QP_VALUE)
#define MAX_INTRA_QP_DELTA (MAX_QP_VALUE)
#define MIN_FIXED_INTRA_QP (MIN_QP_VALUE)
#define MAX_FIXED_INTRA_QP (MAX_QP_VALUE)
#define MIN_BFRAME_QP_DELTA (-1)
#define MAX_BFRAME_QP_DELTA (MAX_QP_VALUE)


#define DEFAULT_UNCHANGED (-255)
#define DEFAULT_INPUT_FORMAT (VCENC_YUV420_SEMIPLANAR)
#define DEFAULT_HEVC_PROFILE (VCENC_HEVC_MAIN_PROFILE)
#define DEFAULT_HEVC_LEVEL (VCENC_HEVC_LEVEL_5_1)
#define DEFAULT_H264_PROFILE (VCENC_H264_HIGH_10_PROFILE)
#define DEFAULT_H264_LEVEL (VCENC_H264_LEVEL_5_2)
#define DEFAULT_INTRA_PIC_RATE (30)
#define DEFAULT_GOP_LENGTH (30)
#define DEFAULT_GOP_SIZE (MIN_GOP_SIZE)
#define DEFAULT_QPHDR (26)
#define DEFAULT_QPMIN (MIN_QP_VALUE)
#define DEFAULT_QPMAX (MAX_QP_VALUE)
#define DEFAULT_INTRA_QP_DELTA (-5)
#define DEFAULT_FIXED_INTRA_QP (MIN_QP_VALUE)
#define DEFAULT_BFRAME_QP_DELTA (MIN_BFRAME_QP_DELTA)
#define DEFAULT_BITRATE (MAX_BITRATE)
#define DEFAULT_TOL_MOVING_BITRATE (MAX_BITRATE_VARIABLE_RANGE)
#define DEFAULT_BITVAR_RANGE_I (MAX_BITRATE_VARIABLE_RANGE)
#define DEFAULT_BITVAR_RANGE_P (MAX_BITRATE_VARIABLE_RANGE)
#define DEFAULT_BITVAR_RANGE_B (MAX_BITRATE_VARIABLE_RANGE)
#define DEFAULT_MONITOR_FRAMES (30)
#define DEFAULT_HRD_CPB_SIZE (10000000)


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
  u32 idr_interval;
  i32 last_idr_picture_cnt;
  u32 validencodedframenumber;
  u32 alignment;

  i32 max_cu_size;    /* Max coding unit size in pixels */
  i32 min_cu_size;    /* Min coding unit size in pixels */
  i32 max_tr_size;    /* Max transform size in pixels */
  i32 min_tr_size;    /* Min transform size in pixels */
  i32 tr_depth_intra;   /* Max transform hierarchy depth */
  i32 tr_depth_inter;   /* Max transform hierarchy depth */
  u32 outBufSizeMax; /* Max buf size in MB */
  u32 roiMapDeltaQpBlockUnit;
  
  // Rate Control Params
  i32 qphdr;
  u32 qpmin;
  u32 qpmax;
  i32 intra_qp_delta;
  i32 bFrameQpDelta;
  u32 fixed_intra_qp;
  u32 bitrate;
  u32 bitVarRangeI;
  u32 bitVarRangeP;
  u32 bitVarRangeB;
  u32 tolMovingBitRate;
  u32 monitorFrames;
  u32 pictureRc;
  u32 ctbRc;
  u32 blockRcSize;    /*size of block rate control : 2=16x16,1= 32x32, 0=64x64*/
  u32 pictureSkip;
  u32 hrd;
  u32 hrdCpbSize;

  u32 compressor;

  /* SW/HW shared memories for output buffers */
  void * ewl;
  EWLLinearMem_t outbufMem;

  float sumsquareoferror;
  float averagesquareoferror;
  i32 maxerrorovertarget;
  i32 maxerrorundertarget;
  long numbersquareoferror;

  char * roiArea1;
  char * roiArea2;

  u32 gopSize;
  u32 gopLength;
  VCEncIn encIn;
  VCEncOut encOut;
  bool codecH264;
  u32 intraPicRate;
  u8 gopCfgOffset[MAX_GOP_SIZE + 1];

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
  VCEncPictureCodingType nextCodingType;

} EncoderParams;
