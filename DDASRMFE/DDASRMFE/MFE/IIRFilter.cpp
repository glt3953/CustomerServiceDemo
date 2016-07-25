#include <stdlib.h>
#include "IIRFilter.h"
//#include <malloc.h>
//#include <stdio.h>

typedef struct
{
    // register
    double RegX1,RegX2;
    double RegY1,RegY2;
    // filter coefficients
    double b0, b1, b2; // numerator 
    double a0, a1, a2; // denominator
}FILTCOEF; 

/*
short BI_saturate(double x)
{
    short    res;

    if(x>32767.0){
        res = 32767;
    }else if(x<-32768.0f){
        res = -32768;
    }else    res = (short)x;
    return (res);
}
*/

// Form 1 Biquad Section Calc, called by RunIIRBiquadForm1.                                
double SectCalcForm1(FILTCOEF *filtcoef, double x)                                                      
{                                                                                          
    double y, CenterTap;                                                                      
                                                                                           
    CenterTap = x * filtcoef->b0 + filtcoef->b1 * filtcoef->RegX1 + filtcoef->b2 * filtcoef->RegX2;                              
    y = filtcoef->a0 * CenterTap - filtcoef->a1 * filtcoef->RegY1 - filtcoef->a2 * filtcoef->RegY2;                              
                                                                                           
    filtcoef->RegX2 = filtcoef->RegX1;                                                                      
    filtcoef->RegX1 = x;                                                                             
    filtcoef->RegY2 = filtcoef->RegY1;                                                                      
    filtcoef->RegY1 = y;                                                                             
                                                                                           
    return(y);                                                                                
}                                                                                          

IIR_HANDLE iir_init()
{
    IIR_HANDLE iir_handle; 
    FILTCOEF   *filtcoef; 
    iir_handle = (IIR_HANDLE) malloc(sizeof(FILTCOEF)); 
    filtcoef = (FILTCOEF *) iir_handle; 

    filtcoef->RegX1 = 0.0;                                                                         
    filtcoef->RegX2 = 0.0;                                                                         
    filtcoef->RegY1 = 0.0;                                                                         
    filtcoef->RegY2 = 0.0;                                                                         

    filtcoef->b0 = 0.9201f;
    filtcoef->b1 = -1.8401f;
    filtcoef->b2 = 0.9201f;
    filtcoef->a0 = 1.0f;
    filtcoef->a1 = -1.8337f;
    filtcoef->a2 = 0.8465f;    

    return(iir_handle);
}

void iir_reset(IIR_HANDLE iir_handle)
{
    FILTCOEF   *filtcoef; 
    filtcoef = (FILTCOEF *) iir_handle; 

    filtcoef->RegX1 = 0.0;                                                                         
    filtcoef->RegX2 = 0.0;                                                                         
    filtcoef->RegY1 = 0.0;                                                                         
    filtcoef->RegY2 = 0.0;                                                                         
}

void iir_free(IIR_HANDLE iir_handle)
{
    FILTCOEF   *filtcoef; 
    filtcoef = (FILTCOEF *) iir_handle; 
    free(filtcoef); 
}

// Form 1 Biquad                                                                           
// This uses 2 sets of shift registers, RegX on the input side and RegY on the output side.
//void RunIIRBiquadForm1(double *Input, double *Output, int NumSigPts)                       
void iir_proc(IIR_HANDLE iir_handle, float *Input, int NumSigPts)                       
{                                                                                          
    FILTCOEF   *filtcoef; 
    double y;                                                                                 
    int j, k;     
                                                                                           
    filtcoef = (FILTCOEF *) iir_handle; 
    for(j = 0; j < NumSigPts; j++)
    {
        Input[j] = SectCalcForm1(filtcoef, Input[j]);    
    }
}                                                                                          
