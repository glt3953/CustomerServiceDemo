/*================================================================*/
/* This is the main source file for Dynamic Range Control          */
/* Module Name: drc.cpp                                              */
/* Modified date: Auguest 1, 2014                                 */
/* Written by: Weiwei Cui                                         */
/* (CopyRight) Baidu Co., Ltd.                                    */
/*================================================================*/

#include "drc.h"
#include "IIRFilter.h"
#include "NoiseLevel.h"

#define SCALE_FACTOR 32768            // PCM 16bit
#define DRC_POST_BOOST_FAC 1        // post boost factor
#define      NL_RUN_MIN_LEN    50 //250, 62, 333 for 24deci

//#define DEBUG_NL_EST

#ifdef DEBUG_NL_EST
extern int FrmCnt;
#endif

typedef struct
{
    int      NOISE_FRM_CNT;    // number of frames for noise estimation 
    float THR_FACTOR;        // factor for getting noise floor  
    float alpha_power;        // smoothing factor for power estimation 
    float alpha_gain;        // smoothing factor for gain estimation 
    float slope2;            // slope of expandation 
    float slope1;            // slope of compression
    float thr1;                // threshold1: the maximal expandation SPL (default -16dB)
    float thr2;                // threshold2: thr1>thr2>thr3
    float thr3;      // threshold3: dynamically estimated by beginning NOISE_FRM_CNT frames
    float y_thr2;            // values corresponding to threshold2
    float gain;                // final gain
    float in_power;            // input power 
    // noise tracking
    S_NoiseLevel_RP *noise_est;
    int   noise_level_cnt;
    int   FLAG_DRC_ON; 
    IIR_HANDLE  iir;
} DRC_PARAM; 

void clipping_detect(float *inbuf, float *outbuf, float gain);

DRC_HANDLE drc_create()
{
    DRC_HANDLE drc_handle; 
    drc_handle = (DRC_HANDLE) malloc(sizeof(DRC_PARAM)); 
    return drc_handle; 
}

void drc_destroy(DRC_HANDLE drc_handle)
{
    DRC_PARAM *drc_param; 
    drc_param = (DRC_PARAM *) drc_handle; 
    free(drc_param->noise_est); 
    iir_free(drc_param->iir);
    free(drc_handle);
}
    
void drc_init(DRC_HANDLE drc_handle)
{
    DRC_PARAM *drc_param; 
    drc_param = (DRC_PARAM *) drc_handle; 

    drc_param->NOISE_FRM_CNT = 5; 
    drc_param->THR_FACTOR = 1.2;  
    drc_param->alpha_power = 0.5;
    drc_param->alpha_gain = 0.8; 
    drc_param->thr1 = -6; // -16  
    drc_param->slope2 = 2;  
    drc_param->thr3 = -90; // init value
    drc_param->in_power = 0;
    drc_param->gain = 1; 
    drc_param->noise_est = (S_NoiseLevel_RP *) malloc(sizeof(S_NoiseLevel_RP));
    NoiseLevel_Init_RP(drc_param->noise_est, 0.8545f, 2.3842e-007f, NL_RUN_MIN_LEN);
    drc_param->noise_level_cnt = 0; 
    drc_param->FLAG_DRC_ON = 0; 
    drc_param->iir = iir_init();
}
void drc_reset(DRC_HANDLE drc_handle)
{
    DRC_PARAM *drc_param; 
    drc_param = (DRC_PARAM *) drc_handle; 

    drc_param->in_power = 0;
    drc_param->gain = 1; 
    drc_param->noise_level_cnt = 0; 
    drc_param->FLAG_DRC_ON = 0; 
    NoiseLevel_Init_RP(drc_param->noise_est, 0.8545f, 2.3842e-007f, NL_RUN_MIN_LEN);
}
        
