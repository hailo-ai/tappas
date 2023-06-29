

#include "gopconfig.h"

//           Type POC QPoffset  QPfactor  num_ref_pics ref_pics  used_by_cur
char *RpsDefault_GOPSize_1[] = {
    "Frame1:  P    1   0        0.8     0      1        -1         1",
    NULL,
};
char *RpsDefault_H264_GOPSize_1[] = {
    "Frame1:  P    1   0        0.4     0      1        -1         1",
    NULL,
};

char *RpsDefault_GOPSize_2[] = {
    "Frame1:  P    2   0        0.6     0      1        -2         1",
    "Frame2:  B    1   0        0.68    0      2        -1 1       1 1",
    NULL,
};

char *RpsDefault_GOPSize_3[] = {
    "Frame1:  P    3   0        0.5     0      1        -3         1   ",
    "Frame2:  B    1   0        0.5     0      2        -1 2       1 1 ",
    "Frame3:  B    2   0        0.68    0      2        -1 1       1 1 ",
    NULL,
};


char *RpsDefault_GOPSize_4[] = {
    "Frame1:  P    4   0        0.5      0     1       -4         1 ",
    "Frame2:  B    2   0        0.3536   0     2       -2 2       1 1", 
    "Frame3:  B    1   0        0.5      0     3       -1 1 3     1 1 0", 
    "Frame4:  B    3   0        0.5      0     2       -1 1       1 1 ",
    NULL,
};

char *RpsDefault_GOPSize_5[] = {
    "Frame1:  P    5   0        0.442    0     1       -5         1 ",
    "Frame2:  B    2   0        0.3536   0     2       -2 3       1 1", 
    "Frame3:  B    1   0        0.68     0     3       -1 1 4     1 1 0", 
    "Frame4:  B    3   0        0.3536   0     2       -1 2       1 1 ",
    "Frame5:  B    4   0        0.68     0     2       -1 1       1 1 ",
    NULL,
};

char *RpsDefault_GOPSize_6[] = {
    "Frame1:  P    6   0        0.442    0     1       -6         1 ",
    "Frame2:  B    3   0        0.3536   0     2       -3 3       1 1", 
    "Frame3:  B    1   0        0.3536   0     3       -1 2 5     1 1 0", 
    "Frame4:  B    2   0        0.68     0     3       -1 1 4     1 1 0",
    "Frame5:  B    4   0        0.3536   0     2       -1 2       1 1 ",
    "Frame6:  B    5   0        0.68     0     2       -1 1       1 1 ",
    NULL,
};

char *RpsDefault_GOPSize_7[] = {
    "Frame1:  P    7   0        0.442    0     1       -7         1 ",
    "Frame2:  B    3   0        0.3536   0     2       -3 4       1 1", 
    "Frame3:  B    1   0        0.3536   0     3       -1 2 6     1 1 0", 
    "Frame4:  B    2   0        0.68     0     3       -1 1 5     1 1 0",
    "Frame5:  B    5   0        0.3536   0     2       -2 2       1 1 ",
    "Frame6:  B    4   0        0.68     0     3       -1 1 3     1 1 0",
    "Frame7:  B    6   0        0.68     0     2       -1 1       1 1 ",
    NULL,
};

char *RpsDefault_GOPSize_8[] = {
    "Frame1:  P    8   0        0.442    0  1           -8        1 ",
    "Frame2:  B    4   0        0.3536   0  2           -4 4      1 1 ",
    "Frame3:  B    2   0        0.3536   0  3           -2 2 6    1 1 0 ",
    "Frame4:  B    1   0        0.68     0  4           -1 1 3 7  1 1 0 0",
    "Frame5:  B    3   0        0.68     0  3           -1 1 5    1 1 0",
    "Frame6:  B    6   0        0.3536   0  2           -2 2      1 1",
    "Frame7:  B    5   0        0.68     0  3           -1 1 3    1 1 0",
    "Frame8:  B    7   0        0.68     0  2           -1 1      1 1",
    NULL,
};

char *RpsDefault_Interlace_GOPSize_1[] = {
    "Frame1:  P    1   0        0.8       0   2           -1 -2     0 1",
    NULL,
};

char *RpsLowdelayDefault_GOPSize_1[] = {
    "Frame1:  B    1   0        0.65      0     2       -1 -2         1 1",
    NULL,
};

char *RpsLowdelayDefault_GOPSize_2[] = {
    "Frame1:  B    1   0        0.4624    0     2       -1 -3         1 1",
    "Frame2:  B    2   0        0.578     0     2       -1 -2         1 1", 
    NULL,
};

