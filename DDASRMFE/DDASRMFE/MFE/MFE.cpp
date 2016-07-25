 /***************************************************************************
  * 
  * Copyright (c) 2011 Baidu.com, Inc. All Rights Reserved
  * 
  **************************************************************************/
 
 /**
  * @file MFE.cpp
  * @author songhui(com@baidu.com)
  * @date 2011/12/05 16:55:55
  * @brief Client VAD Interface Function
  *  
  **/

#define     DYNAMIC_THR_ADJUST
#define     USING_BV32
#define     USING_AMR
//#define     USING_AMR_NB
//#define     USING_OPUS
#define     USING_BITMAP
#define     USING_BITMAP_SHAKE

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

#ifdef      USING_BITMAP
#include    "GetAudioBitmap.h"
#include    "BaseLib.h"
#endif

#ifdef      USING_BITMAP_SHAKE
#include    "GetAudioBitmap.h"
#include    "BaseLib.h"
#endif

#ifdef      USING_OPUS
#ifdef      HAVE_CONFIG_H
#include    "config.h"
#endif

#include    "opus.h"
#include    "debug.h"
#include    "opus_types.h"
#include    "opus_private.h"
#define     MAX_PACKET  1500
typedef    unsigned char UWord8;
typedef    int           Word32;
#endif

#ifdef      USING_AMR
/** @brief Include for AMR */
//extern "C"{
#include    "./amr-wb/enc.h"
#include    "./amr-wb/enc_acelp.h"
#include    "./amr-wb/enc_dtx.h"
#include    "./amr-wb/enc_gain.h"
#include    "./amr-wb/enc_if.h"
#include    "./amr-wb/enc_lpc.h"
#include    "./amr-wb/enc_main.h"
#include    "./amr-wb/enc_util.h"
#include    "./amr-wb/if_rom.h"
#include    "./amr-wb/typedef_amr.h"
#include    "./amr-wb/enc_rom.h"
//}
#endif

#ifdef      USING_AMR_NB
#include    "./amr-nb/interf_enc.h"
#include    "./amr-nb/interf_rom.h"
#include    "./amr-nb/rom_enc.h"
#include    "./amr-nb/sp_enc.h"
#include    <stdint.h>
#endif

#include    "MFE.h"
#include    "PreDefinition.h"
#include    "BaseLib.h"
#include    <stddef.h>
#include    <math.h>
#include    <pthread.h>
#include    <unistd.h>

#ifdef  USING_BV32
/** @brief Include for BV32 */
#include    "./bvcommon/typedef.h"
#include    "./bvcommon/bvcommon.h"
#include    "./bv32/bv32cnst.h"
#include    "./bv32/bv32strct.h"
#include    "./bv32/bv32.h"
#include    "./bvcommon/utility.h"
#include    "./bv32/bitpack.h"
#include    "./bvcommon/memutil.h"
#endif

/** @brief Include for Log */
#define     MFE_PLATFORM    0   /**< @brief 1 means PC, 0 means cellphone */
#define     DEFAULT_VOLUME  15000

#if MFE_PLATFORM
        #include    "ul_log.h"
        #define     LOG_PATH    "./"
        #define     LOG_MAX_SIZE    1024 * 1024 * 1024
#endif
#include    "drc.h"


/** @brief MFE Parameters */
enum MFEState
{
        UNKNOWN     =   0,
        BOOT        =   1,
        IDLE        =   2,
        RUN         =   3,
};

enum VADState
{
        SILENCE             =   0,
        SILENCE_TO_SPEECH   =   1,
        SPEECH_TO_SILENCE   =   2,
        NO_SPEECH           =   3,
        SPEECH_TOO_SHORT    =   4,
        SPEECH_TOO_LONG     =   5,
        SPEECH_PAUSE        =   6,  /**< @brief For Speech Input */
        NO_DETECT           =   7,  /**< @brief No Data Arrived */
};

/** @brief Global Variable Definition */
/** @brief Global Memory Data Space: 60KB in all.(30 * nFrameLength(256))
 * Line1: pSubbandDivision,     Length: nSubbandNum + 1     (DWORD)
 * Line2: pProbability,         Length: nFrameLength
 * Line3: pPowSpec,             Length: nFrameLength 
 * Line4: pSubbandEntropy,      Length: nSubbandNum
 * Line5: pTempEntropyArray,    Length: nL
 * Line6 ~ 6+nL-1: ppSubbandData,   Length: nL(17) * nSubbandNum
 * Line23: Default
 * ... */
double **g_ppMFEDataSpace;          /**< @brief Parameters used in VAD processing */
short  *g_pData;                    /**< @brief Original Data */
UWord8 *g_pBVData;                  /**< @brief BV32 Data */
short  *g_pVADResult;               /**< @brief Save VAD Frame-Result */
DWORD SubbandDivisionIdx = 1;
DWORD ProbabilityIdx = 2;
DWORD PowSpecIdx = 3;
DWORD SubbandEntropyIdx = 4;
DWORD TempEntropyArrayIdx = 5;
DWORD SubbandDataIdx = 6;


/** @brief Basic Parameters */
DWORD nCurState = (DWORD)UNKNOWN;       /**< @brief Current State of LSM */
DWORD nVADCurState = (DWORD)SILENCE;
DWORD nVADLastState = (DWORD)SILENCE;

DDWORD lSample = 0;                     /**< @brief Current Sample numbers */
                                        /**< @brief Also End of current segment */
DDWORD lSampleStart = 0;                /**< @brief Beginning of current segment */
DDWORD lSampleEnd   = 0;                /**< @brief End of current segment */
DDWORD lBVStartLoc = 0;                 /**< @brief Start Loc of VAD Cache g_pBVData */
DDWORD lBVCurLoc   = 0;                 /**< @brief Current Loc of VAD Cache */
DDWORD lVADResultStartLoc = 0;          /**< @brief VAD Result Start Loc (Frame) */
DDWORD lVADResultCurLoc = 0;            /**< @brief VAD Result Current Loc (Frame) */
DDWORD lFrameCnt = 0;                   /**< @brief Frame Counter (Last time) */
DDWORD lFrameCntTotal = 0;              /**< @brief For SpeechMode = 1, Total Frame Cnt */
DWORD nStartFrame = 0;
DWORD nEndFrame = 0;
double dThr_InSpeech = 0;
double dThr_OutSpeech = 0;
DWORD nIsFinishFlag = 0;                /**< @brief User Finish Voluntarily or Not */

/** @brief Basic Constant */
DWORD nSampleRate = 8000;
DWORD nCodeFormat = MFE_FORMAT_BV32_8K;
DWORD nFrameLength = VAD_FRAMELENGTH;
DWORD nFrameShift = VAD_FRAMELENGTH / 2;
DWORD nFFTOrder = 8;
bool bNoiseDetectionFlag = false;       /**< @brief Noise Detection Flag, Default: false */
int iLogLevel = 0;      /** @brief 0 means close Log; 7 means Log at /sdcard/decoder_api.log */

/** @brief VAD Parameters */
DWORD nN = 8;
DWORD nL = 2 * nN + 1;
DWORD nSubbandNum = 8;
double dQ = 10000000.0f;
double dBeta = 1.0f;
double dTheta = -0.0f;
double dLambda = 0.85;
DWORD nStartBackFrame = 20;
/** @brief For Speed */
DWORD nH = DWORD(dLambda * double(nL - 1));
double dLambda_hat = 1 - dLambda;

/** @brief 10 Open and Adjustable Parameters */
DWORD nMax_Wait_Duration_Init = 250;
DWORD nMax_Speech_Duration_Init = 400;
DWORD nMax_Speech_Pause_Init = 30;
DWORD nMin_Speech_Duration_Init = 5;
DWORD nSleep_Timeout_Init = 18;     /**< @brief Max Speech Length: 18s */
double dThreshold_Start_Init = 0.0;
double dThreshold_End_Init = 0.0;
DWORD nOffset_Init = 0;             /**< @brief Frames */
DWORD nSpeech_End_Init = 50;
DWORD nSpeech_Mode_Init = 0;        /**< @brief 1 means Continuous Speech Detect */
double dThrBias_SpeechIn_Init = 0;
double dThrBias_SpeechOut_Init = 0;

/** @brief Adjustable Parameters for Voice Input Method */
double dThrBias_SpeechIn_BI_Silence_Init = 0;
double dThrBias_SpeechIn_BI_Slightnoise_Init = 0;
double dThrBias_SpeechIn_BI_Noise_Init = 0;
double dThrBias_SpeechOut_BI_Silence_Init = 0;
double dThrBias_SpeechOut_BI_Slightnoise_Init = 0;
double dThrBias_SpeechOut_BI_Noise_Init = 0;

DWORD nMax_Wait_Duration    = nMax_Wait_Duration_Init;
DWORD nMax_Speech_Duration  = nMax_Speech_Duration_Init;
DWORD nMax_Speech_Pause     = nMax_Speech_Pause_Init;
DWORD nMin_Speech_Duration  = nMin_Speech_Duration_Init;
DWORD nSleep_Timeout        = nSleep_Timeout_Init;
double dThreshold_Start     = dThreshold_Start_Init;
double dThreshold_End       = dThreshold_End_Init;
DWORD nOffset               = nOffset_Init;
DWORD nSpeech_End           = nSpeech_End_Init;
DWORD nSpeech_Mode          = nSpeech_Mode_Init;
double dThrBias_SpeechIn    = dThrBias_SpeechIn_Init;
double dThrBias_SpeechOut   = dThrBias_SpeechOut_Init;

double dThrBias_SpeechIn_BI_Silence         = dThrBias_SpeechIn_BI_Silence_Init;
double dThrBias_SpeechIn_BI_Slightnoise     = dThrBias_SpeechIn_BI_Slightnoise_Init;
double dThrBias_SpeechIn_BI_Noise           = dThrBias_SpeechIn_BI_Noise_Init;
double dThrBias_SpeechOut_BI_Silence        = dThrBias_SpeechOut_BI_Silence_Init;
double dThrBias_SpeechOut_BI_Slightnoise    = dThrBias_SpeechOut_BI_Slightnoise_Init;
double dThrBias_SpeechOut_BI_Noise          = dThrBias_SpeechOut_BI_Noise_Init;

short nIn_Speech_Threshold  = 8;
DWORD nOffsetLength         = nOffset * nFrameLength;
DWORD nVADInnerCnt          = 0;    /**< @brief If there is eight "1", change VADState */
DWORD nVADInnerZeroCnt      = 0;
DWORD nSpeechEndCnt         = 0;    /**< @brief Used ONLY in SpeechMode 1 */
DWORD nSpeechEncLength      = 0;    /**< @brief Speech Encoder Framelength: BV: 80; AMR: 320 */

/** @brief For Endpoint Data Cache */
DWORD nFindPossibleEndPoint         = 0;
DWORD nPossible_Speech_Pause_Init   = 2;
DWORD nPossible_Speech_Pause        = nPossible_Speech_Pause_Init;
DWORD nPossible_Speech_Start        = 0;

/** @brief For More Accurate Startpoint & Endpoint Detection */
double dMaxSubEntro         = -100.0;   /**< @brief Max Subband Entropy during ONE SPEECH */
double dMinSubEntro         = 100.0;    /**< @brief Max Subband Entropy during ONE SPEECH */
double dThrAdjust_SpeechOut = 0.0;      /**< @brief Endpoint Threshold adjusted dynamicly */
double dThrAdjust_SpeechIn  = 0.0;      /**< @brief Startpoint Threshold adjusted dynamicly */
DWORD  nAmp                 = 0;
double pThrSubEntro[8];
double dBasicEnergy         = 0.0;
double pStartEnergy[8]      = {0.0, 0.0, 0.0, 0.0};
double pZeroPass[8]         = {0.0, 0.0, 0.0, 0.0};
double pMean[4]             = {0.0, 0.0, 0.0, 0.0};
DWORD  nBackEnd             = 4;        /**< @brief Backwards Frame in Speech pause (Input Mode) */

/** @brief For Keypad Tone Removing or Filtering */
bool bKeypadFiltering       = false;    /**< @brief Keypad Tone Filtering or Not */
DWORD nKeyToneRange         = 500;      /**< @brief The First 500ms */
DWORD nKeyToneStep          = 10;
DWORD nKeyToneOffset        = 50;       /**< @brief Offset 50ms */
DWORD nKeyTonePeakThr       = 10000;
DWORD nKeyToneLeftThr       = 1800;
DWORD nKeyToneRightThr      = 1400;
DWORD nKeyToneLeftRange     = 300;      /**< @brief 300ms silence on the left side of the Peak */
DWORD nKeyToneRightRange    = 300;
DWORD nKeyToneDelta         = 12;

/** @brief For First 200ms Modification */
bool bInvalidRecModification = false;
DWORD nModificationRange = 500;         /**< @brief The First 500ms */
DWORD nThr_ZeroPass = 8;
DWORD nThr_Energy = 1500;

/** @brief new para*/
bool nVAD 			     = true;
bool nCompress	     = true;
bool isFirst				 = true;
bool isNull					 = false;

/** @brief For BandTrap 1k and 2kHz */
float fa_16k[5] = {1, -2.93577536465610, 3.73663200309973, -2.37797804537144, 0.656100000000000};
float fb_16k[5] = {1, -3.26197262739567, 4.61312592975275, -3.26197262739567, 1};
float fa_8k[5] = {1, -1.35764501987817, 1.84320000000000, -1.25120565031972, 0.849346560000000};
float fb_8k[5] = {1, -1.41421356237310, 2, -1.41421356237310, 1};
float fa[5] = {1, -1.41279934881072, 1.99600200000000, -1.40997516291245, 0.996005996001000};
float fb[5] = {1, -1.41421356237310, 2, -1.41421356237310, 1};
float fx[5] = {0, 0, 0, 0, 0};
float fy[4] = {0, 0, 0, 0};

/** @brief Mode Combination */
DWORD nModeComb = 0;

/** @brief Carlife Noisy Environment endpoint detection 201450728 */
DWORD nCarlife = 0;
DRC_HANDLE drc_handle;
/** @brief Carlife Noisy Environment endpoint detection End 201450728 */

/** @brief BV32 Variables and Parameters */
int     frame;
int     sizebitstream, sizestate;
int     frsz;
struct  BV32_Encoder_State* state;
struct  BV32_Bit_Stream* bs;
UWord8  PackedStream[20];

#ifdef      USING_OPUS
int err;
OpusEncoder *enc = NULL;
int len[2];
int frame_size, channels;
opus_int32 bitrate_bps_init = 9000;
opus_int32 bitrate_bps = 0;
unsigned char *data[2];
unsigned char *fbytes;
opus_int32 sampling_rate;
int use_vbr;
int max_payload_bytes;
int complexity;
int use_inbandfec;
int use_dtx;
int forcechannels;
int cvbr = 0;
int packet_loss_perc;
opus_int32 count = 0, count_act = 0;
int k;
opus_int32 skip = 0;
short *in, *out;
int application = OPUS_APPLICATION_AUDIO;
double bits = 0.0, bits_max = 0.0, bits_act = 0.0, bits2 = 0.0, nrg;
int bandwidth = -1;
const char *bandwidth_string;
int lost = 0, lost_prev = 1;
int toggle = 0;
opus_uint32 enc_final_range[2];
int max_frame_size = 960 * 6;
int curr_read = 0;
int nb_modes_in_list = 0;
int curr_mode = 0;
int curr_mode_count = 0;
int mode_switch_time = 48000;
#endif

#ifdef      USING_AMR
/** @brief AMR Variables and Parameters */
UWord8  serial[NB_SERIAL_MAX];      /**< @brief AMR Data */
Word16  signalwave[L_FRAME16k];     /**< @brief Speech Data */
Word16  coding_mode = 4;            /**< @brief AMR Coding Mode: 16:1 */
void*   st;
#endif

#ifdef      USING_AMR_NB
void*   amr;
enum Mode mode = MR122;
int dtx = 0;
short buf[160];                     /**< @brief Input */
uint8_t outbuf[500];                /**< @brief Output */
#endif

#ifdef      USING_BITMAP
CAudioBitmap *g_extractBitmap;
DWORD nPackID;
DWORD nByteSizePerFrame = 125;
DWORD nFeatureDimPerFrame = 1997;
DWORD nBitSizePerUnit = 16;
UINT16 *g_pbitmap;
DWORD nMaxFeatureDensity = 3000;
#endif

#ifdef      USING_BITMAP_SHAKE
CAudioBitmap *g_extractBitmap_shake;
DWORD nPackID_shake;
DWORD nByteSizePerFrame_shake = 79;
DWORD nFeatureDimPerFrame_shake = 1254;
DWORD nBitSizePerUnit_shake = 16;
UINT16 *g_pbitmap_shake;
DWORD nMaxFeatureDensity_shake = 3000;
#endif

/** @brief MultiThread Mutex */
pthread_mutex_t MyMutex;
pthread_cond_t MyCond = PTHREAD_COND_INITIALIZER;
struct timespec ts;

/** @brief Log state */
#if MFE_PLATFORM
        ul_logstat_t logstate = {UL_LOG_ALL, UL_LOG_FATAL, 0};
#endif

/** @brief C functions */
#ifdef  USING_OPUS
void int_to_char(opus_uint32 i, unsigned char ch[4])
{
        ch[0] = i >> 24;
        ch[1] = (i >> 16) & 0xFF;
        ch[2] = (i >> 8) & 0xFF;
        ch[3] = i & 0xFF;
}
#endif

#ifdef  USING_AMR
int AMR_encode(void* st, char* buf_in, int len_in, char* buf_out, int* len_out, int mode, int coding_mode)
{
        int byte_used = 0;
        if(mode == 1)
        {
                if(len_in < L_FRAME16k * sizeof(Word16))
                {
                        *len_out = 0;
                        return 0;
                }

                *len_out = 0;
                *len_out = bdvr_amr::E_IF_encode(st, coding_mode,
                                (Word16*)buf_in, (UWord8*)buf_out, 0);
                byte_used = L_FRAME16k * sizeof(Word16);
        }
        return byte_used;
}
#endif


double VAD_SubbandEntropy_CalThreshold()
{
        double dSum = 0.0;
        double dThr = 0.0;
        DDWORD lMid = DDWORD((nN - 1) / 2);     /**< @brief Mid Value Index */
        DDWORD i, k;

        /** @brief Mid-value Filter for Every Subband, to Obtain the Threshold */
        for(k = 0; k < nSubbandNum; k++)
        {
                for(i = 0; i < nN; i++)
                {
                        g_ppMFEDataSpace[TempEntropyArrayIdx][i] = 
                            g_ppMFEDataSpace[SubbandDataIdx + i][k];
                }

                /** @brief Sort */
                Sort_QuickSort(g_ppMFEDataSpace[TempEntropyArrayIdx], nN);

                /** @brief Mid-value Filter (Find Mid-value), and Sum up */
                dSum += g_ppMFEDataSpace[TempEntropyArrayIdx][lMid];
                //printf("$$$$$$$ %.12f\n", g_ppMFEDataSpace[TempEntropyArrayIdx][lMid]);
        }

        dThr = dBeta * dSum - dTheta * nSubbandNum;

        return dThr;
}