void drc_process(DRC_HANDLE drc_handle, short *input, short *output)
{
    DRC_PARAM *drc_param; 
    int i; 
    float in_power_dB, out_power_dB; 
    float *inbuf, *outbuf; 
    float enrg; 
    float gain_t0; 

#ifdef DEBUG_NL_EST
    FILE *fp_tmp2; 
    char fname[200];
#endif
    drc_param = (DRC_PARAM *) drc_handle; 

    inbuf = (float *)calloc(NFRM, sizeof(float)); 
    outbuf = (float *)calloc(NFRM, sizeof(float)); 

    for (i = 0; i < NFRM; i++)
    {
        inbuf[i] = (float)input[i]/SCALE_FACTOR; 
    }

    // ---- HPF ---- //
    iir_proc(drc_param->iir, inbuf, NFRM);
    // in_power = alpha_power * in_power + (1-alpha_power) * (input.'* input / NFRM); 
    enrg = 0.0f; 
    for (i = 0; i < NFRM; i++)
    {
        enrg += inbuf[i] * inbuf[i]; 
    }
    // minimum tracking noise level
    NoiseLevel_RP(enrg, drc_param->noise_est);

    if (enrg < 1.5 * drc_param->noise_est->NoiseLevel) {
        drc_param->noise_level_cnt++; 
    } else {
        drc_param->noise_level_cnt = 0; 
    }

    drc_param->in_power = drc_param->alpha_power * drc_param->in_power
      + (1 - drc_param->alpha_power) * enrg / NFRM;
    if (enrg <= 2.3283e-009)
    {            
        for (i = 0; i < NFRM; i++)
            output[i] = input[i];

        drc_reset(drc_handle); 

        free(inbuf); 
        free(outbuf); 
        return;
    }
    if (drc_param->noise_level_cnt > drc_param->NOISE_FRM_CNT)
    {
        // DRC curve initialization
        drc_param->thr3 =  10 * log10(drc_param->THR_FACTOR * drc_param->in_power);
        drc_param->thr2 = (drc_param->thr1 - drc_param->thr3) / 3 + drc_param->thr3;
        drc_param->y_thr2 = drc_param->slope2 * (drc_param->thr2 - drc_param->thr3) + drc_param->thr3; 
        drc_param->slope1 = (drc_param->thr1-drc_param->y_thr2) / (drc_param->thr1 - drc_param->thr2);
        drc_param->gain = 1;
        drc_param->noise_level_cnt = 0; 
        drc_param->FLAG_DRC_ON = 1; 
    }
    if (drc_param->FLAG_DRC_ON == 1)
    {
        in_power_dB = 10 * log10(drc_param->in_power);
        if (in_power_dB > drc_param->thr3 && in_power_dB < drc_param->thr2) 
        {
            out_power_dB = drc_param->slope2 * (in_power_dB - drc_param->thr3) + drc_param->thr3;  
        }
        else if (in_power_dB> drc_param->thr2 && in_power_dB < drc_param->thr1)
        {
            out_power_dB = drc_param->slope1 * (in_power_dB - drc_param->thr2) + drc_param->y_thr2;  
        }
        else
        {
            out_power_dB = in_power_dB; 
        }
        gain_t0 = pow(10.0f, ((out_power_dB - in_power_dB) / 20.0f) );
        drc_param->gain = drc_param->alpha_gain * drc_param->gain + (1 - drc_param->alpha_gain) * gain_t0; 
    }
    clipping_detect(inbuf, outbuf, drc_param->gain); 
    for (i = 0; i < NFRM; i++)
    {
        output[i] = (short)(outbuf[i] * DRC_POST_BOOST_FAC * SCALE_FACTOR);
    }

#ifdef DEBUG_NL_EST
        sprintf(fname, "..\\pcm\\noise_est_2.txt");
        if (FrmCnt == 1)
        {
            if (remove(fname) == 0) {
                printf("Removed %s.\n",fname);     
            } else {
                perror("remove");
            }
        }
        fp_tmp2 = fopen(fname,"a");
        fprintf(fp_tmp2, "%.12f, %.12f, %.2f\n", enrg, drc_param->noise_est->NoiseLevel, drc_param->thr3);
        fclose(fp_tmp2); 
#endif

    free(inbuf); 
    free(outbuf); 
} 

// clipping detection 
void clipping_detect(float *inbuf, float *outbuf, float gain)
{
    int i; 
    float maxValue = 0.0f; 
    float ftemp; 
    
    // maxv = max(abs(input*gain));
    for (i = 0; i < NFRM; i++)
    {
        ftemp = fabs(inbuf[i] * gain);
        if (ftemp > maxValue) 
            maxValue = ftemp; 
    }

    if (maxValue > 0.9)
    {
        gain = 0.97 * gain; 
        clipping_detect(inbuf, outbuf, gain); 
    }
    else
    {
        // outbuf = inbuf * gain; 
        for (i = 0; i < NFRM; i++)
        {
            outbuf[i] = inbuf[i] * gain;
        }
    }
}

