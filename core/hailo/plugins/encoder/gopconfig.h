#pragma once
#include "enc_common.h"
int VCEncInitGopConfigs (int gopSize, char *gopCfgName, VCEncGopConfig *gopCfg, u8 *gopCfgOffset, int bFrameQpDelta, bool codecH264);
VCEncPictureCodingType find_next_pic(EncoderParams *enc_params);