bool VAD_SubbandEntropy_CalSubEntro(short *pData)
{
        DDWORD i, k;
        COMPLEX pFFTData[VAD_FRAMELENGTH];  /**< @brief Used in VAD CalSubEntro */

        /** @brief Cal Mean */
        pMean[lFrameCnt % 4] = 0.0;
        for(i = 0; i < VAD_FRAMELENGTH; i++)
        {
                pMean[lFrameCnt % 4] += double(pData[i]);
        }
        pMean[lFrameCnt % 4] /= VAD_FRAMELENGTH;
        //printf("%f\n", pMean[lFrameCnt % 4]);

        if(pMean[lFrameCnt % 4] > 1000.0 || pMean[lFrameCnt % 4] < -1000.0)
        {
                for(i = 0; i < VAD_FRAMELENGTH; i++)
                {
                        pFFTData[i].real = float(pData[i]) - float(pMean[lFrameCnt % 4]);
                        pFFTData[i].image = 0;
                }
        }
        else
        {
                for(i = 0; i < VAD_FRAMELENGTH; i++)
                {
                        pFFTData[i].real = float(pData[i]);
                        pFFTData[i].image = 0;
                }
        }

        /** @brief Calculate Power Spectrum of Current Frame */
        FFT(pFFTData, nFFTOrder);
        for(i = 0; i < VAD_FRAMELENGTH; i++)
        {
                g_ppMFEDataSpace[PowSpecIdx][i] = 
                    sqrt(pFFTData[i].real * pFFTData[i].real + pFFTData[i].image * pFFTData[i].image);
        }

        /** @brief Calculate Probability of Every Freq-Domain point */
        double dTemp = 0.0;
        for(k = 0; k < nSubbandNum; k++)
        {
                dTemp = 0.0;

                /** @brief Calculate Denominator of Every Subband */
                for(i = DDWORD(g_ppMFEDataSpace[SubbandDivisionIdx][k]); 
                        i < DDWORD(g_ppMFEDataSpace[SubbandDivisionIdx][k+1]); i++)
                {
                        dTemp += g_ppMFEDataSpace[PowSpecIdx][i] + dQ;
                }

                /** @brief Replace Division by Multiply */
                dTemp = double(1.0 / dTemp);

                for(i = DDWORD(g_ppMFEDataSpace[SubbandDivisionIdx][k]);
                        i < DDWORD(g_ppMFEDataSpace[SubbandDivisionIdx][k+1]); i++)
                {
                        g_ppMFEDataSpace[ProbabilityIdx][i] = 
                            (g_ppMFEDataSpace[PowSpecIdx][i] + dQ) * dTemp;
                }
        }

        /** @brief Calculate K Subband Entropy of Current Frame: E[l, k] */
        for(k = 0; k < nSubbandNum; k++)
        {
                g_ppMFEDataSpace[SubbandEntropyIdx][k] = 0.0;

                for(i = DDWORD(g_ppMFEDataSpace[SubbandDivisionIdx][k]);
                        i < DDWORD(g_ppMFEDataSpace[SubbandDivisionIdx][k+1]); i++)
                {
                        g_ppMFEDataSpace[SubbandEntropyIdx][k] += 
                            g_ppMFEDataSpace[ProbabilityIdx][i] * 
                            log(g_ppMFEDataSpace[ProbabilityIdx][i]);
                }
        }

        return true;
}

short VAD_SubbandEntropy_Process(short *pData)
{
        /** @brief Cal SubbandEntropy, result in g_ppMFEDataSpace[SubbandEntropyIdx][k] */
        VAD_SubbandEntropy_CalSubEntro(pData);

        /** @brief Assume the first nN Frames are non-speech */
        /** @brief Do Not Consider Offset: 2012.11.27 */
        if(nSpeech_Mode == 0)
        {
                if(lFrameCnt < nOffset)
                {
                        return 0;
                }
                else
                {
                        lFrameCnt -= nOffset;
                }
        }
        else
        {
                if(lFrameCntTotal < nOffset)
                {
                        return 0;
                }
                else
                {
                        lFrameCntTotal -= nOffset;
                }
        }
        /** @brief Do Not Consider Offset: 2012.11.27 End */

        /** @brief Fix Offset=8 bug 2013.11.18 */
        DDWORD lFrameNum;
        if(nSpeech_Mode == 0)
        {
                lFrameNum = lFrameCnt;
        }
        else
        {
                lFrameNum = lFrameCntTotal;
        }
        /** @brief Fix Offset=8 bug End */

        if(lFrameNum <= nN && nVADCurState == SILENCE)
        {
                /** @brief Calculate Basic Energy */
                if(lFrameNum < 4)
                {
                        if(lFrameNum == 1)
                        {
                                dBasicEnergy = 0.0;
                        }
                        for(int ii = 0; ii < VAD_FRAMELENGTH; ii++)
                        {
                                dBasicEnergy += double(abs(pData[ii]));
                        }
                        if(lFrameNum == 3)
                        {
                                dBasicEnergy /= (4 * VAD_FRAMELENGTH);
                                //printf("BasicEnergy: %.12f\n", dBasicEnergy);
                        }
                }

                if(lFrameNum < 8)
                {
                        pZeroPass[lFrameNum] = 0.0;
                        for(int ii = 0; ii < VAD_FRAMELENGTH - 1; ii++)
                        {
                                if((pData[ii] > 0 && pData[ii + 1] < 0) || (pData[ii] < 0 && pData[ii + 1] > 0))
                                {
                                        pZeroPass[lFrameNum] += 1;
                                }
                        }
                        //printf("ZeroPass %d : %.12f\n", lFrameNum, pZeroPass[lFrameNum]);

                        pStartEnergy[lFrameNum] = 0.0;
                        for(int ii = 0; ii < VAD_FRAMELENGTH; ii++)
                        {
                                pStartEnergy[lFrameNum] += double(abs(pData[ii]));
                        }
                        pStartEnergy[lFrameNum] /= VAD_FRAMELENGTH;
                        //printf("StartEnergy %d : %.12f\n", lFrameNum, pStartEnergy[lFrameNum]);
                }

                for(DWORD k = 0; k < nSubbandNum; k++)
                {
                        g_ppMFEDataSpace[SubbandDataIdx + lFrameNum][k] = 
                            g_ppMFEDataSpace[SubbandEntropyIdx][k];
                        if(lFrameNum < DDWORD(nN))
                        {
                                g_ppMFEDataSpace[SubbandDataIdx + lFrameNum + nL - nN][k] = 
                                    g_ppMFEDataSpace[SubbandEntropyIdx][k];
                        }
                }

                /** @brief Calculate Threshold */
                if(lFrameNum == DDWORD(nN))
                {
                        if(nSpeech_Mode == 2)   // Old VoiceSearch Param, dQ = 1000000.0
                        {
                                dThr_InSpeech = VAD_SubbandEntropy_CalThreshold();
                                dThr_OutSpeech = dThr_InSpeech + 0.0003;
                                dThr_InSpeech += 0.00025;

                                dThr_OutSpeech = dThr_InSpeech;
                                dThr_OutSpeech -= 0.0001;

                                /** @brief Param Adjust by Client */
                                dThr_InSpeech += dThrBias_SpeechIn;
                                dThr_OutSpeech += dThrBias_SpeechOut;
                        }
                        else if(nSpeech_Mode == 0 || nSpeech_Mode == 1)   //  dQ = 10000000.0
                        {
                                if (nCarlife == 0)    //  8k & 16k
                                {
                                        dThr_InSpeech = VAD_SubbandEntropy_CalThreshold();
                                        dThr_OutSpeech = dThr_InSpeech + 0.0001;
                                        dThr_InSpeech += 0.0001;
                                        dThr_OutSpeech -= 0.00007;
                                        dThr_InSpeech = dThr_OutSpeech;
                                        dThr_OutSpeech -= 0.00002;

                                        //printf("In Thr: %.9f\n", dThr_InSpeech);
                                        if(dThr_InSpeech > -27.725830 && dThr_InSpeech < -27.725800)
                                        {
                                                dThr_InSpeech += 0.00012;
                                                dThr_OutSpeech += 0.00018;
                                                dThr_InSpeech += dThrBias_SpeechIn_BI_Noise;
                                                dThr_OutSpeech += dThrBias_SpeechOut_BI_Noise;
                                                nAmp = 3;
                                        }
                                        else if(dThr_InSpeech > -27.725854555 && dThr_InSpeech <= -27.725830)
                                        {
                                                dThr_InSpeech += 0.00006;
                                                dThr_OutSpeech += 0.00006;
                                                dThr_InSpeech += dThrBias_SpeechIn_BI_Noise;
                                                dThr_OutSpeech += dThrBias_SpeechOut_BI_Noise;
                                                nAmp = 2;
                                        }
                                        else if(dThr_InSpeech >= -27.725800)
                                        {
                                                dThr_InSpeech += 0.00015;
                                                dThr_OutSpeech += 0.00015;
                                                dThr_InSpeech += dThrBias_SpeechIn_BI_Noise;
                                                dThr_OutSpeech += dThrBias_SpeechOut_BI_Noise;
                                                nAmp = 4;
                                        }
                                        else if(dThr_InSpeech <= -27.72585555)     // -27.725857
                                        {
                                                dThr_InSpeech = -27.7258825;
                                                dThr_OutSpeech = -27.72588525;
                                                dThr_InSpeech += dThrBias_SpeechIn_BI_Silence;
                                                dThr_OutSpeech += dThrBias_SpeechOut_BI_Silence;
                                                nAmp = 0;
                                        }
                                        else    //  Slight Noise
                                        {
                                                dThr_InSpeech = -27.725866;
                                                dThr_OutSpeech = -27.725880;
                                                dThr_InSpeech += dThrBias_SpeechIn_BI_Slightnoise;
                                                dThr_OutSpeech += dThrBias_SpeechOut_BI_Slightnoise;
                                                nAmp = 1;
                                        }

                                        if(nAmp > 0)
                                        {
                                                double dMinZeroPass = 1000.0;
                                                double dMaxZeroPass = -1.0;
                                                for(int ii = 3; ii < nN; ii++)
                                                {
                                                        if(pStartEnergy[ii] > 1000.0)
                                                        {
                                                                if(pZeroPass[ii] < dMinZeroPass)
                                                                {
                                                                        dMinZeroPass = pZeroPass[ii];
                                                                }
                                                        }
                                                        if(1)
                                                        {
                                                                if(pZeroPass[ii] > dMaxZeroPass)
                                                                {
                                                                        dMaxZeroPass = pZeroPass[ii];
                                                                }
                                                        }
                                                }

                                                if(dMinZeroPass == 1000.0)
                                                {
                                                        for(int ii = 3; ii < nN - 1; ii++)
                                                        {
                                                                if(pZeroPass[ii] < pZeroPass[ii + 1])
                                                                {
                                                                        if(pZeroPass[ii] < dMinZeroPass)
                                                                        {
                                                                                dMinZeroPass = pZeroPass[ii];
                                                                        }
                                                                }
                                                                else
                                                                {
                                                                        if(pZeroPass[ii + 1] < dMinZeroPass)
                                                                        {
                                                                                dMinZeroPass = pZeroPass[ii + 1];
                                                                        }
                                                                }
                                                        }
                                                        if(dMinZeroPass != 0.00)
                                                        {
                                                                dMinZeroPass = 1000.0;
                                                        }
                                                }

                                                if((dMinZeroPass < 30) || (dMaxZeroPass > 130))
                                                {
                                                        dThr_InSpeech = -27.7258825;
                                                        dThr_OutSpeech = -27.72588525;
                                                        dThr_InSpeech += dThrBias_SpeechIn_BI_Silence;
                                                        dThr_OutSpeech += dThrBias_SpeechOut_BI_Silence;
                                                        nAmp = 0;
                                                }
                                                pZeroPass[0] = dMinZeroPass;
                                        }

                                        //printf("nAmp: %d\n", nAmp);
                                }
                                else    // Carlife Environment
                                {
                                        dThr_InSpeech = VAD_SubbandEntropy_CalThreshold();
                                        dThr_OutSpeech = dThr_InSpeech + 0.0001;
                                        dThr_InSpeech += 0.0001;
                                        dThr_OutSpeech -= 0.00007;
                                        dThr_InSpeech = dThr_OutSpeech;
                                        dThr_OutSpeech -= 0.00002;

                                        //printf("In Thr: %.9f\n", dThr_InSpeech);
                                        if (dThr_InSpeech > -27.725830 && dThr_InSpeech < -27.725800)
                                        {
                                                dThr_InSpeech += 0.00012;
                                                dThr_OutSpeech += 0.00018;
                                                dThr_InSpeech += dThrBias_SpeechIn_BI_Noise;
                                                dThr_OutSpeech += dThrBias_SpeechOut_BI_Noise;
                                                nAmp = 3;
                                        }
                                        else if (dThr_InSpeech > -27.725854555 && dThr_InSpeech <= -27.725830)
                                        {
                                                dThr_InSpeech += 0.00006;
                                                dThr_OutSpeech += 0.00006;
                                                dThr_InSpeech += dThrBias_SpeechIn_BI_Noise;
                                                dThr_OutSpeech += dThrBias_SpeechOut_BI_Noise;
                                                nAmp = 2;
                                        }
                                        else if (dThr_InSpeech >= -27.725800)
                                        {
                                                dThr_InSpeech += 0.00015;
                                                dThr_OutSpeech += 0.00015;
                                                dThr_InSpeech += dThrBias_SpeechIn_BI_Noise;
                                                dThr_OutSpeech += dThrBias_SpeechOut_BI_Noise;
                                                nAmp = 4;
                                        }
                                        else if (dThr_InSpeech <= -27.725857)     // -27.725857
                                        {
                                                dThr_InSpeech = -27.7258825;
                                                dThr_OutSpeech = -27.72588525;
                                                dThr_InSpeech += dThrBias_SpeechIn_BI_Silence;
                                                dThr_OutSpeech += dThrBias_SpeechOut_BI_Silence;
                                                nAmp = 0;
                                        }
                                        else    //  Slight Noise
                                        {
                                                dThr_InSpeech = -27.725866;
                                                dThr_OutSpeech = -27.725880;
                                                dThr_InSpeech += dThrBias_SpeechIn_BI_Slightnoise;
                                                dThr_OutSpeech += dThrBias_SpeechOut_BI_Slightnoise;
                                                nAmp = 1;
                                        }
                                }

                                /** @brief Param Adjust by Client */
                                dThr_InSpeech += dThrBias_SpeechIn;
                                dThr_OutSpeech += dThrBias_SpeechOut;
                        }
                        else
                        {
                                ;
                        }
                        //printf("In Thr: %f\n", dThr_InSpeech);
                        //printf("Out Thr: %f\n", dThr_OutSpeech);
                }

                /** @brief Do Not Consider Offset: 2012.11.27 */
                if(nSpeech_Mode == 0)
                {
                        lFrameCnt += nOffset;
                }
                else
                {
                        lFrameCntTotal += nOffset;
                }
                /** @brief Do Not Consider Offset: 2012.11.27 End */
                return 0;
        }
        else
        {
                DDWORD lCurrPos;
                if(nSpeech_Mode == 0)
                {
                        lCurrPos = lFrameCnt % nL;
                }
                else
                {
                        lCurrPos = lFrameCntTotal % nL;
                }
                for(DWORD k = 0; k < nSubbandNum; k++)
                {
                        g_ppMFEDataSpace[SubbandDataIdx + lCurrPos][k] = 
                            g_ppMFEDataSpace[SubbandEntropyIdx][k];
                }

                /** @brief Calculate Subband Entropy of the Current Frame */
                double dH = 0.0;

                for(DWORD k = 0; k < nSubbandNum; k++)
                {
                        for(DWORD i = 0; i < nL; i++)
                        {
                                g_ppMFEDataSpace[TempEntropyArrayIdx][i] =
                                    g_ppMFEDataSpace[SubbandDataIdx + i][k];
                        }

                        /** @brief Sort */
                        Sort_QuickSort(g_ppMFEDataSpace[TempEntropyArrayIdx], nL);

                        /** @brief Calculate Subbband Entropy after Mid-Value Filtering */
                        dH += dLambda_hat * g_ppMFEDataSpace[TempEntropyArrayIdx][nH]
                            + dLambda * g_ppMFEDataSpace[TempEntropyArrayIdx][nH + 1];
                }
                //printf("%.12f\n", dH);

#ifdef DYNAMIC_THR_ADJUST
                // Reset
                dThrAdjust_SpeechOut = 0.0;
                dThrAdjust_SpeechIn = 0.0;

                /** @brief Find Max SubEntro in history */
                if(dH > dMaxSubEntro && nVADCurState == SILENCE_TO_SPEECH)
                {
                        dMaxSubEntro = dH;
                        //printf("Max: %f\n", dMaxSubEntro);

                        /** @brief Adjust Thr_SpeechOut Bias according to Max SubEntro */
                        if(nAmp == 0)   // Silence Mode
                        {
                                if(dMaxSubEntro > -27.725700)
                                {
                                        dThrAdjust_SpeechOut = 0.00001225;
                                }
                                else if(dMaxSubEntro > -27.725775)
                                {
                                        dThrAdjust_SpeechOut = 0.00001100;
                                }
                                else if(dMaxSubEntro > -27.725800)
                                {
                                        dThrAdjust_SpeechOut = 0.00001;
                                }
                                else if(dMaxSubEntro > -27.725840)
                                {
                                        dThrAdjust_SpeechOut = 0.000001;
                                }
                                else
                                {
                                        dThrAdjust_SpeechOut = 0.0;
                                }
                        }
                        else if(nAmp == 1 || nAmp == 2)
                        {
                                if(dMaxSubEntro > -27.724600)
                                {
                                        dThrAdjust_SpeechOut = 0.00018;
                                }
                                else
                                {
                                        dThrAdjust_SpeechOut = 0.0;
                                }
                        }
                        else if(nAmp == 3 || nAmp == 4)
                        {
                                if(dMaxSubEntro > -27.724600)
                                {
                                        dThrAdjust_SpeechOut = 0.00018;
                                }
                                else
                                {
                                        dThrAdjust_SpeechOut = 0.0;
                                }
                        }
                        else
                        {
                                dThrAdjust_SpeechOut = 0.0;
                        }
                }

                if(dH < dMinSubEntro && nVADCurState == SILENCE_TO_SPEECH)
                {
                        dMinSubEntro = dH;
                        //printf("Min: %f\n", dMinSubEntro);
                }

                if(dBasicEnergy < 8.0)
                {
                        dThrAdjust_SpeechIn = -0.0000043;
                        dThrAdjust_SpeechOut = -0.00000185;
                        if(dBasicEnergy == 0 && nVADCurState == SPEECH_PAUSE && dMaxSubEntro < -27.725885)
                        {
                                dThrAdjust_SpeechOut = -0.00000185;
                                dThrAdjust_SpeechIn = (dThr_OutSpeech + dThrAdjust_SpeechOut) - dThr_InSpeech;
                        }
                }
                else if(dBasicEnergy < 90.0 || pZeroPass[0] == 0)
                {
                        if(dMaxSubEntro < -27.725881)
                        {
                                dThrAdjust_SpeechOut = -0.00000185;
                                dThrAdjust_SpeechIn = -0.0000038;
                        }
                        else if(dMaxSubEntro < -27.725873)
                        {
                                dThrAdjust_SpeechOut = -0.00000155;
                                dThrAdjust_SpeechIn = -0.0000028;
                        }
                        else if(dMaxSubEntro < -27.725840)
                        {
                                dThrAdjust_SpeechOut = -0.000001;
                                dThrAdjust_SpeechIn = 0.0000000;
                        }
                        else if(dMaxSubEntro < -27.725820)
                        {
                                dThrAdjust_SpeechOut = -0.0000005;
                                dThrAdjust_SpeechIn = 0.0000000;
                        }
                        else if(dMaxSubEntro < -27.725700)
                        {
                                //dThrAdjust_SpeechOut = 0.00001225 - 0.000008;
                                dThrAdjust_SpeechOut = 0.000000;
                                dThrAdjust_SpeechIn = 0.0000000;
                        }
                        else
                        {
                                dThrAdjust_SpeechIn = 0.0000000;
                        }
                }
                else
                {
                        dThrAdjust_SpeechIn = 0.0;
                }

                /** @brief No Endpoint Detect if Noisy Environment (2013.8.23) */
                if(nAmp > 1 && nSpeech_Mode == 0)
                {
                        dThrAdjust_SpeechOut = -24;
                }
                    
                //printf("%.12f\n", dThrAdjust_SpeechIn);
                //printf("%.12f\n", dThrAdjust_SpeechOut);
#endif

                /** @brief Do Not Consider Offset: 2012.11.27 */
                if(nSpeech_Mode == 0)
                {
                        lFrameCnt += nOffset;
                }
                else
                {
                        lFrameCntTotal += nOffset;
                }
                /** @brief Do Not Consider Offset: 2012.11.27 End */

                if((nVADCurState == SILENCE) || (nVADCurState == SPEECH_PAUSE))
                {   
                        if(nSpeech_Mode == 0)
                        {
                                return dH > dThr_InSpeech + dThrAdjust_SpeechIn ? 1 : 0;
                        }
                        else
                        {
                                if(nVADCurState == SPEECH_PAUSE && nAmp > 0 && dMaxSubEntro > -27.724600)
                                {
                                        return dH > dThr_InSpeech + (dMaxSubEntro - dThr_InSpeech) / 5 ? 1 : 0;
                                }
                                else
                                {
                                        return dH > ((dThr_InSpeech + dThrAdjust_SpeechIn) 
                                                + (dThr_OutSpeech + dThrAdjust_SpeechOut)) / 2 ? 1 : 0;
                                }
                        }
                }
                else
                {
                        if(nSpeech_Mode == 1)
                        {
                                return dH > dThr_OutSpeech + dThrAdjust_SpeechOut ? 1 : 0;
                        }
                        else
                        {
                                return dH > dThr_OutSpeech + dThrAdjust_SpeechOut ? 1 : 0;
                        }
                }
        }
}