char *RpsLowdelayDefault_GOPSize_3[] = {
    "Frame1:  B    1   0        0.4624    0     2       -1 -4         1 1",
    "Frame2:  B    2   0        0.4624    0     2       -1 -2         1 1", 
    "Frame3:  B    3   0        0.578     0     2       -1 -3         1 1", 
    NULL,
};

char *RpsLowdelayDefault_GOPSize_4[] = {
    "Frame1:  B    1   0        0.4624    0     2       -1 -5         1 1",
    "Frame2:  B    2   0        0.4624    0     2       -1 -2         1 1", 
    "Frame3:  B    3   0        0.4624    0     2       -1 -3         1 1", 
    "Frame4:  B    4   0        0.578     0     2       -1 -4         1 1",
    NULL,
};

static char *nextToken (char *str)
{
  char *p = strchr(str, ' ');
  if (p)
  {
    while (*p == ' ') p ++;
    if (*p == '\0') p = NULL;
  }
  return p;
}
static int ParseGopConfigString (char *line, VCEncGopConfig *gopCfg, int frame_idx, int gopSize)
{
  if (!line)
    return -1;

  //format: FrameN Type POC QPoffset QPfactor  num_ref_pics ref_pics  used_by_cur
  int frameN, poc, num_ref_pics, i;
  char type;
  VCEncGopPicConfig *cfg = &(gopCfg->pGopPicCfg[gopCfg->size ++]);

  //frame idx
  sscanf (line, "Frame%d", &frameN);
  if (frameN != (frame_idx + 1)) return -1;

  //frame type
  line = nextToken(line);
  if (!line) return -1;
  sscanf (line, "%c", &type);
  if (type == 'P' || type == 'p')
    cfg->codingType = VCENC_PREDICTED_FRAME;
  else if (type == 'B' || type == 'b')
    cfg->codingType = VCENC_BIDIR_PREDICTED_FRAME;
  else
    return -1;

  //poc
  line = nextToken(line);
  if (!line) return -1;
  sscanf (line, "%d", &poc);
  if (poc < 1 || poc > gopSize) return -1;
  cfg->poc = poc;

  //qp offset
  line = nextToken(line);
  if (!line) return -1;
  sscanf (line, "%d", &(cfg->QpOffset));

  //qp factor
  line = nextToken(line);
  if (!line) return -1;
  sscanf (line, "%lf", &(cfg->QpFactor));

  //temporalId factor
  line = nextToken(line);
  if (!line) return -1;
  sscanf (line, "%d", &(cfg->temporalId));

  //num_ref_pics
  line = nextToken(line);
  if (!line) return -1;
  sscanf (line, "%d", &num_ref_pics);
  if (num_ref_pics < 0 || num_ref_pics > VCENC_MAX_REF_FRAMES)
  {
    printf ("GOP Config: Error, num_ref_pic can not be more than %d \n", VCENC_MAX_REF_FRAMES);
    return -1;
  }
  cfg->numRefPics = num_ref_pics;

  //ref_pics
  for (i = 0; i < num_ref_pics; i ++)
  {
    line = nextToken(line);
    if (!line) return -1;
    sscanf (line, "%d", &(cfg->refPics[i].ref_pic));

    if (0)//(cfg->refPics[i].ref_pic < (-gopSize-1))
    {
      printf ("GOP Config: Error, Reference picture %d for GOP size = %d is not supported\n", cfg->refPics[i].ref_pic, gopSize);
      return -1;
    }
  }
  if (i < num_ref_pics) return -1;

  //used_by_cur
  for (i = 0; i < num_ref_pics; i ++)
  {
    line = nextToken(line);
    if (!line) return -1;
    sscanf (line, "%u", &(cfg->refPics[i].used_by_cur));
  }
  if (i < num_ref_pics) return -1;
  return 0;
}

static int HEVCParseGopConfigFile (int gopSize, char *fname, VCEncGopConfig *gopCfg)
{
#define MAX_LINE_LENGTH 1024
  int frame_idx = 0, line_idx = 0;
  char achParserBuffer[MAX_LINE_LENGTH];
  FILE *fIn = fopen (fname, "r");
  if (fIn == NULL)
  {
    printf("GOP Config: Error, Can Not Open File %s\n", fname );
    return -1;
  }

  while (frame_idx < gopSize)
  {
    if (feof (fIn)) break;
    line_idx ++;
    achParserBuffer[0] = '\0';
    // Read one line
    char *line = fgets ((char *) achParserBuffer, MAX_LINE_LENGTH, fIn);
    if (!line) break;
    //handle line end
    char* s = strpbrk(line, "#\n");
    if(s) *s = '\0';

    line = strstr(line, "Frame");
    if (line)
    {
      if (ParseGopConfigString (line, gopCfg, frame_idx, gopSize) < 0)
        break;

      frame_idx ++;
    }
  }
  while(!feof (fIn)) {
    line_idx ++;
    achParserBuffer[0] = '\0';
    // Read one line
    char *line = fgets ((char *) achParserBuffer, MAX_LINE_LENGTH, fIn);
    if (!line) break;
    //handle line end
    char* s = strpbrk(line, "#\n");
    if(s) *s = '\0';

    line = strstr(line, "longTermRefPicRate=");
    if (line && sscanf (line, "longTermRefPicRate=%d", &gopCfg->ltrInterval) == 1)
      break;
  }

  fclose(fIn);
  if (frame_idx != gopSize)
  {
    printf ("GOP Config: Error, Parsing File %s Failed at Line %d\n", fname, line_idx);
    return -1;
  }
  return 0;
}

