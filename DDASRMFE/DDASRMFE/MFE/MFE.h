 /**
  * @file MFE.h
  * @author songhui(com@baidu.com)
  * @date 2012/03/12 15:46:50
  * @brief 
  **/  
#ifndef  __MFE_H_
#define  __MFE_H_

/** @brief Edition Number */

#define     EDITION     503

/** @brief Pre-Definition */

#define     MFE_SUCCESS                 0
#define     MFE_ERROR_UNKNOWN           -100
#define     MFE_STATE_ERR               -102
#define     MFE_POINTER_ERR             -103
#define     MFE_MEMALLOC_ERR            -107
#define     MFE_PARAMRANGE_ERR          -109
#define     MFE_SEND_TOOMORE_DATA_ONCE  -118
#define     MFE_VAD_INIT_ERROR          -120

#define     PARAM_MAX_WAIT_DURATION     1
#define     PARAM_MAX_SP_DURATION       2
#define     PARAM_MAX_SP_PAUSE          3
#define     PARAM_MIN_SP_DURATION       4
#define     PARAM_SLEEP_TIMEOUT         5
#define     PARAM_ENERGY_THRESHOLD_SP   6
#define     PARAM_ENERGY_THRESHOLD_EP   7
#define     PARAM_OFFSET                8
#define     PARAM_SPEECH_END            9
#define     PARAM_SPEECH_MODE           10
#define			PARAM_NEED_VAD              11                   //是否需要vad
#define     PARAM_NEED_COMPRESS         12                	 //是否需要压缩
#define     PARAM_SAMPLE_RATE           13                   //设置采样率
#define     PARAM_CODE_FORMAT           14                   //设置压缩格式
#define     PARAM_SPEECHIN_THRESHOLD_BIAS   15
#define     PARAM_SPEECHOUT_THRESHOLD_BIAS  16
#define     PARAM_BITRATE_NB            17
#define     PARAM_BITRATE_WB            18
#define     PARAM_BITRATE_OPUS_8K       19
#define     PARAM_BITRATE_OPUS_16K      20
#define     PARAM_MODE_CMB              21
#define     PARAM_CARLIFE_ENABLE        22



#define     PARAM_VADTHR_SILENCE_SP         0
#define     PARAM_VADTHR_SLIGHTNOISE_SP     1
#define     PARAM_VADTHR_NOISE_SP           2
#define     PARAM_VADTHR_SILENCE_EP         3
#define     PARAM_VADTHR_SLIGHTNOISE_EP     4
#define     PARAM_VADTHR_NOISE_EP           5


#define     MFE_FORMAT_BV32_8K          0
#define     MFE_FORMAT_PCM_8K           1
#define     MFE_FORMAT_ADPCM_8K         2
#define     MFE_FORMAT_AMR_8K           3
#define     MFE_FORMAT_BV32_16K         4
#define     MFE_FORMAT_PCM_16K          5
#define     MFE_FORMAT_AMR_16K          7
#define     MFE_FORMAT_FEA_16K_2_3_30   20
#define     MFE_FORMAT_FEA_16K_Shake    21
#define     MFE_FORMAT_OPUS_8K          64
#define     MFE_FORMAT_OPUS_16K         68

#define     VAD_FRAMELENGTH             256

#define     THREAD_SLEEPTIME            2000

enum DetectResult
{
        RET_SILENCE             =   0,
        RET_SILENCE_TO_SPEECH   =   1,
        RET_SPEECH_TO_SILENCE   =   2,
        RET_NO_SPEECH           =   3,
        RET_SPEECH_TOO_SHORT    =   4,
        RET_SPEECH_TOO_LONG     =   5,
        RET_RESERVE1            =   6,
        RET_RESERVE2            =   7,
        RET_RESERVE3            =   8,
        RET_RESERVE4            =   9,
};

int mfeInit();
int mfeExit();
int mfeSendData(short* pDataIn, int iLen);
int mfeGetCallbackData(char* pDataOut, int iLen);
int mfeDetect();
int mfeSetParam(int type, int val);
int mfeGetParam(int type);
int mfeEnableNoiseDetection(bool val);
void mfeSetLogLevel(int iLevel);
int mfeSetVADParam(int type, int val);  //  Extended Set Param Func, for Shanghai Voice Input Method


#endif  //__MFE_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