int mfeInit()
{
				isFirst = true;
				isNull  = false; 
        #if MFE_PLATFORM
                if(iLogLevel == 7)
                {
                        if(ul_openlog(LOG_PATH, "Audiofe_api", &logstate, LOG_MAX_SIZE) < 0)
                        {
                                ul_writelog(UL_LOG_FATAL, "[%s:%d] Fatal Error: Open log fail!", 
                                        __FILE__, __LINE__);
                        }
                        return MFE_ERROR_UNKNOWN;
                }
        #endif

        if(nCodeFormat == MFE_FORMAT_FEA_16K_2_3_30 && nSampleRate != 16000)
        {
                return MFE_PARAMRANGE_ERR;
        }

        if(nCurState == (DWORD)UNKNOWN)
        {
                /** @brief Parameters Initialization */
                nVADLastState = DWORD(SILENCE);
                lSample = 0;        // Maximum: nSleep_Timeout * nSampleRate
                lSampleStart = 0;
                lSampleEnd = 0;
                lFrameCnt = 0;
                lFrameCntTotal = 0;
                nStartFrame = 0;
                nEndFrame = 0;
                dThr_InSpeech = 0.0;
                dThr_OutSpeech = 0.0;
                nIsFinishFlag = 0;
                if(0)
                {
                        ;
                }
#ifdef  USING_BV32
                else if((nCodeFormat == MFE_FORMAT_BV32_8K) || (nCodeFormat == MFE_FORMAT_BV32_16K))
                {
                        nSpeechEncLength = FRSZ;
                }
#endif
#ifdef  USING_AMR
                else if(nCodeFormat == MFE_FORMAT_AMR_16K)
                {
                        nSpeechEncLength = L_FRAME16k;
                }
#endif
#ifdef  USING_AMR_NB
                else if(nCodeFormat == MFE_FORMAT_AMR_8K)
                {
                        nSpeechEncLength = 160;
                }
#endif
#ifdef  USING_OPUS
                else if(nCodeFormat == MFE_FORMAT_OPUS_8K)
                {
                        nSpeechEncLength = 8000 / 50;
                }
                else if(nCodeFormat == MFE_FORMAT_OPUS_16K)
                {
                        nSpeechEncLength = 16000 / 50;
                }
#endif
                else
                {
                        nSpeechEncLength = 80;
                }

                if(nSpeech_Mode_Init == 1)
                {
                        //nN = 4;
                        //nL = 2 * nN + 1;
                        //nH = DWORD(dLambda * double(nL - 1));
                        nStartBackFrame = 26;
                        nMax_Speech_Pause_Init = 10;
                        nSleep_Timeout_Init = 60;
                        nPossible_Speech_Pause_Init = 8;
                }
                else
                {
                        nStartBackFrame = 20;
                        nMax_Speech_Pause_Init = 30;
                        nSleep_Timeout_Init = 60;
                        nPossible_Speech_Pause_Init = 2;
                }

                if(nSampleRate == 8000)
                {
                        for(int ii = 0; ii < 5; ii++)
                        {
                                fa[ii] = fa_8k[ii];
                                fb[ii] = fb_8k[ii];
                        }
                }
                else
                {
                        for(int ii = 0; ii < 5; ii++)
                        {
                                fa[ii] = fa_16k[ii];
                                fb[ii] = fb_16k[ii];
                        }
                }

                /** @brief Init Mutex using Default Property */
                pthread_mutex_init(&MyMutex, NULL);

                if(nSampleRate == 16000)
                {
                        nMax_Wait_Duration = nMax_Wait_Duration_Init * 2;
                        nMax_Speech_Duration = nMax_Speech_Duration_Init * 2;
                        nMax_Speech_Pause = nMax_Speech_Pause_Init * 2;
                        nMin_Speech_Duration = nMin_Speech_Duration_Init * 2;
                        nSleep_Timeout = nSleep_Timeout_Init;
                        dThreshold_Start = 0.0;
                        dThreshold_End = 0.0;
                        nOffset = nOffset_Init * 2;
                        nSpeech_End = nSpeech_End_Init * 2;
                        nSpeech_Mode = nSpeech_Mode_Init;
                        nIn_Speech_Threshold = 8;
                        nPossible_Speech_Pause = nPossible_Speech_Pause_Init * 2;
                }
                else
                {
                        nMax_Wait_Duration = nMax_Wait_Duration_Init;
                        nMax_Speech_Duration = nMax_Speech_Duration_Init;
                        nMax_Speech_Pause = nMax_Speech_Pause_Init;
                        nMin_Speech_Duration = nMin_Speech_Duration_Init;
                        nSleep_Timeout = nSleep_Timeout_Init;
                        dThreshold_Start = 0.0;
                        dThreshold_End = 0.0;
                        nOffset = nOffset_Init;
                        nSpeech_End = nSpeech_End_Init;
                        nSpeech_Mode = nSpeech_Mode_Init;
                        nIn_Speech_Threshold = 8;
                        nPossible_Speech_Pause = nPossible_Speech_Pause_Init;
                }

                dThrBias_SpeechIn = dThrBias_SpeechIn_Init * 1e-7;
                dThrBias_SpeechOut = dThrBias_SpeechOut_Init * 1e-7;
                dThrBias_SpeechIn_BI_Silence = dThrBias_SpeechIn_BI_Silence_Init * 1e-7;
                dThrBias_SpeechIn_BI_Slightnoise = dThrBias_SpeechIn_BI_Slightnoise_Init * 1e-7;
                dThrBias_SpeechIn_BI_Noise = dThrBias_SpeechIn_BI_Noise_Init * 1e-7;
                dThrBias_SpeechOut_BI_Silence = dThrBias_SpeechOut_BI_Silence_Init * 1e-7;
                dThrBias_SpeechOut_BI_Slightnoise = dThrBias_SpeechOut_BI_Slightnoise_Init * 1e-7;
                dThrBias_SpeechOut_BI_Noise = dThrBias_SpeechOut_BI_Noise_Init * 1e-7;
                nOffsetLength = nOffset * nFrameLength;
                nVADInnerCnt = 0;
                nVADInnerZeroCnt = 0;
                nSpeechEndCnt = 0;
                nFindPossibleEndPoint = 0;

                /** @brief FrontEnd Initilization */
                g_ppMFEDataSpace = (double **)malloc(sizeof(double *) * 30);
                for(DDWORD i = 0; i < 30; i++)
                {
                        g_ppMFEDataSpace[i] = (double *)malloc(sizeof(double) * nFrameLength);
                }
                if(g_ppMFEDataSpace == NULL)
                {
                        #if MFE_PLATFORM
                                if(iLogLevel == 7)
                                {
			                            ul_writelog(UL_LOG_FATAL, "[%s:%d] mfeInit: \
                                                Malloc Globel Memory Failed!", \
                                                __FILE__, __LINE__);
                                }
                        #endif
                        return MFE_MEMALLOC_ERR;
                }

                g_pData = (short *)malloc(sizeof(short) * nSleep_Timeout * nSampleRate);
                if(g_pData == NULL)
                {
                        #if MFE_PLATFORM
                                if(iLogLevel == 7)
                                {
			                            ul_writelog(UL_LOG_FATAL, "[%s:%d] mfeInit: \
                                                Malloc Globel Memory Failed!", \
                                                __FILE__, __LINE__);
                                }
                        #endif
                        if(g_ppMFEDataSpace != NULL)
                        {
                                for(DDWORD i = 0; i < 30; i++)
                                {
                                        free(g_ppMFEDataSpace[i]);
                                }
                                free(g_ppMFEDataSpace);
                                g_ppMFEDataSpace = NULL;
                        }
                        return MFE_MEMALLOC_ERR;
                }

                g_pBVData = (UWord8 *)malloc(sizeof(UWord8) * nSleep_Timeout * nSampleRate / 4);
                if(g_pBVData == NULL)
                {
                        #if MFE_PLATFORM
                                if(iLogLevel == 7)
                                {
			                            ul_writelog(UL_LOG_FATAL, "[%s:%d] mfeInit: \
                                                Malloc Globel Memory Failed!", \
                                                __FILE__, __LINE__);
                                }
                        #endif
                        if(g_ppMFEDataSpace != NULL)
                        {
                                for(DDWORD i = 0; i < 30; i++)
                                {
                                        free(g_ppMFEDataSpace[i]);
                                }
                                free(g_ppMFEDataSpace);
                                g_ppMFEDataSpace = NULL;
                        }
                        if(g_pData != NULL)
                        {
                                free(g_pData);
                                g_pData = NULL;
                        }
                        return MFE_MEMALLOC_ERR;
                }

                g_pVADResult = (short *)malloc(sizeof(short) * DWORD(nSleep_Timeout * nSampleRate / nFrameLength));
                if(g_pVADResult == NULL)
                {
                        #if MFE_PLATFORM
                                if(iLogLevel == 7)
                                {
			                            ul_writelog(UL_LOG_FATAL, "[%s:%d] mfeInit: \
                                                Malloc Globel Memory Failed!", \
                                                __FILE__, __LINE__);
                                }
                        #endif
                        if(g_ppMFEDataSpace != NULL)
                        {
                                for(DDWORD i = 0; i < 30; i++)
                                {
                                        free(g_ppMFEDataSpace[i]);
                                }
                                free(g_ppMFEDataSpace);
                                g_ppMFEDataSpace = NULL;
                        }
                        if(g_pData != NULL)
                        {
                                free(g_pData);
                                g_pData = NULL;
                        }
                        if(g_pBVData != NULL)
                        {
                                free(g_pBVData);
                                g_pBVData = NULL;
                        }
                        return MFE_MEMALLOC_ERR;
                }

                /** @brief Memory Initialization */
                for(DWORD i = 0; i < 30; i++)
                {
                        for(DWORD j = 0; j < nFrameLength; j++)
                        {
                                g_ppMFEDataSpace[i][j] = 0.0;
                        }
                }

                for(DDWORD i = 0; i < nSleep_Timeout * nSampleRate; i++)
                {
                        g_pData[i] = 0;
                }

                for(DDWORD i = 0; i < nSleep_Timeout * nSampleRate / 4; i++)
                {
                        g_pBVData[i] = (UWord8)0;
                }

                for(DDWORD i = 0; i < DDWORD(nSleep_Timeout * nSampleRate / nFrameLength); i++)
                {
                        g_pVADResult[i] = 0;
                }

                /** @brief Calculate Subband Division */
                for(DWORD i = 0; i <= nSubbandNum; i++)
                {
                        g_ppMFEDataSpace[SubbandDivisionIdx][i] = nFrameLength * i / nSubbandNum;
                }

                /** @brief VAD&Compress Head Protocol */
                if(nCodeFormat == MFE_FORMAT_BV32_8K)
                {
                        g_pBVData[0] = 0;
                        g_pBVData[1] = 0;
                        g_pBVData[2] = 0;
                        g_pBVData[3] = 0;
                }
                else if(nCodeFormat == MFE_FORMAT_BV32_16K)
                {
                        g_pBVData[0] = 4;
                        g_pBVData[1] = 0;
                        g_pBVData[2] = 0;
                        g_pBVData[3] = 0;
                }
                else if(nCodeFormat == MFE_FORMAT_ADPCM_8K)
                {
                        g_pBVData[0] = 2;
                        g_pBVData[1] = 0;
                        g_pBVData[2] = 0;
                        g_pBVData[3] = 0;
                }
                else if(nCodeFormat == MFE_FORMAT_AMR_16K)
                {
                        g_pBVData[0] = 7;
                        g_pBVData[1] = 0;
                        g_pBVData[2] = 0;
                        g_pBVData[3] = 0;
                }
                else if(nCodeFormat == MFE_FORMAT_AMR_8K)
                {
                        g_pBVData[0] = 3;
                        g_pBVData[1] = 0;
                        g_pBVData[2] = 0;
                        g_pBVData[3] = 0;
                }
                else if(nCodeFormat == MFE_FORMAT_OPUS_8K)
                {
                        g_pBVData[0] = 64;
                        g_pBVData[1] = 0;
                        g_pBVData[2] = 0;
                        g_pBVData[3] = 0;
                }
                else if(nCodeFormat == MFE_FORMAT_OPUS_16K)
                {
                        g_pBVData[0] = 68;
                        g_pBVData[1] = 0;
                        g_pBVData[2] = 0;
                        g_pBVData[3] = 0;
                }
                else if(nCodeFormat == MFE_FORMAT_FEA_16K_2_3_30)
                {
                        g_pBVData[0] = 20;
                        g_pBVData[1] = 0;
                        g_pBVData[2] = 0;
                        g_pBVData[3] = 0;
                }
                else    /**< @brief 8k or 16k PCM */
                {
                        g_pBVData[0] = 1;
                        g_pBVData[1] = 0;
                        g_pBVData[2] = 0;
                        g_pBVData[3] = 0;
                        if(nSampleRate == 16000)
                        {
                                g_pBVData[0] = 5;
                        }
                }
                lBVStartLoc = 0;
                lBVCurLoc = 4;
                lVADResultStartLoc = 0;
                lVADResultCurLoc = 0;

								/** open**/
                /** @brief Memory Initialization */
                if((g_ppMFEDataSpace == NULL) || (g_pData == NULL) 
                        || (g_pBVData == NULL) || (g_pVADResult == NULL))
                {
                        return MFE_VAD_INIT_ERROR;
                }

                for(DWORD i = 0; i < 30; i++)
                {
                        for(DWORD j = 0; j < nFrameLength; j++)
                        {
                                g_ppMFEDataSpace[i][j] = 0.0;
                        }
                }

                for(DDWORD i = 0; i < nSleep_Timeout * nSampleRate; i++)
                {
                        g_pData[i] = 0;
                }

                for(DDWORD i = 4; i < nSleep_Timeout * nSampleRate / 4; i++)
                {
                        g_pBVData[i] = (UWord8)0;
                }

                for(DDWORD i = 0; i < DDWORD(nSleep_Timeout * nSampleRate / nFrameLength); i++)
                {
                        g_pVADResult[i] = 0;
                }

                /** @brief Parameters Initialization */

                nVADLastState = DWORD(SILENCE);
                lSample = 0;
                lSampleStart = 0;
                lSampleEnd = 0;
                lFrameCnt = 0;
                lFrameCntTotal = 0;
                nStartFrame = 0;
                nEndFrame = 0;
                dThr_InSpeech = dThreshold_Start;
                dThr_OutSpeech = dThreshold_End;
                nIsFinishFlag = 0;
                nOffsetLength = nOffset * nFrameLength;
                nVADInnerCnt = 0;
                nVADInnerZeroCnt = 0;
                nSpeechEndCnt = 0;
                nFindPossibleEndPoint = 0;

                /** @brief Calculate Subband Division */
                for(DWORD i = 0; i <= nSubbandNum; i++)
                {
                        g_ppMFEDataSpace[SubbandDivisionIdx][i] = nFrameLength * i / nSubbandNum;
                }

                lBVStartLoc = 0;
                lBVCurLoc = 4;
                lVADResultStartLoc = 0;
                lVADResultCurLoc = 0;

								/**start**/
								nVADLastState = DWORD(SILENCE);
                nVADCurState = DWORD(SILENCE);
                lSample = 0;
                lSampleStart = 0;
                lSampleEnd = 0;
                lFrameCnt = 0;
                lFrameCntTotal = 0;
                nStartFrame = 0;
                nEndFrame = 0;
                dThr_InSpeech = dThreshold_Start;
                dThr_OutSpeech = dThreshold_End;
                nIsFinishFlag = 0;
                nOffsetLength = nOffset * nFrameLength;
                nVADInnerCnt = 0;
                nVADInnerZeroCnt = 0;
                nSpeechEndCnt = 0;
                nFindPossibleEndPoint = 0;
                nPossible_Speech_Start = 0;
                bKeypadFiltering = false;
                bInvalidRecModification = false;

                dMaxSubEntro = -100;
                dMinSubEntro = 100; 

                /** @brief Memory Initialization */
                for(DWORD i = 0; i < 30; i++)
                {
                        for(DWORD j = 0; j < nFrameLength; j++)
                        {
                                g_ppMFEDataSpace[i][j] = 0.0;
                        }
                }

                for(DDWORD i = 0; i < nSleep_Timeout * nSampleRate; i++)
                {
                        g_pData[i] = 0;
                }

                for(DDWORD i = 4; i < nSleep_Timeout * nSampleRate / 4; i++)
                {
                        g_pBVData[i] = (UWord8)0;
                }

                for(DDWORD i = 0; i < DDWORD(nSleep_Timeout * nSampleRate / nFrameLength); i++)
                {
                        g_pVADResult[i] = 0;
                }

                /** @brief Calculate Subband Division */
                for(DWORD i = 0; i <= nSubbandNum; i++)
                {
                        g_ppMFEDataSpace[SubbandDivisionIdx][i] = nFrameLength * i / nSubbandNum;
                }

                lBVStartLoc = 0;
                lBVCurLoc = 4;
                lVADResultStartLoc = 0;
                lVADResultCurLoc = 0;

                /** @brief Reset BV32 Encoder */
                if(0)
                {
                        ;
                }
#ifdef  USING_BV32
                else if((nCodeFormat == MFE_FORMAT_BV32_8K) || (nCodeFormat == MFE_FORMAT_BV32_16K))
                {
                        frsz = FRSZ;
                        sizebitstream = sizeof(struct BV32_Bit_Stream);
                        sizestate = sizeof(struct BV32_Encoder_State);
                        state = allocEncoderState(0, sizestate/2 - 1);
                        Reset_BV32_Encoder((struct BV32_Encoder_State*)state);
                        bs = allocBitStream(0, sizebitstream/2 - 1);
                }
#endif
#ifdef  USING_AMR
                /** @brief Reset AMR Encoder */
                else if(nCodeFormat == MFE_FORMAT_AMR_16K)
                {
                        st = bdvr_amr::E_IF_init();
                }
#endif
#ifdef  USING_AMR_NB
                else if(nCodeFormat == MFE_FORMAT_AMR_8K)
                {
                        amr = Encoder_Interface_init(dtx);
                }
#endif
#ifdef  USING_OPUS
                else if(nCodeFormat == MFE_FORMAT_OPUS_8K || nCodeFormat == MFE_FORMAT_OPUS_16K)
                {
                        application = OPUS_APPLICATION_AUDIO;
                        sampling_rate = (opus_int32)nSampleRate;
                        channels = 1;
                        bitrate_bps = bitrate_bps_init;
                        frame_size = nSpeechEncLength;

                        use_vbr = 1;
                        bandwidth = OPUS_AUTO;
                        max_payload_bytes = MAX_PACKET;
                        complexity = 10;
                        use_inbandfec = 0;
                        forcechannels = OPUS_AUTO;
                        use_dtx = 0;
                        packet_loss_perc = 0;
                        max_frame_size = 960 * 6;
                        curr_read = 0;

                        enc = opus_encoder_create(sampling_rate, channels, application, &err);
                        if(err != OPUS_OK)
                        {
                                return MFE_ERROR_UNKNOWN;
                        }

                        opus_encoder_ctl(enc, OPUS_SET_BITRATE(bitrate_bps));
                        opus_encoder_ctl(enc, OPUS_SET_BANDWIDTH(bandwidth));
                        opus_encoder_ctl(enc, OPUS_SET_VBR(use_vbr));
                        opus_encoder_ctl(enc, OPUS_SET_VBR_CONSTRAINT(cvbr));
                        opus_encoder_ctl(enc, OPUS_SET_COMPLEXITY(complexity));
                        opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC(use_inbandfec));
                        opus_encoder_ctl(enc, OPUS_SET_FORCE_CHANNELS(forcechannels));
                        opus_encoder_ctl(enc, OPUS_SET_DTX(use_dtx));
                        opus_encoder_ctl(enc, OPUS_SET_PACKET_LOSS_PERC(packet_loss_perc));
                        opus_encoder_ctl(enc, OPUS_GET_LOOKAHEAD(&skip));
                        opus_encoder_ctl(enc, OPUS_SET_LSB_DEPTH(16));

                        switch(bandwidth)
                        {
                            case OPUS_BANDWIDTH_NARROWBAND:
                                bandwidth_string = "narrowband";
                                break;
                            case OPUS_BANDWIDTH_MEDIUMBAND:
                                bandwidth_string = "mediumband";
                                break;
                            case OPUS_BANDWIDTH_WIDEBAND:
                                bandwidth_string = "wideband";
                                break;
                            case OPUS_BANDWIDTH_SUPERWIDEBAND:
                                bandwidth_string = "superwideband";
                                break;
                            case OPUS_BANDWIDTH_FULLBAND:
                                bandwidth_string = "fullband";
                                break;
                            case OPUS_AUTO:
                                bandwidth_string = "auto";
                                break;
                            default:
                                bandwidth_string = "unknown";
                                break;
                        }

                        // Allocate Memory
                        in = (short*)malloc(max_frame_size * channels * sizeof(short));
                        out = (short*)malloc(max_frame_size * channels * sizeof(short));
                        fbytes = (unsigned char*)malloc(max_frame_size * channels * sizeof(short));
                        data[0] = (unsigned char*)calloc(max_payload_bytes, sizeof(char));
                        if(use_inbandfec)
                        {
                                data[1] = (unsigned char*)calloc(max_payload_bytes, sizeof(char));
                        }
                }
#endif
#ifdef  USING_BITMAP
                else if(nCodeFormat == MFE_FORMAT_FEA_16K_2_3_30)
                {
                        g_extractBitmap = new CAudioBitmap(1, 100, 7900, 4096, 1024, 16000, 10, 2.3, false, 30);
                        nPackID = 0;
                }
#endif
#ifdef  USING_BITMAP_SHAKE
                else if(nCodeFormat == MFE_FORMAT_FEA_16K_Shake)
                {
                        g_extractBitmap_shake = new CAudioBitmap(1, 100, 5000, 4096, 1024, 16000, 10, 2.0, false, 30);
                        nPackID_shake = 0;
                }
#endif
                else
                {
                        ;
                }
                if (nCarlife !=0)
                {
                    drc_handle = drc_create(); 
                    drc_init(drc_handle);
                }
                nCurState = (DWORD)RUN;
                return MFE_SUCCESS;
        }
        else
        {
                return MFE_STATE_ERR;
        }
}