static int HEVCReadGopConfig (char *fname, char **config, VCEncGopConfig *gopCfg, int gopSize, u8 *gopCfgOffset)
{
  int ret = -1;

  if (gopCfg->size >= MAX_GOP_PIC_CONFIG_NUM)
    return -1;
  
  if (gopCfgOffset)
    gopCfgOffset[gopSize] = gopCfg->size;
  if(fname)
  {
    ret = HEVCParseGopConfigFile (gopSize, fname, gopCfg);
  }
  else if(config)
  {
    int id = 0;
    while (config[id])
    {
      ParseGopConfigString (config[id], gopCfg, id, gopSize);
      id ++;
    }
    ret = 0;
  }
  return ret;
}

int VCEncInitGopConfigs (int gopSize, char *gopCfgName, VCEncGopConfig *gopCfg, u8 *gopCfgOffset, int bFrameQpDelta, bool codecH264)
{
  int i, pre_load_num;
  char *fname = gopCfgName;
  char **default_configs[8] = {
              codecH264?RpsDefault_H264_GOPSize_1:RpsDefault_GOPSize_1,
              RpsDefault_GOPSize_2,
              RpsDefault_GOPSize_3,
              RpsDefault_GOPSize_4,
              RpsDefault_GOPSize_5,
              RpsDefault_GOPSize_6,
              RpsDefault_GOPSize_7,
              RpsDefault_GOPSize_8 };

  if (gopSize < 0 || gopSize > MAX_GOP_SIZE)
  {
    printf ("GOP Config: Error, Invalid GOP Size\n");
    return -1;
  }

  // GOP size in rps array for gopSize=N
  // N<=4:      GOP1, ..., GOPN
  // 4<N<=8:   GOP1, GOP2, GOP3, GOP4, GOPN
  // N > 8:       GOP1, GOPN
  // Adaptive:  GOP1, GOP2, GOP3, GOP4, GOP6, GOP8
  if (gopSize > 8)
    pre_load_num = 1;
  else if (gopSize>=4 || gopSize==0)
    pre_load_num = 4;
  else
    pre_load_num = gopSize;

  gopCfg->ltrInterval = 0;
  for (i = 1; i <= pre_load_num; i ++)
  {    
    if (HEVCReadGopConfig (gopSize==i ? fname : NULL, default_configs[i-1], gopCfg, i, gopCfgOffset))
      return -1;
  }

  if (gopSize == 0)
  {
    //gop6
    if (HEVCReadGopConfig (NULL, default_configs[5], gopCfg, 6, gopCfgOffset))
      return -1;
    //gop8
    if (HEVCReadGopConfig (NULL, default_configs[7], gopCfg, 8, gopCfgOffset))
      return -1;
  }
  else if (gopSize > 4)
  {
    //gopSize
    if (HEVCReadGopConfig (fname, default_configs[gopSize-1], gopCfg, gopSize, gopCfgOffset))
      return -1;
  }

  if (gopCfg->ltrInterval > 0)
  {
    for(i = 0; i < (gopSize == 0 ? gopCfg->size : gopCfgOffset[gopSize]); i++)
    {
      // when use long-term, change P to B in default configs (used for last gop)
      VCEncGopPicConfig *cfg = &(gopCfg->pGopPicCfg[i]);
      if (cfg->codingType == VCENC_PREDICTED_FRAME)
        cfg->codingType = VCENC_BIDIR_PREDICTED_FRAME;
    }
  }

  //Compatible with old bFrameQpDelta setting
  if (bFrameQpDelta >= 0)
  {
    for (i = 0; i < gopCfg->size; i++)
    {
      VCEncGopPicConfig *cfg = &(gopCfg->pGopPicCfg[i]);
      if (cfg->codingType == VCENC_BIDIR_PREDICTED_FRAME)
        cfg->QpOffset = bFrameQpDelta;
    }
  }

  return 0;
}