int mfeExit()
{
        if(nCurState == (DWORD)RUN)
        {
                /** @brief Memory Clean */
                for(DWORD i = 0; i < 30; i++)
                {
                        for(DWORD j = 0; j < nFrameLength; j++)
                        {
                                g_ppMFEDataSpace[i][j] = 0.0;
                        }
                }

                for(DDWORD i = 0; i < nSleep_Timeout * nSampleRate; i++)
                {
                        g_pData[i] = 0;
                }

                for(DDWORD i = 4; i < nSleep_Timeout * nSampleRate / 4; i++)
                {
                        g_pBVData[i] = (UWord8)0;
                }

                for(DDWORD i = 0; i < DDWORD(nSleep_Timeout * nSampleRate / nFrameLength); i++)
                {
                        g_pVADResult[i] = 0;
                }

                /** @brief Parameters Initialization */

                nVADLastState = DWORD(SILENCE);
                nVADCurState = DWORD(SILENCE);
                lSample = 0;
                lSampleStart = 0;
                lSampleEnd = 0;
                lBVStartLoc = 0;
                lBVCurLoc = 4;
                lVADResultStartLoc = 0;
                lVADResultCurLoc = 0;
                lFrameCnt = 0;
                lFrameCntTotal = 0;
                nStartFrame = 0;
                nEndFrame = 0;
                nIsFinishFlag = 0;
                nOffsetLength = nOffset * nFrameLength;
                nVADInnerCnt = 0;
                nVADInnerZeroCnt = 0;
                nSpeechEndCnt = 0;
                nFindPossibleEndPoint = 0;
                bKeypadFiltering = false;

                dMaxSubEntro = -100;
                dMinSubEntro = 100; 

                dThr_InSpeech = dThreshold_Start;
                dThr_OutSpeech = dThreshold_End;

                /** @brief Free BV32 */
                if(0)
                {
                        ;
                }
#ifdef  USING_BV32
                else if((nCodeFormat == MFE_FORMAT_BV32_8K) || (nCodeFormat == MFE_FORMAT_BV32_16K))
                {
                        deallocEncoderState(state, 0, sizestate/2 - 1);
                        deallocBitStream(bs, 0, sizebitstream/2 - 1);
                }
#endif
#ifdef  USING_AMR
                /** @brief Free AMR */
                else if(nCodeFormat == MFE_FORMAT_AMR_16K)
                {
                        bdvr_amr::E_IF_exit(st);
                }
#endif
#ifdef  USING_AMR_NB
                else if(nCodeFormat == MFE_FORMAT_AMR_8K)
                {
                        Encoder_Interface_exit(amr);
                }
#endif
#ifdef  USING_OPUS
                else if(nCodeFormat == MFE_FORMAT_OPUS_8K || nCodeFormat == MFE_FORMAT_OPUS_16K)
                {
                        opus_encoder_destroy(enc);
                        free(data[0]);
                        if(use_inbandfec)
                        {
                                free(data[1]);
                        }
                        free(in);
                        free(out);
                        free(fbytes);
                }
#endif
#ifdef  USING_BITMAP
                else if(nCodeFormat == MFE_FORMAT_FEA_16K_2_3_30)
                {
                        delete g_extractBitmap;
                        nPackID = 0; 
                }
#endif
#ifdef  USING_BITMAP_SHAKE
                else if(nCodeFormat == MFE_FORMAT_FEA_16K_Shake)
                {
                        delete g_extractBitmap_shake;
                        nPackID_shake = 0; 
                }
#endif
                else
                {
                        ;
                }

                /**close**/
                /** @brief Memory Clean */
                for(DWORD i = 0; i < 30; i++)
                {
                        for(DWORD j = 0; j < nFrameLength; j++)
                        {
                                g_ppMFEDataSpace[i][j] = 0.0;
                        }
                }

                for(DDWORD i = 0; i < nSleep_Timeout * nSampleRate; i++)
                {
                        g_pData[i] = 0;
                }

                for(DDWORD i = 4; i < nSleep_Timeout * nSampleRate / 4; i++)
                {
                        g_pBVData[i] = (UWord8)0;
                }

                for(DDWORD i = 0; i < DDWORD(nSleep_Timeout * nSampleRate / nFrameLength); i++)
                {
                        g_pVADResult[i] = 0;
                }

                /** @brief Parameters Reset */

                nVADLastState = DWORD(SILENCE);
                lSample = 0;
                lSampleStart = 0;
                lSampleEnd = 0;
                lBVStartLoc = 0;
                lBVCurLoc = 4;
                lVADResultStartLoc = 0;
                lVADResultCurLoc = 0;
                lFrameCnt = 0;
                lFrameCntTotal = 0;
                nStartFrame = 0;
                nEndFrame = 0;
                dThr_InSpeech = dThreshold_Start;
                dThr_OutSpeech = dThreshold_End;
                nIsFinishFlag = 0;
                nOffsetLength = nOffset * nFrameLength;
                nVADInnerCnt = 0;
                nVADInnerZeroCnt = 0;
                nSpeechEndCnt = 0;
                nFindPossibleEndPoint = 0;
                
                /**exit**/
                /** @brief Free Memory */
                if(g_ppMFEDataSpace != NULL)
                {
                        for(DDWORD i = 0; i < 30; i++)
                        {
                                free(g_ppMFEDataSpace[i]);
                        }
                        free(g_ppMFEDataSpace);
                        g_ppMFEDataSpace = NULL;
                }
                if(g_pData != NULL)
                {
                        free(g_pData);
                        g_pData = NULL;
                }
                if(g_pBVData != NULL)
                {
                        free(g_pBVData);
                        g_pBVData = NULL;
                }
                if(g_pVADResult != NULL)
                {
                        free(g_pVADResult);
                        g_pVADResult = NULL;
                }

                /** @brief Parameters Reset */
                nVADLastState = DWORD(SILENCE);
                lSample = 0;
                lSampleStart = 0;
                lSampleEnd = 0;
                lBVStartLoc = 0;
                lBVCurLoc = 4;
                lVADResultStartLoc = 0;
                lVADResultCurLoc = 0;
                lFrameCnt = 0;
                lFrameCntTotal = 0;
                nStartFrame = 0;
                nEndFrame = 0;
                dThr_InSpeech = 0.0;
                dThr_OutSpeech = 0.0;
                nIsFinishFlag = 0;
                nOffsetLength = nOffset * nFrameLength;
                nVADInnerCnt = 0;
                nVADInnerZeroCnt = 0;
                nSpeechEndCnt = 0;
                nFindPossibleEndPoint = 0;

                nCurState = (DWORD)UNKNOWN;

                if (nCarlife !=0)
                {
                    // drc destroy
                    drc_destroy(drc_handle);
                }
                return MFE_SUCCESS;
        }
        else
        {
                return MFE_STATE_ERR;
        }
}


int mfeSendData(short* pDataIn, int iLen)
{
    if(nCurState == (DWORD)RUN)
    {
        pthread_mutex_lock(&MyMutex);
        
        /** @brief User Stop */
        if((pDataIn == NULL) && (iLen == 0))
        {
            nIsFinishFlag = 1;
                        
            if(nVAD)
            {
                nVADLastState = nVADCurState;
                nVADCurState = SPEECH_TO_SILENCE;
						}
						else
						{
								isNull = true;
						}						
            pthread_mutex_unlock(&MyMutex);
            usleep(THREAD_SLEEPTIME);
            return MFE_SUCCESS;
        }

        /* Band Trap 2014.1.17 */
        /*if(lSample + iLen < nSampleRate / 2)   // At least The First 1s
        {
            if(lSample == 0)
            {
                for(int ii = 0; ii < 5; ii++)
                {
                    fx[ii] = 0.0;
                }
                for(int ii = 0; ii < 4; ii++)
                {
                    fy[ii] = 0.0;
                }

                for(int ii = 4; ii < iLen; ii++)
                {
                    fx[0] = pDataIn[ii];
                    double dX, dY;
                    dX = 0; 
                    dY = 0;
                    for(int k = 0; k < 5; k++)
                    {
                        dX += fb[k] * fx[k];
                    }
                    for(int k = 0; k < 4; k++)
                    {
                        dY += fa[k + 1] * fy[k];
                    }
                    pDataIn[ii] = short(dX - dY);

                    for(int k = 0; k < 4; k++)
                    {
                        fx[4 - k] = fx[3 - k];
                    }
                    for(int k = 0; k < 3; k++)
                    {
                        fy[3 - k] = fy[2 - k];
                    }
                    fy[0] = dX - dY;
                }
            }
            else
            {
                    for(int ii = 0; ii < iLen; ii++)
                    {
                        fx[0] = pDataIn[ii];
                        double dX, dY;
                        dX = 0; 
                        dY = 0;
                        for(int k = 0; k < 5; k++)
                        {
                            dX += fb[k] * fx[k];
                        }
                        for(int k = 0; k < 4; k++)
                        {
                            dY += fa[k + 1] * fy[k];
                        }
                        pDataIn[ii] = short(dX - dY);

                        for(int k = 0; k < 4; k++)
                        {
                            fx[4 - k] = fx[3 - k];
                        }
                        for(int k = 0; k < 3; k++)
                        {
                            fy[3 - k] = fy[2 - k];
                        }
                        fy[0] = dX - dY;
                    }
            }
        }*/

        //FILE *fp = fopen("senddata.pcm", "ab");
       	//fwrite(pDataIn, sizeof(short), iLen, fp);
        //fclose(fp);
        /* Band Trap 2014.1.17 End */

        /* Filter Invalid Recording 2013.12.18 */
        if(nVAD && (nVADCurState == SILENCE))
        {
                int nZeroPass = 0;
                long lEnergy = 0;
                int nZeroPassRange = 0;
                
                if(lSample == 0)
                {
                    nZeroPassRange = iLen > 1600 ? 1600 : iLen;
                    for(int i = 11; i < nZeroPassRange; i++)
                    {
                        if((pDataIn[i - 1] < 0 && pDataIn[i] > 0) || 
                                (pDataIn[i - 1] > 0 && pDataIn[i] < 0))
                        {
                            nZeroPass++;
                        }
                        lEnergy = lEnergy + abs(pDataIn[i]);
                    }
                    
                    for(int i = 1; i < nZeroPassRange; i++)
                    {
                        lEnergy = lEnergy + abs(pDataIn[i]);
                    }
                    lEnergy = lEnergy / (nZeroPassRange - 1);
                    
                    if(nZeroPass < nThr_ZeroPass && lEnergy > nThr_Energy)
                    {
                        bInvalidRecModification = true;
                    }
                }
                //printf("nZeroPass: %d\t lEnergy: %d\n", nZeroPass, lEnergy);

                if(bInvalidRecModification)
                {
                    if(lSample < nModificationRange * nSampleRate / 1000)
                    {
                        int nStart = lSample;
                        int nEnd = lSample + iLen > nModificationRange * nSampleRate / 1000 ? 
                            nModificationRange * nSampleRate / 1000 : lSample + iLen;
                        
                        for(int i = nStart; i < nEnd; i++)
                        {
                            pDataIn[i - lSample] /= 128;
                        } 
                    }
                }
        }
        /* Filter Invalid Recording End 2013.12.18 */

        /** @brief Voice Search */
        if(nSpeech_Mode == 0)
        {
                /** @brief Smaller than 96K Once */
                if(iLen > 16000)        // 96 * 1024 / 2 (Samples)
                {
                    pthread_mutex_unlock(&MyMutex);
                    usleep(THREAD_SLEEPTIME);
                    return MFE_SEND_TOOMORE_DATA_ONCE;
                }

                /** @brief Send Data Too Long */
                if(lSample + iLen > nSleep_Timeout * nSampleRate)
                {
                    pthread_mutex_unlock(&MyMutex);
                    usleep(THREAD_SLEEPTIME);
                    return MFE_MEMALLOC_ERR;
                }

                /** @brief Consider Offset */
                /*if((lSample == 0) && (iLen > nOffsetLength))
                {
                    for(DWORD i = nOffsetLength; i < DWORD(iLen); i++)
                    {
                        g_pData[lSample + i - nOffsetLength] = pDataIn[i];
                    }
                    lSample += DDWORD(iLen - nOffsetLength);
                    nOffsetLength = 0;
                }
                else
                {
                    if(nOffsetLength == 0)
                    {
                        for(DWORD i = 0; i < DWORD(iLen); i++)
                        {
                            g_pData[lSample + i] = pDataIn[i];
                        }
                        lSample += (DDWORD)iLen;
                    }
                    else
                    {
                        lSample = 0;
                        nOffsetLength -= iLen;
                    }
                }*/

                /** @brief Do Not Consider Offset: 2012.11.27 */
                for(DWORD i = 0; i < DWORD(iLen); i++)
                {
                    g_pData[lSample + i] = pDataIn[i];
                }
                lSample += DDWORD(iLen);
                /** @brief Do Not Consider Offset: 2012.11.27 End */
				
                if(nVAD)
				{
                    /** @brief Obtain VAD Last State */
                    nVADLastState = nVADCurState;

                    /** @brief Filter Keypad tone in the First Pack (500ms) (songhui 2013.8.26) */
                    if(lSample >= nKeyToneRange * nSampleRate / 1000 && bKeypadFiltering == false)
                    {
                        bKeypadFiltering = true;

                        DWORD pAmpArray[360];
                        DWORD nMaxAmp = 0;
                        DWORD nMaxLoc = 0;
                        float fMaxAmpLoc = 0;
                        DWORD nStep = nKeyToneStep * (nSampleRate / 8000);
                        for(int ii = nKeyToneOffset * nSampleRate / 1000; 
                                ii < nKeyToneRange * nSampleRate / 1000; ii += nStep)
                        {
                            DWORD nAbsAmp = 0;
                            for(int jj = 0; jj < nStep; jj++)
                            {
                                nAbsAmp += abs(g_pData[ii + jj]);
                            }
                            nAbsAmp /= nStep;
                            pAmpArray[(ii - nKeyToneOffset * nSampleRate / 1000) / nStep] = nAbsAmp;
                            //printf("%d\n", pAmpArray[(ii - 50 * nSampleRate / 1000) / nStep]);

                            if(nAbsAmp > nMaxAmp)
                            {
                                nMaxAmp = nAbsAmp;
                                nMaxLoc = (ii - nKeyToneOffset * nSampleRate / 1000) / nStep;
                                fMaxAmpLoc = float((float)ii / nSampleRate);
                            }
                            //printf("%d: Amp: %d\n", ii, nMaxAmp);
                        }
                        //printf("Max Peak Loc: %f s (nMaxLoc: %d)\n", fMaxAmpLoc, nMaxLoc);

                        if(nMaxAmp > nKeyTonePeakThr)
                        {
                            bool bFilter = true;
                            DWORD nThrLeft = nKeyToneLeftThr;
                            DWORD nThrRight = nKeyToneRightThr;
                            DWORD nLeftLength = 0;
                            DWORD nRightLength = 0;
                            nLeftLength = (nKeyToneLeftRange * nSampleRate / 1000) / nStep < nMaxLoc ?
                                (nKeyToneLeftRange * nSampleRate / 1000) / nStep : nMaxLoc;
                            nRightLength = (nKeyToneRightRange * nSampleRate / 1000) / nStep < (360 - nMaxLoc) ?
                                  	          (nKeyToneRightRange * nSampleRate / 1000) / nStep : (360 - nMaxLoc);
	                        //printf("Left Length: %d \t Right Length: %d\n", nLeftLength, nRightLength);

  	                        for(int jj = nKeyToneDelta; jj < nLeftLength; jj++)
    	                    {
                                if(pAmpArray[nMaxLoc - jj] > nThrLeft)
        	                    {
                                    bFilter = false;
            	                    break;
              	                }
                	            bFilter = true;
                  	        }
                    	    for(int jj = nMaxLoc + nKeyToneDelta; jj < nRightLength; jj++)
                      	    {
                        	    if(pAmpArray[jj] > nThrRight)
                          	    {
                                    bFilter = false;
                              	    break;
                                }
                            }

                            // Begin to Filter
                            if(bFilter)
                            {
                                //printf("HAHA! Begin to Filter Keypad Tone!\n");
                                DWORD nFilterStartSample = (nMaxLoc - nKeyToneDelta) * nKeyToneStep 
                                    + nKeyToneOffset * nSampleRate / 1000;
                                DWORD nFilterEndSample = (nMaxLoc + nKeyToneDelta) * nKeyToneStep 
                                    + nKeyToneOffset * nSampleRate / 1000;
                                memcpy(&g_pData[nFilterStartSample], &g_pData[nFilterEndSample 
                                        + 100 * nSampleRate / 1000], 
                                        sizeof(short) * (nFilterEndSample - nFilterStartSample));

                                // Adjust lVADResultCurLoc
                                lVADResultCurLoc = 0;
                            }
                        }
                    }
                }
                
                /** @brief Filter Keypad tone in the First Pack (500ms) (songhui 2013.8.26) End */

                pthread_mutex_unlock(&MyMutex);
                usleep(THREAD_SLEEPTIME);
                return MFE_SUCCESS;
        }
        /** @brief Speech Input Method */
        else
        {
                /** @brief Smaller than 96K Once */
                if(iLen > 16000)
                {
                    pthread_mutex_unlock(&MyMutex);
                    usleep(THREAD_SLEEPTIME);
                    return MFE_SEND_TOOMORE_DATA_ONCE;
                }
												
				if(nVAD)
				{
					/** @brief Adjust lSample, if necessary */
                    if((nVADLastState == SILENCE_TO_SPEECH) && (nVADCurState == SPEECH_PAUSE))
                    {
                        if(lSample < nEndFrame * nFrameLength)
                        {
                            pthread_mutex_unlock(&MyMutex);
                            usleep(THREAD_SLEEPTIME);
                            return MFE_ERROR_UNKNOWN;
                        }

                        if(nSampleRate == 8000)
                        {
                            nBackEnd = 4;
                        }
                        else
                        {
                            nBackEnd = 8;
                        }
                        
                        /** @brief Move the Remaining Data */
                        for(DDWORD i = (nEndFrame - nBackEnd) * nFrameLength; i < lSample; i++)
                        {
                            g_pData[i - (nEndFrame - nBackEnd) * nFrameLength] = g_pData[i];
                        }
                        lSample = DDWORD(lSample - (nEndFrame - nBackEnd) * nFrameLength);

                        /** @brief Move the Remaining VADResult */
                        for(DDWORD i = nEndFrame - nBackEnd; i < lVADResultCurLoc; i++)
                        {
                            g_pVADResult[i - (nEndFrame - nBackEnd)] = g_pVADResult[i];
                        }

                        /** @brief Reset VADResult */
                        for(DDWORD i = lVADResultCurLoc - (nEndFrame - nBackEnd); 
                                i < DDWORD(nSleep_Timeout * nSampleRate / nFrameLength); i++)
                        {
                            g_pVADResult[i] = 0;
                        }

                        /** @brief Start From the Part that HAS NOT been processed */
                        lSampleStart = lSample;
                        lSampleEnd = lSample;
                        lVADResultCurLoc = lSample / VAD_FRAMELENGTH;
                        lVADResultStartLoc = lVADResultCurLoc;

                        /** @brief Send Data */
                        if(lSample + iLen > nSleep_Timeout * nSampleRate)
                        {
                            pthread_mutex_unlock(&MyMutex);
                            usleep(THREAD_SLEEPTIME);
                            return MFE_MEMALLOC_ERR;
                        }

                        for(DWORD i = 0; i < DWORD(iLen); i++)
                        {
                            g_pData[lSample + i] = pDataIn[i];
                        }
                        lSample += DDWORD(iLen);

                        lBVCurLoc = 4;
                        lBVStartLoc = 0;
#ifdef  USING_BV32
                        if(nCodeFormat == MFE_FORMAT_BV32_8K || nCodeFormat == MFE_FORMAT_BV32_16K)
                        {
                            Reset_BV32_Encoder((struct BV32_Encoder_State*)state);
                        }
#endif
#ifdef  USING_AMR
                        if(nCodeFormat == MFE_FORMAT_AMR_16K)
                        {
                            st = bdvr_amr::E_IF_init();
                        }
#endif
#ifdef  USING_AMR_NB
                        if(nCodeFormat == MFE_FORMAT_AMR_8K)
                        {
                            amr = Encoder_Interface_init(dtx);
                        }
#endif
#ifdef  USING_OPUS
                        if(nCodeFormat == MFE_FORMAT_OPUS_8K || nCodeFormat == MFE_FORMAT_OPUS_16K)
                        {
                            enc = opus_encoder_create(sampling_rate, channels, application, &err);
                        }
#endif
                    }
                    else
                    {
                        /** @brief Send Data */
                        if(lSample + iLen > nSleep_Timeout * nSampleRate)
                        {
                            pthread_mutex_unlock(&MyMutex);
                            usleep(THREAD_SLEEPTIME);
                            return MFE_MEMALLOC_ERR;
                        }

                        /** @brief Consider Offset */
                        if((nVADCurState == SILENCE) && (lSample == 0) &&
                        (iLen > nOffsetLength))
                        {
                            for(DWORD i = nOffsetLength; i < DWORD(iLen); i++)
                            {
                                g_pData[lSample + i - nOffsetLength] = pDataIn[i];
                            }
                            lSample += DDWORD(iLen - nOffsetLength);
                            nOffsetLength = 0;
                        }
                        else
                        {
                            if(nOffsetLength == 0)
                            {
                                for(DWORD i = 0; i < DWORD(iLen); i++)
                                {
                                    g_pData[lSample + i] = pDataIn[i];
                                }
                                lSample += (DDWORD)iLen;
                            }
                            else
                            {
                                lSample = 0;
                                nOffsetLength -= iLen;
                            }
                        }
                    }

                    nVADLastState = nVADCurState;
				}	
				else
				{
                    if(lSample + iLen > nSleep_Timeout * nSampleRate)
                    {
                        pthread_mutex_unlock(&MyMutex);
                        usleep(THREAD_SLEEPTIME);
                        return MFE_MEMALLOC_ERR;
                    }
                          
                    for(DWORD i = 0; i < DWORD(iLen); i++)
                    {
                        g_pData[lSample + i] = pDataIn[i];
                    }
                    lSample += (DDWORD)iLen;
                          
				}	
                        
                pthread_mutex_unlock(&MyMutex);
                usleep(THREAD_SLEEPTIME);
                return MFE_SUCCESS;
        }
    }
    else
    {
         return MFE_STATE_ERR;
    }
}


int mfeGetCallbackData(char* pDataOut, int iLen)
{      
        pthread_mutex_lock(&MyMutex);
        
        if(nVAD)
        {
            if((nVADCurState == SILENCE) || (nVADCurState == NO_SPEECH))
        	{
                pthread_mutex_unlock(&MyMutex);
                usleep(THREAD_SLEEPTIME);
                return 0;
        	}
       
        	/** @brief Multi GetCallbackData */
        	if(lSampleStart == lSampleEnd)
        	{
                pthread_mutex_unlock(&MyMutex);
                usleep(THREAD_SLEEPTIME);
                return 0;
        	}
		}
				
        /** @brief Voice Search */
        if(nSpeech_Mode == 0)
        {
            if(nVAD)
        	{
                /** @brief Adjust lSampleStart and lSampleEnd */
                if((nVADCurState == SILENCE_TO_SPEECH) && (nVADLastState == SILENCE))
                {
                    lSampleStart = nStartFrame * nFrameLength;
                    lSampleEnd = lSampleStart + DDWORD((lSample - lSampleStart) / nSpeechEncLength) 
                        * nSpeechEncLength;
                }
                if((nVADCurState == SPEECH_TO_SILENCE) && (nVADLastState == SILENCE_TO_SPEECH))
                {
                    if(nEndFrame * nFrameLength > lSampleStart)
                    {
                        lSampleEnd = lSampleStart + 
                            DDWORD((nEndFrame * nFrameLength - lSampleStart) / nSpeechEncLength) 
                            * nSpeechEncLength;
                    }
                    else
                    {
                        //lSampleEnd = lSampleStart + nSpeechEncLength;
                        lSampleEnd = lSampleStart;
                    }
                }
            }
			else
			{
                lSampleEnd = lSampleStart + DDWORD((lSample - lSampleStart) / nSpeechEncLength) 
                    * nSpeechEncLength;	
			}	
			if (!nCompress)
			{
				int iDataLength = (lSampleEnd-lSampleStart)*2;
				if(isFirst)
				{
                    if (iDataLength + 4 > iLen) {
                        pthread_mutex_unlock(&MyMutex);
                        usleep(THREAD_SLEEPTIME);
                        return -1;
                    }
                    
                    if(nSampleRate == 8000)
					{
							pDataOut[0] = 0x01;
					}
					else
					{
							pDataOut[0] = 0x05;
					}
					pDataOut[1] = 0x00;
					pDataOut[2] = 0x00;
					pDataOut[3] = 0x00;
					pDataOut+=4;
					isFirst = false;
                    memcpy(pDataOut, &g_pData[lSampleStart], iDataLength);
                    iDataLength += 4;
                } else {
                    if (iDataLength > iLen) {
                        pthread_mutex_unlock(&MyMutex);
                        usleep(THREAD_SLEEPTIME);
                        return -1;
                    }
                    memcpy(pDataOut, &g_pData[lSampleStart], iDataLength);
                }

				lSampleStart = lSampleEnd;
				pthread_mutex_unlock(&MyMutex);
				return iDataLength;
			}	
								
            /** @brief Compress Processing */
            int i;
            int nSegNum = 0;
            Word32 lReturnLength = 0;
            if(0)
            {
                ;
            }
#ifdef  USING_BV32
            else if((nCodeFormat == MFE_FORMAT_BV32_8K) || (nCodeFormat == MFE_FORMAT_BV32_16K))
            {
                Word16  *x;
                x = allocWord16(0, frsz - 1);

                nSegNum = int((lSampleEnd - lSampleStart) / frsz);

                /** @brief Start the Main Frame Loop */
                frame = 0;

                while(frame < nSegNum)
                {
                    /** @brief Frame Counter */
                    int iFrameLoc = frame * frsz + lSampleStart;
                    frame++;

                    /** @brief Read in One Frame Data */
                    for(i = 0; i < frsz; i++)
                    {
                        x[i] = g_pData[iFrameLoc + i];
                    }

                    /** @brief BV32 Coding */
                    BV32_Encode((struct BV32_Bit_Stream*)bs, (struct BV32_Encoder_State*)state, x);
                    BV32_BitPack(PackedStream, (struct BV32_Bit_Stream*)bs);

                    for(i = 0; i < 20; i++)
                    {
                        g_pBVData[lBVCurLoc + i] = PackedStream[i];
                    }
                    lBVCurLoc += 20;
                }

                deallocWord16(x, 0, frsz - 1);
            }
#endif
#ifdef  USING_AMR
            else if(nCodeFormat == MFE_FORMAT_AMR_16K)
            {
                nSegNum = int((lSampleEnd - lSampleStart) / nSpeechEncLength);
                frame = 0;

                while(frame < nSegNum)
                {
                    /** @brief Frame Counter */
                    int iFrameLoc = frame * nSpeechEncLength + lSampleStart;
                    frame++;

                    /** @brief Read in One Frame Data */
                    for(i = 0; i < nSpeechEncLength; i++)
                    {
                        signalwave[i] = g_pData[iFrameLoc + i];
                    }

                    /** @brief AMR Coding */
                    AMR_encode(st, (char*)signalwave, nSpeechEncLength * 2, (char*)serial, 
                            &lReturnLength, 1, coding_mode);

                    for(i = 0; i < lReturnLength; i++)
                    {
                        g_pBVData[lBVCurLoc + i] = serial[i];
                    }
                    lBVCurLoc += lReturnLength;
                }
            }
#endif
#ifdef  USING_AMR_NB
            else if(nCodeFormat == MFE_FORMAT_AMR_8K)
            {
                nSegNum = int((lSampleEnd - lSampleStart) / nSpeechEncLength);
                frame = 0;

                while(frame < nSegNum)
                {
                    int iFrameLoc = frame * nSpeechEncLength + lSampleStart;
                    frame++;

                    for(i = 0; i < nSpeechEncLength; i++)
                    {
                        buf[i] = g_pData[iFrameLoc + i];
                    }

                    lReturnLength = Encoder_Interface_Encode(amr, mode, buf, outbuf, 0);

                    for(i = 0; i < lReturnLength; i++)
                    {
                        g_pBVData[lBVCurLoc + i] = outbuf[i];
                    }
                    lBVCurLoc += lReturnLength;
                }
            }
#endif
#ifdef  USING_OPUS
            else if(nCodeFormat == MFE_FORMAT_OPUS_8K || nCodeFormat == MFE_FORMAT_OPUS_16K)
            {
                nSegNum = int((lSampleEnd - lSampleStart) / nSpeechEncLength);
                frame = 0;

                while(frame < nSegNum)
                {
                    int iFrameLoc = frame * nSpeechEncLength + lSampleStart;
                    frame++;

                    for(i = 0; i < nSpeechEncLength; i++)
                    {
                        in[i] = g_pData[iFrameLoc + i];
                    }

                    len[toggle] = opus_encode(enc, in, frame_size, data[toggle], max_payload_bytes);
                    opus_encoder_ctl(enc, OPUS_GET_FINAL_RANGE(&enc_final_range[toggle]));

                    if(len[toggle] < 0)
                    {
                        return -2;
                    }

                    curr_mode_count += frame_size;
                    if (curr_mode_count > mode_switch_time && curr_mode < nb_modes_in_list - 1)
                    {
                        curr_mode++;
                        curr_mode_count = 0;
                    }

                    lReturnLength = len[toggle] + 4 + 4;
                    unsigned char int_field[4];
                    int_to_char(len[toggle], int_field);
                    for(i = 0; i < 4; i++)
                    {
                        g_pBVData[lBVCurLoc + i] = int_field[i];
                    }
                    int_to_char(enc_final_range[toggle], int_field);
                    for(i = 0; i < 4; i++)
                    {
                        g_pBVData[lBVCurLoc + 4 + i] = int_field[i];
                    }
                    for(i = 0; i < len[toggle]; i++)
                    {
                        g_pBVData[lBVCurLoc + 8 + i] = data[toggle][i];
                    }
                    lBVCurLoc += lReturnLength;

                    lost_prev = lost;
                }
                        toggle = (toggle + use_inbandfec) & 1;
            }
#endif
#ifdef  USING_BITMAP
            if(nCodeFormat == MFE_FORMAT_FEA_16K_2_3_30)
            {
                nPackID++;

                int ret;
                short *pPCMData;
                UINT16 *pbitmap;
                int framenum;
                int frameFirst = 0;
                int frameLast = 0;

                pPCMData = g_extractBitmap->GetDataPtr();
                if(pPCMData == NULL)
                {
                    return -1;
                }
                g_extractBitmap->SetDataNum(nPackID * nSampleRate);
                memcpy(&pPCMData[(nPackID - 1) * nSampleRate], &g_pData[(nPackID - 1) * nSampleRate], nSampleRate * sizeof(short));

                ret = g_extractBitmap->ExtractBitmap_Online(nPackID, 14);
                if(ret == -2)
                {
                    pthread_mutex_unlock(&MyMutex);
                    usleep(THREAD_SLEEPTIME);
                    return -1;
                }

                pbitmap = g_extractBitmap->GetBitmap(framenum);
                g_pbitmap = pbitmap;

                ret = g_extractBitmap->GetPackBitmapPos(frameFirst, frameLast, 1);
                if((ret == -2) || (frameFirst < 0) || (frameLast < 0) || (frameLast < frameFirst))
                {
                    pthread_mutex_unlock(&MyMutex);
                    usleep(THREAD_SLEEPTIME);
                    return -1;
                }

                /** @brief Save bitmap */
                /*FILE *fp = fopen("data.bit", "ab");
                fwrite(&g_pbitmap[frameFirst * 125], sizeof(UINT16), (frameLast - frameFirst) * 125, fp);
                fclose(fp);*/

                lBVStartLoc = frameFirst * nByteSizePerFrame * 2;
                lBVCurLoc = frameLast * nByteSizePerFrame * 2;
            }
#endif
#ifdef  USING_BITMAP_SHAKE
            if(nCodeFormat == MFE_FORMAT_FEA_16K_Shake)
            {
                nPackID_shake++;

                int ret;
                short *pPCMData;
                UINT16 *pbitmap;
                int framenum;
                int frameFirst = 0;
                int frameLast = 0;

                pPCMData = g_extractBitmap_shake->GetDataPtr();
                if(pPCMData == NULL)
                {
                    return -1;
                }
                g_extractBitmap_shake->SetDataNum(nPackID_shake * nSampleRate);
                memcpy(&pPCMData[(nPackID_shake - 1) * nSampleRate], &g_pData[(nPackID_shake - 1) * nSampleRate], nSampleRate * sizeof(short));

                ret = g_extractBitmap_shake->ExtractBitmap_Online(nPackID_shake, 14);
                if(ret == -2)
                {
                    pthread_mutex_unlock(&MyMutex);
                    usleep(THREAD_SLEEPTIME);
                    return -1;
                }

                pbitmap = g_extractBitmap_shake->GetBitmap(framenum);
                g_pbitmap_shake = pbitmap;

                ret = g_extractBitmap_shake->GetPackBitmapPos(frameFirst, frameLast, 1);
                if((ret == -2) || (frameFirst < 0) || (frameLast < 0) || (frameLast < frameFirst))
                {
                    pthread_mutex_unlock(&MyMutex);
                    usleep(THREAD_SLEEPTIME);
                    return -1;
                }

                lBVStartLoc = frameFirst * nByteSizePerFrame_shake * 2;
                lBVCurLoc = frameLast * nByteSizePerFrame_shake * 2;
            }
#endif
            else
            {
                ;
            }

            /** @brief If iLen is TOO SMALL */
            if(lBVCurLoc - lBVStartLoc > iLen)
            {
                pthread_mutex_unlock(&MyMutex);
                usleep(THREAD_SLEEPTIME);
                return -1;
            }

            lSampleStart = lSampleEnd;

            if(0)
            {
                ;
            }
#ifdef  USING_BITMAP
            else if(nCodeFormat == MFE_FORMAT_FEA_16K_2_3_30)
            {
                int iHeaderLength = 0;
                int iBodyLength = 0;
                int frameFirst, frameLast;
                int fct;
                int tmpNf = 0;
                int fctOffset = 0;
                int ectOffset = 0;
                int nFFlag = 0;
                frameFirst = lBVStartLoc / nByteSizePerFrame / 2;
                frameLast = lBVCurLoc / nByteSizePerFrame / 2;
                if(nPackID == 1)
                {
                    pDataOut[0] = g_pBVData[0];
                    pDataOut[1] = g_pBVData[1];
                    pDataOut[2] = g_pBVData[2];
                    pDataOut[3] = g_pBVData[3];
                    pDataOut[4] = (char)nPackID;
                    *(short*)(&pDataOut[5]) = short(lBVStartLoc / nByteSizePerFrame / 2);
                    *(short*)(&pDataOut[7]) = short(lBVCurLoc / nByteSizePerFrame / 2);
                    *(short*)(&pDataOut[9]) = short(100);
                    iHeaderLength = 11;

                    while(nFFlag == 0 && ectOffset < 4 && fctOffset < 2)
                    {
                        fct = 0;
                        for(int frameId = frameFirst; frameId <= frameLast; frameId++)
                        {
                            if(nFFlag == 1)
                            {
                                break;
                            }
                            if(frameId >= 0)
                            {
                                if((fct + fctOffset) % 2 != 0)
                                {
                                    fct++;
                                    continue;
                                }
                                for(int ect = nFeatureDimPerFrame - 1; ect >= 0; ect--)
                                {
                                    if((ect + ectOffset) % 4 != 0)
                                    {
                                        continue;
                                    }
                                    int byteId = (int)(((float)ect) / nBitSizePerUnit);
                                    int bytePos = ect - nBitSizePerUnit * byteId;
                                    UINT16 feavec = g_pbitmap[frameId * nByteSizePerFrame + byteId];
                                    char bitval = (feavec >> bytePos) & 0x0001;
                                    if(bitval)
                                    {
                                        tmpNf++;
                                        if(tmpNf > nMaxFeatureDensity)
                                        {
                                            nFFlag = 1;
                                            break;
                                        }

                                        // Save bitmap for package
                                        pDataOut[iHeaderLength + iBodyLength] = 
                                            (char)((int)ect % 8 + (frameId - frameFirst) * 8);  // X
                                        iBodyLength++;        
                                        pDataOut[iHeaderLength + iBodyLength] = (char)((int)ect / 8);  // Y Coordinate
                                        iBodyLength++;
                                    }
                                }
                            }
                            
                            fct++;
                        }
                        if(fctOffset == 1)
                        {
                            ectOffset++;
                            fctOffset = 0;
                        }
                        else
                        {
                            fctOffset++;
                        }
                    }
                            
                    // Write DateLength in the Package
                    *(short*)(&pDataOut[iHeaderLength - 2]) = short(iBodyLength);
                }
                else
                {
                    pDataOut[0] = (char)nPackID;
                    *(short*)(&pDataOut[1]) = short(lBVStartLoc / nByteSizePerFrame / 2);
                    *(short*)(&pDataOut[3]) = short(lBVCurLoc / nByteSizePerFrame / 2);
                    *(short*)(&pDataOut[5]) = short(100);
                    iHeaderLength = 7;

                    while(nFFlag == 0 && ectOffset < 4 && fctOffset < 2)
                    {
                        fct = 0;
                        for(int frameId = frameFirst; frameId <= frameLast; frameId++)
                        {
                            if(nFFlag == 1)
                            {
                                break;
                            }
                            if(frameId >= 0)
                            {
                                if((fct + fctOffset) % 2 != 0)
                                {
                                    fct++;
                                    continue;
                                }
                                for(int ect = nFeatureDimPerFrame - 1; ect >= 0; ect--)
                                {
                                    if((ect + ectOffset) % 4 != 0)
                                    {
                                        continue;
                                    }
                                    
                                    int byteId = (int)(((float)ect) / nBitSizePerUnit);
                                    int bytePos = ect - nBitSizePerUnit * byteId;
                                    UINT16 feavec = g_pbitmap[frameId * nByteSizePerFrame + byteId];
                                    char bitval = (feavec >> bytePos) & 0x0001;
                                    if(bitval)
                                    {
                                        tmpNf++;
                                        if(tmpNf > nMaxFeatureDensity)
                                        {
                                            nFFlag = 1;
                                            break;
                                        }

                                        // Save bitmap for package
                                        pDataOut[iHeaderLength + iBodyLength] = (char)((int)ect % 8 + 
                                                (frameId - frameFirst) * 8);  // X
                                        iBodyLength++;        
                                        pDataOut[iHeaderLength + iBodyLength] = 
                                            (char)((int)ect / 8);  // Y Coordinate
                                        iBodyLength++;
                                    }
                                }
                            }
                            fct++;
                        }
                        if(fctOffset == 1)
                        {
                            ectOffset++;
                            fctOffset = 0;
                        }
                        else
                        {
                            fctOffset++;
                        }
                    }
                            
                    // Write DateLength in the Package
                    *(short*)(&pDataOut[iHeaderLength - 2]) = short(iBodyLength);
                }
                    
                pthread_mutex_unlock(&MyMutex);
                usleep(THREAD_SLEEPTIME);
                return iHeaderLength + iBodyLength;
            }
#endif
#ifdef  USING_BITMAP_SHAKE
            else if(nCodeFormat == MFE_FORMAT_FEA_16K_Shake)
            {
                int iHeaderLength = 0;
                int iBodyLength = 0;
                int frameFirst, frameLast;
                int fct;
                int tmpNf = 0;
                int fctOffset = 0;
                int ectOffset = 0;
                int nFFlag = 0;
                frameFirst = lBVStartLoc / nByteSizePerFrame_shake / 2;
                frameLast = lBVCurLoc / nByteSizePerFrame_shake / 2;
                if(nPackID == 1 || 1)
                {
                    pDataOut[0] = g_pBVData[0];
                    pDataOut[1] = g_pBVData[1];
                    pDataOut[2] = g_pBVData[2];
                    pDataOut[3] = g_pBVData[3];
                    *(int*)(&pDataOut[4]) = nPackID_shake;
                    *(int*)(&pDataOut[8]) = short(lBVStartLoc / nByteSizePerFrame_shake / 2);
                    *(int*)(&pDataOut[12]) = short(lBVCurLoc / nByteSizePerFrame_shake / 2);
                    *(short*)(&pDataOut[16]) = short(100);
                    iHeaderLength = 18;

                    while(nFFlag == 0 && ectOffset < 4 && fctOffset < 2)
                    {
                        fct = 0;
                        for(int frameId = frameFirst; frameId <= frameLast; frameId++)
                        {
                            if(nFFlag == 1)
                            {
                                break;
                            }
                            if(frameId >= 0)
                            {
                                if((fct + fctOffset) % 2 != 0)
                                {
                                    fct++;
                                    continue;
                                }
                                for(int ect = nFeatureDimPerFrame_shake - 1; ect >= 0; ect--)
                                {
                                    if((ect + ectOffset) % 4 != 0)
                                    {
                                        continue;
                                    }
                                    int byteId = (int)(((float)ect) / nBitSizePerUnit_shake);
                                    int bytePos = ect - nBitSizePerUnit_shake * byteId;
                                    UINT16 feavec = g_pbitmap_shake[frameId * nByteSizePerFrame_shake + byteId];
                                    char bitval = (feavec >> bytePos) & 0x0001;
                                    if(bitval)
                                    {
                                        tmpNf++;
                                        if(tmpNf > nMaxFeatureDensity_shake)
                                        {
                                            nFFlag = 1;
                                            break;
                                        }

                                        // Save bitmap for package
                                        pDataOut[iHeaderLength + iBodyLength] = 
                                            (char)((int)ect % 8 + (frameId - frameFirst) * 8);  // X
                                        iBodyLength++;        
                                        pDataOut[iHeaderLength + iBodyLength] = (char)((int)ect / 8);  // Y Coordinate
                                        iBodyLength++;
                                    }
                                }
                            }
                            
                            fct++;
                        }
                        if(fctOffset == 1)
                        {
                            ectOffset++;
                            fctOffset = 0;
                        }
                        else
                        {
                            fctOffset++;
                        }
                    }
                            
                    // Write DateLength in the Package
                    *(short*)(&pDataOut[iHeaderLength - 2]) = short(iBodyLength);
                }
                else
                {
                    ;
                }
                    
                pthread_mutex_unlock(&MyMutex);
                usleep(THREAD_SLEEPTIME);
                return iHeaderLength + iBodyLength;
            }
#endif
            else
            {
                if (nVAD)
                {
                    if((nVADCurState == SILENCE_TO_SPEECH) && (0))
                    {
                        int iDataLength = int((lBVCurLoc - lBVStartLoc) / 640) * 640;

                        for(i = 0; i < iDataLength; i++)
                        {
                            pDataOut[i] = char(g_pBVData[lBVStartLoc + i]);
                        }
                        lBVStartLoc += iDataLength;

                        pthread_mutex_unlock(&MyMutex);
                        usleep(THREAD_SLEEPTIME);
                        return iDataLength;
                    }
                    if((nVADCurState == SPEECH_TO_SILENCE) || (nVADCurState == SILENCE_TO_SPEECH))
                    {
                        int iDataLength = int(lBVCurLoc - lBVStartLoc);

                        for(i = 0; i < iDataLength; i++)
                        {
                            pDataOut[i] = char(g_pBVData[lBVStartLoc + i]);
                        }
                        lBVStartLoc += iDataLength;

                        pthread_mutex_unlock(&MyMutex);
                        usleep(THREAD_SLEEPTIME);
                        return iDataLength;
                    }
                    else
                    {
                        pthread_mutex_unlock(&MyMutex);
                        usleep(THREAD_SLEEPTIME);
                        return 0;
                    }
                }
                else
                {
                    int iDataLength = int(lBVCurLoc - lBVStartLoc);

                    for(i = 0; i < iDataLength; i++)
                    {
                        pDataOut[i] = char(g_pBVData[lBVStartLoc + i]);
                    }
                    lBVStartLoc += iDataLength;

                    pthread_mutex_unlock(&MyMutex);
                    usleep(THREAD_SLEEPTIME);
                    return iDataLength;
                }	
            }
        }
        /** @brief Speech Input Method */
        else
        {
            if(nVAD)
            {
                if((nVADCurState == SPEECH_PAUSE) && (nVADLastState == SPEECH_PAUSE))
                {
                    pthread_mutex_unlock(&MyMutex);
                    usleep(THREAD_SLEEPTIME);
                    return 0;
                }

                if((nVADCurState == SPEECH_TO_SILENCE) && ((nVADLastState == SPEECH_PAUSE) || 
                            (nVADLastState == SPEECH_TO_SILENCE)))
                {
                    pthread_mutex_unlock(&MyMutex);
                    usleep(THREAD_SLEEPTIME);
                    return 0;
               	}

               	/** @brief Adjust lSampleStart and lSampleEnd */
                if((nVADCurState == SILENCE_TO_SPEECH) && (nVADLastState == SILENCE))
                {
                    lSampleStart = nStartFrame * nFrameLength;
                    lSampleEnd = lSampleStart + DDWORD((lSample - lSampleStart) / nSpeechEncLength) 
                        * nSpeechEncLength;
               	}
                if((nVADCurState == SPEECH_PAUSE) && (nVADLastState == SILENCE_TO_SPEECH))
                {
                    if(nEndFrame * nFrameLength > lSampleStart)
                    {
                        lSampleEnd = lSampleStart + 
                            DDWORD((nEndFrame * nFrameLength - lSampleStart) / nSpeechEncLength) 
                            * nSpeechEncLength;
                    }
                    else
                    {
                        lSampleEnd = lSampleStart + nSpeechEncLength;
                    }
                }
               	if((nVADCurState == SILENCE_TO_SPEECH) && (nVADLastState == SPEECH_PAUSE))
                {
                    lSampleStart = nStartFrame * nFrameLength;
                    lSampleEnd = lSampleStart + DDWORD((lSample - lSampleStart) / nSpeechEncLength) 
                        * nSpeechEncLength;
                }
			}
			else
			{
					lSampleEnd = lSampleStart + DDWORD((lSample - lSampleStart) / nSpeechEncLength) 
                        * nSpeechEncLength;
			}
			if (!nCompress)
			{
				int iDataLength = (lSampleEnd-lSampleStart)*2;
				if(isFirst)
				{
                    if (iDataLength + 4 > iLen) {
                        pthread_mutex_unlock(&MyMutex);
                        usleep(THREAD_SLEEPTIME);
                        return -1;
                    }
                    
                    if(nSampleRate == 8000)
					{
							pDataOut[0] = 0x01;
					}
					else
					{
							pDataOut[0] = 0x05;
					}
					pDataOut[1] = 0x00;
					pDataOut[2] = 0x00;
					pDataOut[3] = 0x00;
					pDataOut+=4;
					isFirst = false;
                    memcpy(pDataOut, &g_pData[lSampleStart], iDataLength);
                    iDataLength += 4;
                } else {
                    if (iDataLength > iLen) {
                        pthread_mutex_unlock(&MyMutex);
                        usleep(THREAD_SLEEPTIME);
                        return -1;
                    }
                    
                    memcpy(pDataOut, &g_pData[lSampleStart], iDataLength);
                }
                
				lSampleStart = lSampleEnd;
				pthread_mutex_unlock(&MyMutex);
				return iDataLength;
			}
            /** @brief Compress Processing */
            int     i;
            int     nSegNum = 0;
            Word32  lReturnLength = 0;
            if(0)
            {
                ;
            }
#ifdef  USING_BV32
            else if((nCodeFormat == MFE_FORMAT_BV32_8K) || (nCodeFormat == MFE_FORMAT_BV32_16K))
            {
                Word16  *x;
                x = allocWord16(0, frsz - 1);

                nSegNum = int((lSampleEnd - lSampleStart) / frsz);

                /** @brief Start the Main Frame Loop */
                frame = 0;

                while(frame < nSegNum)
                {
                    /** @brief Frame Counter */
                    int iFrameLoc = frame * frsz + lSampleStart;
                    frame++;

                    /** @brief Read in One Frame Data */
                    for(i = 0; i < frsz; i++)
                    {
                        x[i] = g_pData[iFrameLoc + i];
                    }

                    /** @brief BV32 Coding */
                    BV32_Encode((struct BV32_Bit_Stream*)bs, (struct BV32_Encoder_State*)state, x);
                    BV32_BitPack(PackedStream, (struct BV32_Bit_Stream*)bs);

                    for(i = 0; i < 20; i++)
                    {
                        g_pBVData[lBVCurLoc + i] = PackedStream[i];
                    }
                    lBVCurLoc += 20;
                }

                deallocWord16(x, 0, frsz - 1);
            }
#endif
#ifdef  USING_AMR
            else if(nCodeFormat == MFE_FORMAT_AMR_16K)
            {
                nSegNum = int((lSampleEnd - lSampleStart) / nSpeechEncLength);
                frame = 0;

                while(frame < nSegNum)
                {
                    /** @brief Frame Counter */
                    int iFrameLoc = frame * nSpeechEncLength + lSampleStart;
                    frame++;

                    /** @brief Read in One Frame Data */
                    for(i = 0; i < nSpeechEncLength; i++)
                    {
                        signalwave[i] = g_pData[iFrameLoc + i];
                    }

                    /** @brief AMR Coding */
                    AMR_encode(st, (char*)signalwave, nSpeechEncLength * 2, (char*)serial,
                            &lReturnLength, 1, coding_mode);

                    for(i = 0; i < lReturnLength; i++)
                    {
                        g_pBVData[lBVCurLoc + i] = serial[i];
                    }
                    lBVCurLoc += lReturnLength;
                }
            }
#endif  
#ifdef  USING_AMR_NB
            else if(nCodeFormat == MFE_FORMAT_AMR_8K)
            {
                nSegNum = int((lSampleEnd - lSampleStart) / nSpeechEncLength);
                frame = 0;

                while(frame < nSegNum)
                {
                    int iFrameLoc = frame * nSpeechEncLength + lSampleStart;
                    frame++;

                    for(i = 0; i < nSpeechEncLength; i++)
                    {
                        buf[i] = g_pData[iFrameLoc + i];
                    }

                    lReturnLength = Encoder_Interface_Encode(amr, mode, buf, outbuf, 0);

                    for(i = 0; i < lReturnLength; i++)
                    {
                        g_pBVData[lBVCurLoc + i] = outbuf[i];
                    }
                    lBVCurLoc += lReturnLength;
                }
            }
#endif
#ifdef  USING_OPUS
            else if(nCodeFormat == MFE_FORMAT_OPUS_8K || nCodeFormat == MFE_FORMAT_OPUS_16K)
            {
                nSegNum = int((lSampleEnd - lSampleStart) / nSpeechEncLength);
                frame = 0;

                while(frame < nSegNum)
                {
                    int iFrameLoc = frame * nSpeechEncLength + lSampleStart;
                    frame++;

                    for(i = 0; i < nSpeechEncLength; i++)
                    {
                        in[i] = g_pData[iFrameLoc + i];
                    }

                    len[toggle] = opus_encode(enc, in, frame_size, data[toggle], max_payload_bytes);
                    opus_encoder_ctl(enc, OPUS_GET_FINAL_RANGE(&enc_final_range[toggle]));

                    if(len[toggle] < 0)
                    {
                        return -2;
                    }
                    curr_mode_count += frame_size;
                    if (curr_mode_count > mode_switch_time && curr_mode < nb_modes_in_list - 1)
                    {
                        curr_mode++;
                        curr_mode_count = 0;
                    }

                    lReturnLength = len[toggle] + 4 + 4;
                    unsigned char int_field[4];
                    int_to_char(len[toggle], int_field);
                    for(i = 0; i < 4; i++)
                    {
                        g_pBVData[lBVCurLoc + i] = int_field[i];
                    }
                    int_to_char(enc_final_range[toggle], int_field);
                    for(i = 0; i < 4; i++)
                    {
                        g_pBVData[lBVCurLoc + 4 + i] = int_field[i];
                    }
                    for(i = 0; i < len[toggle]; i++)
                    {
                        g_pBVData[lBVCurLoc + 8 + i] = data[toggle][i];
                    }
                    lBVCurLoc += lReturnLength;

                    lost_prev = lost;
                }
                toggle = (toggle + use_inbandfec) & 1;
            }
#endif
            else
            {
                ;
            }

            /** @brief If iLen is TOO SMALL */
            if(lBVCurLoc - lBVStartLoc > iLen)
            {

                pthread_mutex_unlock(&MyMutex);
                usleep(THREAD_SLEEPTIME);
                return -1;
            }

            lSampleStart = lSampleEnd;
                
            if(nVAD)
			{
                if((nVADCurState == SILENCE_TO_SPEECH) && (0))
                {	
                    int iDataLength = int((lBVCurLoc - lBVStartLoc) / 640) * 640;

                    for(i = 0; i < iDataLength; i++)
                    {
                        pDataOut[i] = char(g_pBVData[lBVStartLoc + i]);
                    }
                    lBVStartLoc += iDataLength;

                    pthread_mutex_unlock(&MyMutex);
                    usleep(THREAD_SLEEPTIME);
                    return iDataLength;
               	}
                else if((nVADCurState == SPEECH_PAUSE) || (nVADCurState == SILENCE_TO_SPEECH))
                {				
                				
                    int iDataLength = int(lBVCurLoc - lBVStartLoc);

                    for(i = 0; i < iDataLength; i++)
                    {
                        pDataOut[i] = char(g_pBVData[lBVStartLoc + i]);
                    }
                    lBVStartLoc += iDataLength; /**< @brief Now lBVStartLoc == lBVCurLoc */
                        
                    //Remove Header of SubSentence 2014.3.21
                    if(nModeComb != 0)
                    {
                        if(nVADLastState == SPEECH_PAUSE && nVADCurState == SILENCE_TO_SPEECH)
                        {
                            for(i = 0; i < iDataLength - 4; i++)
                            {
                                pDataOut[i] = pDataOut[i + 4];
                            }
                            iDataLength -= 4;
                        }
                    }
                    //Remove Header of SubSentence 2014.3.21 End

                    pthread_mutex_unlock(&MyMutex);
                    usleep(THREAD_SLEEPTIME);
                    return iDataLength;
                }
                else if((nVADCurState == SPEECH_TO_SILENCE) && (nVADLastState == SILENCE_TO_SPEECH))
                {
                    int iDataLength = int(lBVCurLoc - lBVStartLoc);

                    for(i = 0; i < iDataLength; i++)
                    {
                        pDataOut[i] = char(g_pBVData[lBVStartLoc + i]);
                    }
                    lBVStartLoc += iDataLength; /**< @brief Now lBVStartLoc == lBVCurLoc */

                    pthread_mutex_unlock(&MyMutex);
                    usleep(THREAD_SLEEPTIME);
                    return iDataLength;
                }
                else
                {
                    pthread_mutex_unlock(&MyMutex);
                    usleep(THREAD_SLEEPTIME);
                    return 0;
                }
            }
            else
            {
                int iDataLength = int(lBVCurLoc - lBVStartLoc);

                for(i = 0; i < iDataLength; i++)
                {
                    pDataOut[i] = char(g_pBVData[lBVStartLoc + i]);
                }
                lBVStartLoc += iDataLength; /**< @brief Now lBVStartLoc == lBVCurLoc */

                pthread_mutex_unlock(&MyMutex);
                usleep(THREAD_SLEEPTIME);
                return iDataLength;
            }	
        }
}