VCEncPictureCodingType find_next_pic_internal(VCEncIn *encIn, EncoderParams *enc_params,
                                              i32 nextGopSize, u8 *gopCfgOffset)
{
  VCEncPictureCodingType nextCodingType;
  int idx, offset, cur_poc, delta_poc_to_next;
  int next_gop_size = nextGopSize;
  int picture_cnt_tmp = enc_params->picture_cnt;
  VCEncGopConfig *gopCfg = (VCEncGopConfig *)(&(encIn->gopConfig));

  //get current poc within GOP
  if (encIn->codingType == VCENC_INTRA_FRAME)
  {
    // next is an I Slice
    cur_poc = 0;
    encIn->gopPicIdx = 0;
  }
  else
  {
    //Update current idx and poc within a GOP
    idx = encIn->gopPicIdx + gopCfgOffset[encIn->gopSize];
    cur_poc = gopCfg->pGopPicCfg[idx].poc;
    encIn->gopPicIdx = (encIn->gopPicIdx + 1) % encIn->gopSize;
    if (encIn->gopPicIdx == 0)
      cur_poc -= encIn->gopSize;
  }

  //a GOP end, to start next GOP
  if (encIn->gopPicIdx == 0)
    offset = gopCfgOffset[next_gop_size];
  else
    offset = gopCfgOffset[encIn->gopSize];

  //get next poc within GOP, and delta_poc
  idx = encIn->gopPicIdx + offset;
  delta_poc_to_next = gopCfg->pGopPicCfg[idx].poc - cur_poc;
  //next picture cnt
  enc_params->picture_cnt = picture_cnt_tmp + delta_poc_to_next;

  //Handle Tail (cut by an I frame) 
  {
    //just finished a GOP and will jump to a P frame
    if (encIn->gopPicIdx == 0 && delta_poc_to_next > 1)
    {
      int gop_end_pic = enc_params->picture_cnt;
      int gop_shorten = 0;

      //cut by an IDR
      if ((enc_params->idr_interval) && ((gop_end_pic - enc_params->last_idr_picture_cnt) >= enc_params->idr_interval))
        gop_shorten = 1 + ((gop_end_pic - enc_params->last_idr_picture_cnt) - enc_params->idr_interval);

      if (gop_shorten >= next_gop_size)
      {
        //for gopsize = 1
        enc_params->picture_cnt = picture_cnt_tmp + 1 - cur_poc;
      }
      else if (gop_shorten > 0)
      {
        //reduce gop size
        const int max_reduced_gop_size = 4;
        next_gop_size -= gop_shorten;
        if (next_gop_size > max_reduced_gop_size)
          next_gop_size = max_reduced_gop_size;

        idx = gopCfgOffset[next_gop_size];
        delta_poc_to_next = gopCfg->pGopPicCfg[idx].poc - cur_poc;
        enc_params->picture_cnt = picture_cnt_tmp + delta_poc_to_next;
      }
      encIn->gopSize = next_gop_size;
    }

    encIn->poc += enc_params->picture_cnt - picture_cnt_tmp;
    //next coding type
    bool forceIntra = enc_params->idr_interval && ((enc_params->picture_cnt - enc_params->last_idr_picture_cnt) >= enc_params->idr_interval);
    if (forceIntra)
      nextCodingType = VCENC_INTRA_FRAME;
    else
    {
      idx = encIn->gopPicIdx + gopCfgOffset[encIn->gopSize];
      nextCodingType = gopCfg->pGopPicCfg[idx].codingType;
    }
  }
  gopCfg->id = encIn->gopPicIdx + gopCfgOffset[encIn->gopSize];
  {
    // guess next rps needed for H.264 DPB management (MMO), assume gopSize unchanged.
    // gopSize change only occurs on adaptive GOP or tail GOP (lowdelay = 0).
    // then the next RPS is 1st of default RPS of some gopSize, which only includes the P frame of last GOP
    i32 next_poc = gopCfg->pGopPicCfg[gopCfg->id].poc;
    i32 gopPicIdx = (encIn->gopPicIdx + 1) % encIn->gopSize;
    if (gopPicIdx == 0)
      next_poc -= encIn->gopSize;
    gopCfg->id_next = gopPicIdx + gopCfgOffset[encIn->gopSize];
    gopCfg->delta_poc_to_next = gopCfg->pGopPicCfg[gopCfg->id_next].poc - next_poc;
  }
  
  return nextCodingType;
}

VCEncPictureCodingType find_next_pic(EncoderParams *enc_params)
{
  return find_next_pic_internal(&(enc_params->encIn), enc_params, enc_params->nextGopSize, enc_params->gopCfgOffset);
}