int mfeDetect()
{        
        if(nCurState == (DWORD)RUN)
        {
                pthread_mutex_lock(&MyMutex);

								if(!nVAD)
								{				
												int detectflag = RET_SILENCE_TO_SPEECH ;
												
												if(isNull)
												{
																detectflag = RET_SPEECH_TO_SILENCE;				
												}	
												
												pthread_mutex_unlock(&MyMutex);
                        usleep(THREAD_SLEEPTIME);
												return detectflag;
												
								}			

                /** @brief User Stop Voluntarily */
                if(nIsFinishFlag == 1)
                {
                        lSampleStart = lSampleEnd;
                        if((nVADLastState == SPEECH_PAUSE) || (nVADLastState == SILENCE))
                        {
                                lSampleEnd = lSampleStart;
                        }
                        else
                        {
                                lSampleEnd = lSampleStart +
                                    DDWORD((lSample - lSampleStart) / nSpeechEncLength) * nSpeechEncLength;
                        }

                        nEndFrame = DWORD(lSample / nFrameLength);
                        nIsFinishFlag = 0;
                        nFindPossibleEndPoint = 0;

                        if(nVADLastState == DWORD(SILENCE))
                        { 
                                pthread_mutex_unlock(&MyMutex);
                                usleep(THREAD_SLEEPTIME);
                                return RET_NO_SPEECH;
                        }
                        else if((nVADLastState == SILENCE_TO_SPEECH) &&
                                (nEndFrame - nStartFrame < nMin_Speech_Duration))
                        {
                                pthread_mutex_unlock(&MyMutex);
                                usleep(THREAD_SLEEPTIME);
                                return RET_SPEECH_TOO_SHORT;
                        }
                        else if((nVADLastState == SILENCE_TO_SPEECH) && 
                                (nEndFrame - nStartFrame > nMax_Speech_Duration - 10))
                        {
                                pthread_mutex_unlock(&MyMutex);
                                usleep(THREAD_SLEEPTIME);
                                return RET_SPEECH_TOO_LONG;
                        }
                        else    // Usually
                        {
                                pthread_mutex_unlock(&MyMutex);
                                usleep(THREAD_SLEEPTIME);
                                return RET_SPEECH_TO_SILENCE;
                        }
                }

                /** @brief Voice Search */
                if(nSpeech_Mode == 0)
                {
                        DDWORD i;
                        short sVADResult = 0;       /**< @brief Result of each frame */
                        short sResultSum = 0;
                        short sResultSum_PossibleEp = 0;
                        DDWORD lClippingNum = 0;    /**< @brief Over-amplitude Number */
                        short pFrameData[VAD_FRAMELENGTH];

                        /** @brief Calculate Subband Entropy Feature and Save it */
                        lVADResultStartLoc = lVADResultCurLoc;
                        lVADResultCurLoc = DDWORD(lSample / VAD_FRAMELENGTH);
                        lFrameCnt = lVADResultStartLoc;

                        /** @brief No Data Arrived, Do Nothing */
                        if(lVADResultStartLoc == lVADResultCurLoc)
                        {
                                goto RET;
                        }

                        while(lFrameCnt < lVADResultCurLoc)
                        {
                                /** @brief Obtain One-Frame Speech Data */
                                DDWORD lTemp = lFrameCnt * VAD_FRAMELENGTH;
                                for(i = 0; i < VAD_FRAMELENGTH; i++)
                                {
                                        pFrameData[i] = g_pData[lTemp + i];

                                        // brief Clipping Detection
                                        if(abs(pFrameData[i]) > 32760)
                                        {
                                                lClippingNum++;
                                        }
                                }
                                /** @brief HighPass filtering and DRC 20150729 */
                                if (nCarlife != 0)
                                {
                                        short pFrameData_DRC[VAD_FRAMELENGTH];
                                        drc_process(drc_handle, pFrameData, pFrameData_DRC);
                                        memcpy(pFrameData, pFrameData_DRC, VAD_FRAMELENGTH * sizeof(short));
                                }
                                /** @brief Adjust VAD Threshold if StartPoint is detected but
                                 * VADState is SILENCE */
                                nVADCurState = (nVADInnerCnt == nIn_Speech_Threshold) ? 
                                    SILENCE_TO_SPEECH : SILENCE;

                                sVADResult = VAD_SubbandEntropy_Process(pFrameData);
                                //sVADResult = VAD_SubbandEntropy_Process(&g_pData[lFrameCnt * VAD_FRAMELENGTH]);
                                
                                if(nVADInnerCnt < nIn_Speech_Threshold)
                                {
                                        nVADInnerCnt = (nVADInnerCnt + sVADResult) * sVADResult;
                                }

                                /** @brief Save VAD Frame-Result */
                                g_pVADResult[lFrameCnt] = sVADResult;
                                //printf("FrameCnt: %ld\t VADResult: %d\t VADState: %d\n", lFrameCnt, sVADResult, nVADCurState);
                                lFrameCnt++;

                                if(sVADResult == 1)
                                {
                                        nPossible_Speech_Start = 1;
                                }
                                else
                                {
                                        nPossible_Speech_Start = 0;
                                }
                        }
                        //printf("$$$$$$$$$$$$$$\n");

                        /** @brief VAD Detect */
                        if(lSampleStart == 0)
                        {
                                nVADCurState = SILENCE;
                                lFrameCnt = lVADResultStartLoc;
                        }
                        else
                        {
                                nVADCurState = nVADLastState;
                                if(lVADResultStartLoc > nIn_Speech_Threshold)
                                {
                                        if(nVADCurState == SILENCE)
                                        {
                                                lFrameCnt = lVADResultStartLoc - nIn_Speech_Threshold;
                                        }
                                        else if(nVADCurState == SILENCE_TO_SPEECH)
                                        {
                                                if(lVADResultStartLoc > nMax_Speech_Pause)
                                                {
                                                        lFrameCnt = lVADResultStartLoc - nMax_Speech_Pause;
                                                }
                                                else
                                                {
                                                        //lFrameCnt = lVADResultStartLoc - nIn_Speech_Threshold;
                                                        lFrameCnt = 0;
                                                }
                                        }
                                        else
                                        {
                                                lFrameCnt = lVADResultStartLoc;
                                        }
                                }
                                else
                                {
                                        lFrameCnt = lVADResultStartLoc;
                                }
                        }

                        while(lFrameCnt < lVADResultCurLoc)
                        {
                                if(nVADCurState == SILENCE)
                                {
                                        sResultSum = 0;
                                        for(i = 0; i < nIn_Speech_Threshold; i++)
                                        {
                                                sResultSum += g_pVADResult[lFrameCnt + i];
                                        }
                                
                                        if(sResultSum == nIn_Speech_Threshold)
                                        {
                                                nVADCurState = SILENCE_TO_SPEECH;
                                                nStartFrame = int(lFrameCnt - nStartBackFrame * nSampleRate / 16000);
                                                if(lFrameCnt < nStartBackFrame * nSampleRate / 16000)
                                                {
                                                        nStartFrame = 0;
                                                }
                                                lFrameCnt += nIn_Speech_Threshold;

                                                for(i = lVADResultCurLoc; 
                                                        i < DDWORD(nSleep_Timeout * nSampleRate / nFrameLength); i++)
                                                {
                                                        g_pVADResult[i] = 1;
                                                }
                                        }
                                        else
                                        {
                                                lFrameCnt += (nIn_Speech_Threshold - sResultSum);

                                                if(lFrameCnt > nMax_Wait_Duration)
                                                {
                                                        nVADCurState = NO_SPEECH;
                                                }
                                        }
                                }
                                else if(nVADCurState == SILENCE_TO_SPEECH)
                                {
                                        /*if(nAmp >= 2)
                                        {
                                                nPossible_Speech_Pause = 2;
                                        }
                                        else
                                        {
                                                nPossible_Speech_Pause = 4;
                                        }*/

                                        /** @brief Find Possible EndPoint */
                                        sResultSum_PossibleEp = 0;
                                        for(i = 0; i < nPossible_Speech_Pause; i++)
                                        {
                                                sResultSum_PossibleEp += g_pVADResult[lFrameCnt + i];
                                        }
                                        if(sResultSum_PossibleEp == 0 && nFindPossibleEndPoint == 0)
                                        {   
                                                nFindPossibleEndPoint = 1;
                                                nEndFrame = lFrameCnt + nPossible_Speech_Pause;
                                        }
                                        if(nFindPossibleEndPoint == 1)
                                        {
                                                for(DDWORD k = lFrameCnt; k < lVADResultCurLoc; k++)
                                                {
                                                        if(g_pVADResult[k] == 1)
                                                        {
                                                                nFindPossibleEndPoint = 0;
                                                                nEndFrame = 0;
                                                                break;
                                                        }
                                                }
                                        }

                                        /** @brief Find Real EndPoint */
                                        sResultSum = 0;
                                        for(i = 0; i < nMax_Speech_Pause; i++)
                                        {
                                                sResultSum += g_pVADResult[lFrameCnt + i];
                                        }

                                        if(sResultSum == 0)
                                        {
                                                nVADCurState = SPEECH_TO_SILENCE;
                                                lFrameCnt = lVADResultCurLoc;
                                                if(nEndFrame - nStartFrame < nMin_Speech_Duration && nEndFrame != 0)
                                                {
                                                        nVADCurState = SPEECH_TOO_SHORT;
                                                }
                                        }
                                        else
                                        {
                                                if(sResultSum_PossibleEp == 0)
                                                {
                                                        lFrameCnt += 1;
                                                }
                                                else
                                                {
                                                        lFrameCnt += sResultSum_PossibleEp;
                                                }

                                                if((lFrameCnt - nStartFrame > nMax_Speech_Duration)
                                                        && (lFrameCnt > nStartFrame))
                                                {
                                                        nVADCurState = SPEECH_TOO_LONG;
                                                }
                                        }
                                }
                                else if(nVADCurState == SPEECH_TOO_SHORT)
                                {
                                        lFrameCnt = lVADResultCurLoc;
                                }
                                else if(nVADCurState == NO_SPEECH)
                                {
                                        lFrameCnt = lVADResultCurLoc;
                                }
                                else
                                {
                                        lFrameCnt = lVADResultCurLoc;
                                }
                        }

                        if(nFindPossibleEndPoint == 0)
                        {
                                lSampleStart = lSampleEnd;
                                lSampleEnd = lSampleStart +
                                    DDWORD((lSample - lSampleStart) / nSpeechEncLength) * nSpeechEncLength;
                        }
                        else if(nFindPossibleEndPoint == 1)
                        {
                                lSampleStart = lSampleEnd;
                                lSampleEnd = lSampleStart +
                                    DDWORD((nEndFrame * nFrameLength - lSampleStart) / nSpeechEncLength) * nSpeechEncLength;
                                if (lSampleEnd < lSampleStart) { /* He Liqiang, TAG-130807 */
                                    lSampleEnd = lSampleStart;
                                }
                        }
                        else
                        {
                                ;
                        }

                        //printf("StartSample: %ld\t EndSample: %ld\t", lSampleStart, lSampleEnd);
                        //printf("Possible EP: %d\t", nFindPossibleEndPoint);

                        /** @brief Return VAD Detect Result */
RET:
                        if(bNoiseDetectionFlag == false)
                        {
                                if(nVADCurState == SILENCE)
                                {
                                        if(nPossible_Speech_Start == 0)
                                        {
                                                pthread_mutex_unlock(&MyMutex);
                                                usleep(THREAD_SLEEPTIME);
                                                return RET_SILENCE;
                                        }
                                        else
                                        {
                                                pthread_mutex_unlock(&MyMutex);
                                                usleep(THREAD_SLEEPTIME);
                                                return RET_SILENCE_TO_SPEECH;
                                        }
                                }
                                if(nVADCurState == SILENCE_TO_SPEECH)
                                {
                                        pthread_mutex_unlock(&MyMutex);
                                        usleep(THREAD_SLEEPTIME);
                                        return RET_SILENCE_TO_SPEECH;
                                }
                                if(nVADCurState == SPEECH_TO_SILENCE)
                                {
                                        pthread_mutex_unlock(&MyMutex);
                                        usleep(THREAD_SLEEPTIME);
                                        return RET_SPEECH_TO_SILENCE;
                                }
                                if(nVADCurState == NO_SPEECH)
                                {
                                        pthread_mutex_unlock(&MyMutex);
                                        usleep(THREAD_SLEEPTIME);
                                        return RET_NO_SPEECH;
                                }
                                if(nVADCurState == SPEECH_TOO_SHORT)
                                {
                                        pthread_mutex_unlock(&MyMutex);
                                        usleep(THREAD_SLEEPTIME);
                                        return RET_SPEECH_TOO_SHORT;
                                }
                                if(nVADCurState == SPEECH_TOO_LONG)
                                {
                                        pthread_mutex_unlock(&MyMutex);
                                        usleep(THREAD_SLEEPTIME);
                                        return RET_SPEECH_TOO_LONG;
                                }
                        }
                        else
                        {
                                int iReturnValue = 0;
                                if(lClippingNum > 3)
                                {
                                        iReturnValue &= 0x0020;
                                }

                                if(nVADCurState == SILENCE)
                                {
                                        iReturnValue &= 0xfff0;
                                }
                                if(nVADCurState == SILENCE_TO_SPEECH)
                                {
                                        iReturnValue &= 0xfff1;
                                }
                                if(nVADCurState == SPEECH_TO_SILENCE)
                                {
                                        iReturnValue &= 0xfff2;
                                }
                                if(nVADCurState == NO_SPEECH)
                                {
                                        iReturnValue &= 0xfff3;
                                }
                                if(nVADCurState == SPEECH_TOO_SHORT)
                                {
                                        iReturnValue &= 0xfff4;
                                }
                                if(nVADCurState == SPEECH_TOO_LONG)
                                {
                                        iReturnValue &= 0xfff5;
                                }
                                pthread_mutex_unlock(&MyMutex);
                                usleep(THREAD_SLEEPTIME);
                                return iReturnValue;
                        }

                        pthread_mutex_unlock(&MyMutex);
                        usleep(THREAD_SLEEPTIME);
                        return MFE_ERROR_UNKNOWN;
                }
                /** @brief Speech Input Method */
                else
                {
                        DDWORD i;
                        short sVADResult = 0;       /**< @brief Result of each frame */
                        short sResultSum = 0;
                        short sResultSum_PossibleEp = 0;
                        DDWORD lClippingNum = 0;
                        short pFrameData[VAD_FRAMELENGTH];

                        /** @brief Calculate Subband Entropy Feature and Save it */
                        lVADResultStartLoc = lVADResultCurLoc;
                        lVADResultCurLoc = DDWORD(lSample / VAD_FRAMELENGTH);
                        lFrameCnt = lVADResultStartLoc;

                        /** @brief No Data Arrived, Do Nothing */
                        if(lVADResultStartLoc == lVADResultCurLoc)
                        {
                                goto RET2;
                        }

                        while(lFrameCnt < lVADResultCurLoc)
                        {
                                /** @brief Obtain One-Frame Speech Data */
                                DDWORD lTemp = lFrameCnt * VAD_FRAMELENGTH;
                                for(i = 0; i < VAD_FRAMELENGTH; i++)
                                {
                                        pFrameData[i] = g_pData[lTemp + i];

                                        /** @brief Clipping Detection */
                                        if(abs(pFrameData[i]) > 32760)
                                        {
                                                lClippingNum++;
                                        }
                                }
                                /** @brief HighPass filtering and DRC 20150729 */
                                if (nCarlife != 0)
                                {
                                        short pFrameData_DRC[VAD_FRAMELENGTH];
                                        drc_process(drc_handle, pFrameData, pFrameData_DRC);
                                        memcpy(pFrameData, pFrameData_DRC, VAD_FRAMELENGTH * sizeof(short));
                               }
                                /** @brief HighPass filtering and DRC End 20150729 */
                                /** @brief Adjust VADState During VADProcessing*/
                                if(nVADCurState == SILENCE)
                                {
                                        nVADInnerZeroCnt = 0;
                                        if(nVADInnerCnt == nIn_Speech_Threshold)
                                        {
                                                nVADCurState = SILENCE_TO_SPEECH;
                                                nVADInnerCnt = 0;
                                                nSpeechEndCnt = 0;
                                        }
                                }
                                else if(nVADCurState == SILENCE_TO_SPEECH)
                                {
                                        nVADInnerCnt = 0;
                                        if(nVADInnerZeroCnt == nMax_Speech_Pause)
                                        {
                                                nVADCurState = SPEECH_PAUSE;
                                                nVADInnerZeroCnt = 0;
                                        }
                                }
                                else if(nVADCurState == SPEECH_PAUSE)
                                {
                                        nVADInnerZeroCnt = 0;
                                        if(nVADInnerCnt == nIn_Speech_Threshold)
                                        {
                                                nVADCurState = SILENCE_TO_SPEECH;
                                                nVADInnerCnt = 0;
                                                nSpeechEndCnt = 0;
                                        }
                                }
                                else
                                {
                                        nVADInnerCnt = 0;
                                        nVADInnerZeroCnt = 0;
                                        nSpeechEndCnt = 0;
                                }

                                sVADResult = VAD_SubbandEntropy_Process(pFrameData);

                                /** @brief Set Counters */
                                if(nVADCurState == SILENCE)
                                {
                                        nVADInnerCnt = (nVADInnerCnt + sVADResult) * sVADResult;
                                }
                                else if(nVADCurState == SILENCE_TO_SPEECH)
                                {
                                        if(sVADResult == 0)
                                        {   
                                                nVADInnerZeroCnt++;
                                        }
                                        else
                                        {
                                                nVADInnerZeroCnt = 0;
                                        }
                                }
                                else if(nVADCurState == SPEECH_PAUSE)
                                {   
                                        nVADInnerCnt = (nVADInnerCnt + sVADResult) * sVADResult;
                                        if(sVADResult == 0)
                                        {
                                                nSpeechEndCnt++;
                                        }
                                }
                                else
                                {
                                        ;
                                }

                                /** @brief Save VAD Frame-Result */
                                g_pVADResult[lFrameCnt] = sVADResult;
                                //printf("%d:\t %d:\t %d\t", lFrameCnt, nVADCurState, nVADInnerCnt);
                                //printf("%d\n", g_pVADResult[lFrameCnt]);
                                lFrameCnt++;
                                lFrameCntTotal++;
                        }
                        //printf("$$$$$$$$$$$$$$$$\n");

                        /** @brief VAD Detect */
                        if(lSampleStart == 0)
                        {
                                nVADCurState = nVADLastState;
                                lFrameCnt = lVADResultStartLoc;
                        }
                        else
                        {
                                nVADCurState = nVADLastState;
                                if(lVADResultStartLoc > nIn_Speech_Threshold)
                                {
                                        if((nVADCurState == SILENCE) || (nVADCurState == SPEECH_PAUSE))
                                        {
                                                lFrameCnt = lVADResultStartLoc - nIn_Speech_Threshold;
                                        }
                                        else if(nVADCurState == SILENCE_TO_SPEECH)
                                        {
                                                if(lVADResultStartLoc > nMax_Speech_Pause)
                                                {
                                                        lFrameCnt = lVADResultStartLoc - nMax_Speech_Pause;
                                                }
                                                else
                                                {
                                                        lFrameCnt = lVADResultStartLoc - nIn_Speech_Threshold;
                                                }
                                        }
                                        else
                                        {
                                                lFrameCnt = lVADResultStartLoc;
                                        }
                                }
                                else
                                {
                                        lFrameCnt = lVADResultStartLoc;
                                }
                        }
                        while(lFrameCnt < lVADResultCurLoc)
                        {
                                if(nVADCurState == SILENCE)
                                {
                                        nFindPossibleEndPoint = 0;
                                        sResultSum = 0;
                                        for(i = 0; i < nIn_Speech_Threshold; i++)
                                        {
                                                sResultSum += g_pVADResult[lFrameCnt + i];
                                        }
                                
                                        if(sResultSum == nIn_Speech_Threshold)
                                        {
                                                nVADCurState = SILENCE_TO_SPEECH;
                                                nStartFrame = int(lFrameCnt - nStartBackFrame * nSampleRate / 16000);
                                                if(lFrameCnt < nStartBackFrame * nSampleRate / 16000)
                                                {
                                                        nStartFrame = 0;
                                                }
                                                //printf("%d\t", nStartFrame);
                                                lFrameCnt += nIn_Speech_Threshold;

                                                for(i = lVADResultCurLoc;
                                                        i < DDWORD(nSleep_Timeout * nSampleRate / nFrameLength); i++)
                                                {
                                                        g_pVADResult[i] = 1;
                                                }
                                        }
                                        else
                                        {
                                                lFrameCnt += (nIn_Speech_Threshold - sResultSum);

                                                /** @brief No Speech */
                                                if(lFrameCnt > nMax_Wait_Duration)
                                                {
                                                        nVADCurState = NO_SPEECH;
                                                }
                                        }
                                }
                                else if(nVADCurState == SILENCE_TO_SPEECH)
                                {
                                        /** @brief Find Possible Endpoint */
                                        sResultSum_PossibleEp = 0;
                                        for(i = 0; i < nPossible_Speech_Pause; i++)
                                        {
                                                sResultSum_PossibleEp += g_pVADResult[lFrameCnt + i];
                                        }
                                        if(sResultSum_PossibleEp == 0 && nFindPossibleEndPoint == 0)
                                        {
                                                nFindPossibleEndPoint = 1;
                                                nEndFrame = lFrameCnt + nPossible_Speech_Pause;
                                        }
                                        if(nFindPossibleEndPoint == 1)
                                        {
                                                for(DDWORD k = lFrameCnt; k < lVADResultCurLoc; k++)
                                                {
                                                        if(g_pVADResult[k] == 1)
                                                        {
                                                                nFindPossibleEndPoint = 0;
                                                                break;
                                                        }
                                                }
                                        }

                                        /** @brief Find Real Speech Pause Point */
                                        sResultSum = 0;
                                        for(i = 0; i < nMax_Speech_Pause; i++)
                                        {
                                                sResultSum += g_pVADResult[lFrameCnt + i];
                                        }
                                
                                        if(sResultSum == 0)
                                        {
                                                nVADCurState = SPEECH_PAUSE;
                                                //nEndFrame = lFrameCnt + DWORD(nMax_Speech_Pause / 1);
                                                if(lFrameCnt + nMax_Speech_Pause > lVADResultCurLoc)
                                                {
                                                        lFrameCnt = lVADResultCurLoc;
                                                }
                                                else
                                                {
                                                        lFrameCnt += nMax_Speech_Pause;
                                                }
                                                for(i = lVADResultCurLoc;
                                                        i < DDWORD(nSleep_Timeout * nSampleRate / nFrameLength); i++)
                                                {
                                                        g_pVADResult[i] = 0;
                                                }
                                                //printf("%d\t", lFrameCntTotal + nPossible_Speech_Pause);
                                        }
                                        else
                                        {
                                                lFrameCnt += sResultSum;

                                                /** @brief Speech too Long and not finished */
                                                if((lFrameCnt - nStartFrame > nMax_Speech_Duration)
                                                        && (lFrameCnt > nStartFrame))
                                                {
                                                        nVADCurState = SPEECH_TOO_LONG;
                                                }
                                        }
                                }
                                else if(nVADCurState == SPEECH_PAUSE)
                                {
                                        nFindPossibleEndPoint = 0;
                                        sResultSum = 0;
                                        for(i = 0; i < nIn_Speech_Threshold; i++)
                                        {
                                                sResultSum += g_pVADResult[lFrameCnt + i];
                                        }

                                        /** @brief Whether End of the Pack */
                                        if(lFrameCnt + nIn_Speech_Threshold > lVADResultCurLoc + 1)
                                        {     
                                                sResultSum = 0;
                                                for(i = 0; i < nIn_Speech_Threshold; i++)
                                                {
                                                        sResultSum += g_pVADResult[lFrameCnt + i];
                                                        if(lFrameCnt + i > lVADResultCurLoc)
                                                        {
                                                                break;
                                                        }
                                                }
                                        }

                                        if(sResultSum == nIn_Speech_Threshold)
                                        {       
                                                nVADCurState = SILENCE_TO_SPEECH;
                                                nStartFrame = int(lFrameCnt - nStartBackFrame * nSampleRate / 16000);
                                                if(lFrameCnt < nStartBackFrame * nSampleRate / 16000)
                                                {
                                                        nStartFrame = 0;
                                                }
                                                lFrameCnt += nIn_Speech_Threshold;
                                                nSpeechEndCnt = 0;

                                                for(i = lVADResultCurLoc;
                                                        i < DDWORD(nSleep_Timeout * nSampleRate / nFrameLength); i++)
                                                {
                                                        g_pVADResult[i] = 1;
                                                }
                                        }
                                        else
                                        {
                                                lFrameCnt += (nIn_Speech_Threshold - sResultSum);

                                                /** @brief Speech End, FINALLY*/
                                                if(nSpeechEndCnt > nSpeech_End)
                                                {
                                                        nVADCurState = SPEECH_TO_SILENCE;
                                                        nSpeechEndCnt = 0;
                                                }
                                        }
                                }
                                else if(nVADCurState == SPEECH_TOO_SHORT)
                                {
                                        lFrameCnt = lVADResultCurLoc;
                                }
                                else if(nVADCurState == NO_SPEECH)
                                {
                                        lFrameCnt = lVADResultCurLoc;
                                }
                                else
                                {
                                        lFrameCnt = lVADResultCurLoc;
                                }
                        }

                        /** @brief Adjust lSampleStart and lSampleEnd */
                        if(nFindPossibleEndPoint == 0)
                        {
                                lSampleStart = lSampleEnd;
                                lSampleEnd = lSampleStart + 
                                    DDWORD((lSample - lSampleStart) / nSpeechEncLength) * nSpeechEncLength;
                        }
                        else if(nFindPossibleEndPoint == 1)
                        {
                                lSampleStart = lSampleEnd;
                                lSampleEnd = lSampleStart +
                                    DDWORD((nEndFrame * nFrameLength - lSampleStart) / nSpeechEncLength) * nSpeechEncLength;
                        }
                        else
                        {
                                ;
                        }

                        /** @brief Return VAD Detect Result */
RET2:                   
                        if(bNoiseDetectionFlag == false)
                        {
                                if(nVADCurState == SILENCE)
                                {
                                        pthread_mutex_unlock(&MyMutex);
                                        usleep(THREAD_SLEEPTIME);
                                        return RET_SILENCE;
                                }
                                if(nVADCurState == SILENCE_TO_SPEECH)
                                {
                                        pthread_mutex_unlock(&MyMutex);
                                        usleep(THREAD_SLEEPTIME);
                                        return RET_SILENCE_TO_SPEECH;
                                }
                                if(nVADCurState == SPEECH_TO_SILENCE)
                                {
                                        pthread_mutex_unlock(&MyMutex);
                                        usleep(THREAD_SLEEPTIME);
                                        return RET_SPEECH_TO_SILENCE;
                                }
                                if(nVADCurState == NO_SPEECH)
                                {
                                        pthread_mutex_unlock(&MyMutex);
                                        usleep(THREAD_SLEEPTIME);
                                        return RET_NO_SPEECH;
                                }
                                if(nVADCurState == SPEECH_TOO_SHORT)
                                {
                                        pthread_mutex_unlock(&MyMutex);
                                        usleep(THREAD_SLEEPTIME);
                                        return RET_SPEECH_TOO_SHORT;
                                }
                                if(nVADCurState == SPEECH_TOO_LONG)
                                {
                                        pthread_mutex_unlock(&MyMutex);
                                        usleep(THREAD_SLEEPTIME);
                                        return RET_SPEECH_TOO_LONG;
                                }
                                if(nVADCurState == SPEECH_PAUSE)
                                {
                                        pthread_mutex_unlock(&MyMutex);
                                        usleep(THREAD_SLEEPTIME);
                                        return RET_SILENCE;
                                }
                        }
                        else
                        {
                                int iReturnValue = 0;
                                if(lClippingNum > 3)
                                {
                                        iReturnValue &= 0x0020;
                                }

                                if(nVADCurState == SILENCE)
                                {
                                        iReturnValue &= 0xfff0;
                                }
                                if(nVADCurState == SILENCE_TO_SPEECH)
                                {
                                        iReturnValue &= 0xfff1;
                                }
                                if(nVADCurState == SPEECH_TO_SILENCE)
                                {
                                        iReturnValue &= 0xfff2;
                                }
                                if(nVADCurState == NO_SPEECH)
                                {
                                        iReturnValue &= 0xfff3;
                                }
                                if(nVADCurState == SPEECH_TOO_SHORT)
                                {
                                        iReturnValue &= 0xfff4;
                                }
                                if(nVADCurState == SPEECH_TOO_LONG)
                                {
                                        iReturnValue &= 0xfff5;
                                }
                                if(nVADCurState == SPEECH_PAUSE)
                                {
                                        iReturnValue &= 0xfff0;
                                }
                                pthread_mutex_unlock(&MyMutex);
                                usleep(THREAD_SLEEPTIME);
                                return iReturnValue;
                        }

                        pthread_mutex_unlock(&MyMutex);
                        usleep(THREAD_SLEEPTIME);
                        return MFE_ERROR_UNKNOWN;
                }
        }
        else
        {
                return MFE_STATE_ERR;
        }
}


int mfeSetParam(int type, int val)
{
        if(nCurState == DWORD(UNKNOWN))
        {
                if (type < 0 || type > 22)
                {
                        return MFE_PARAMRANGE_ERR;
                }

                if(type > 14)
                {
                        switch(type)
                        {
                            case PARAM_CARLIFE_ENABLE:
                                if(val == 0)
                                {
                                    nCarlife = 0;
                                }
                                else
                                {
                                    nCarlife = 1;
                                }
                                return MFE_SUCCESS;

                            case PARAM_MODE_CMB:
                                if(val != 0)
                                {
                                        nModeComb = 1;
                                }
                                else
                                {
                                        nModeComb = 0;
                                }
                                return MFE_SUCCESS;

                            case PARAM_SPEECHIN_THRESHOLD_BIAS:
                                dThrBias_SpeechIn_Init = double(val);
                                return MFE_SUCCESS;

                            case PARAM_SPEECHOUT_THRESHOLD_BIAS:
                                dThrBias_SpeechOut_Init = double(val);
                                return MFE_SUCCESS;

#ifdef  USING_OPUS
                            case PARAM_BITRATE_OPUS_8K:
                                if(val >= 4000 && val <= 64000)
                                {
                                        bitrate_bps_init = val;
                                }
                                else
                                {
                                        bitrate_bps_init = 16000;
                                }
                                return MFE_SUCCESS;

                            case PARAM_BITRATE_OPUS_16K:
                                if(val >= 8000 && val <= 128000)
                                {
                                        bitrate_bps_init = val;
                                }
                                else
                                {
                                        bitrate_bps_init = 32000;
                                }
                                return MFE_SUCCESS;
#endif
#ifdef  USING_AMR_NB
                            case PARAM_BITRATE_NB:
                                switch(val)
                                {
                                    case 8:
                                        mode = MR122;
                                        break;
                                    case 7:
                                        mode = MR102;
                                        break;
                                    case 6:
                                        mode = MR795;
                                        break;
                                    case 5:
                                        mode = MR74;
                                        break;
                                    case 4:
                                        mode = MR67;
                                        break;
                                    case 3:
                                        mode = MR59;
                                        break;
                                    case 2:
                                        mode = MR515;
                                        break;
                                    case 1:
                                        mode = MR475;
                                        break;
                                    default:
                                        mode = MR122;
                                        break;
                                }
                                return MFE_SUCCESS;
#endif
#ifdef  USING_AMR
                            case PARAM_BITRATE_WB:
                                if(val > 0 && val <= 8)
                                {
                                        coding_mode = val;
                                }
                                else
                                {
                                        coding_mode = 4;
                                }
                                return MFE_SUCCESS;
#endif

                            default:
                                return MFE_PARAMRANGE_ERR;
                        }
                        return MFE_SUCCESS;
                }

                DWORD nInputValue = DWORD(val);
                switch(type)
                {
                    case PARAM_MAX_WAIT_DURATION:
                        nMax_Wait_Duration_Init = nInputValue;
                        return MFE_SUCCESS;

                    case PARAM_MAX_SP_DURATION:
                        nMax_Speech_Duration_Init = nInputValue;
                        return MFE_SUCCESS;

                    case PARAM_MAX_SP_PAUSE:
                        nMax_Speech_Pause_Init = nInputValue;
                        return MFE_SUCCESS;

                    case PARAM_MIN_SP_DURATION:
                        nMin_Speech_Duration_Init = nInputValue;
                        return MFE_SUCCESS;

                    case PARAM_OFFSET:
                        nOffset_Init = nInputValue;
                        return MFE_SUCCESS;

                    case PARAM_SPEECH_END:
                        nSpeech_End_Init = nInputValue;
                        return MFE_SUCCESS;

                    case PARAM_SPEECH_MODE:
                        nSpeech_Mode_Init = nInputValue;
                        return MFE_SUCCESS;
                        
                    case PARAM_NEED_VAD:
                    	if (nInputValue == 0)
                    		nVAD = false;
                    	else 
                    		nVAD = true;
                    	return MFE_SUCCESS;
                    	
                    case PARAM_NEED_COMPRESS:
                    	if (nInputValue == 0)
                    		nCompress = false;
                    	else 
                    		nCompress = true;
                    	return MFE_SUCCESS;
                    
                    case PARAM_SAMPLE_RATE:
                    	nSampleRate = (DWORD)nInputValue;
                    	return MFE_SUCCESS;
                    
                    case PARAM_CODE_FORMAT:
                    	nCodeFormat = (DWORD)nInputValue;
                    	return MFE_SUCCESS;
                    
                    default:
                        return MFE_PARAMRANGE_ERR;
                }
        }
        else
        {
                return MFE_STATE_ERR;
        }
}


int mfeGetParam(int type)
{
        if(nCurState == DWORD(UNKNOWN))
        {
                switch(type)
                {
                    case PARAM_MAX_WAIT_DURATION:
                        return int(nMax_Wait_Duration);

                    case PARAM_MAX_SP_DURATION:
                        return int(nMax_Speech_Duration);

                    case PARAM_MAX_SP_PAUSE:
                        return int(nMax_Speech_Pause);

                    case PARAM_MIN_SP_DURATION:
                        return int(nMin_Speech_Duration);

                    case PARAM_OFFSET:
                        return int(nOffset);

                    case PARAM_SPEECH_END:
                        return int(nSpeech_End);

                    case PARAM_SPEECH_MODE:
                        return int(nSpeech_Mode);

#ifdef  USING_OPUS
                    case PARAM_BITRATE_OPUS_8K:
                        return int(bitrate_bps_init);

                    case PARAM_BITRATE_OPUS_16K:
                        return int(bitrate_bps_init);
#endif
#ifdef  USING_AMR_NB
                    case PARAM_BITRATE_NB:
                        switch(mode)
                        {
                            case MR122:
                                return 8;
                            case MR102:
                                return 7;
                            case MR795:
                                return 6;
                            case MR74:
                                return 5;
                            case MR67:
                                return 4;
                            case MR59:
                                return 3;
                            case MR515:
                                return 2;
                            case MR475:
                                return 1;
                            default:
                                return 8;
                        }
#endif
#ifdef  USING_AMR
                    case PARAM_BITRATE_WB:
                        return coding_mode;
#endif

                    case PARAM_MODE_CMB:
                        return nModeComb;

                    default:
                        return MFE_PARAMRANGE_ERR;
                }
        }
        else
        {
                return MFE_STATE_ERR;
        }
}


int mfeEnableNoiseDetection(bool val)
{
        if(nCurState == DWORD(UNKNOWN))
        {
                bNoiseDetectionFlag = val;
                return MFE_SUCCESS;
        }
        else
        {
                return MFE_STATE_ERR;
        }
}


void mfeSetLogLevel(int iLevel)
{
        if(iLevel != 0)
        {
                iLogLevel = 7;
        }
        else
        {
                iLogLevel = 0;
        }
}


int mfeSetVADParam(int type, int val)
{
        if(nCurState == DWORD(UNKNOWN))
        {
                if(type < 0 || type > 5)
                {
                        return MFE_PARAMRANGE_ERR;
                }
                else
                {
                        switch(type)
                        {
                            case PARAM_VADTHR_SILENCE_SP:
                                dThrBias_SpeechIn_BI_Silence_Init = double(val);
                                break;

                            case PARAM_VADTHR_SLIGHTNOISE_SP:
                                dThrBias_SpeechIn_BI_Slightnoise_Init = double(val);
                                break;

                            case PARAM_VADTHR_NOISE_SP:
                                dThrBias_SpeechIn_BI_Noise_Init = double(val);
                                break;

                            case PARAM_VADTHR_SILENCE_EP:
                                dThrBias_SpeechOut_BI_Silence_Init = double(val);
                                break;

                            case PARAM_VADTHR_SLIGHTNOISE_EP:
                                dThrBias_SpeechOut_BI_Slightnoise_Init = double(val);
                                break;

                            case PARAM_VADTHR_NOISE_EP:
                                dThrBias_SpeechOut_BI_Noise_Init = double(val);
                                break;

                            default:
                                return MFE_PARAMRANGE_ERR;
                        }
                        return MFE_SUCCESS;
                }
        }
        else
        {
                return MFE_STATE_ERR;
        }
}


long mfeGetHistVolume()                     // Return History mean volume to JAVA
{
        return 0;
}


int mfeSetHistVolume(long lHistVolume)
{
        if(nCurState == DWORD(UNKNOWN))
        {
                return MFE_SUCCESS;
        }
        else
        {
                return MFE_STATE_ERR;
        }
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
