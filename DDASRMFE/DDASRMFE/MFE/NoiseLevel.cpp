/*
********************************************************************************
**
**  Module Name:
**      NoiseLevel.c
**
**  Abstract:
**      Full band noise level estimator
**
**  Authors:
**      Jianqiang Wei
**
**  Environment:
**      Complier requirements
**          Visual C++ 2010
**
**      Operating System requirements
**          Windows 7
**
********************************************************************************
*/

/*******************************************************************************
    DESCRIPTION
    To estimate the background noise level the following step are applied:
    1)  clipping the input energy into [MaxNoise, MinNoise]
    2)  running minimum (an approximation of the minimum over a shifting
        window of RunMinLen values)
    3)  a linear smoothing as the output of the running min is staircase shaped
********************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "NoiseLevel.h"


/*==========================================================================*/

//void NoiseLevel_Init_RP(S_NoiseLevel_RP* This,
//                          float MaxNoise,
//                        float MinNoise,
//                        int   RunMinLen,
//                        float SmoothRate)
void NoiseLevel_Init_RP(S_NoiseLevel_RP* This, float MaxNoise, float MinNoise, int   RunMinLen)
{
    //float MaxNoise = 6e007; 
    //float MinNoise = 16;
    //int   RunMinLen = 333; 
    float SmoothRate = 0.2; // innovation factor

    This->MaxNoise    = MaxNoise;
    This->MinNoise    = MinNoise;
    This->RunMinLen   = RunMinLen * 2; // 666;

    This->MinEnergy   = MaxNoise;
    This->MinTemp     = MaxNoise;
    This->MinAge      = 0;

    This->SmoothRate  = SmoothRate;
    This->NoiseLevel  = MaxNoise;

    This->MaxNoise1   = MaxNoise;
    This->MinNoise1   = MinNoise;
    This->RunMinLen1  = RunMinLen;

    This->MinEnergy1  = MaxNoise;
    This->MinTemp1    = MaxNoise;
    This->MinAge1     = 0;

    This->SmoothRate1 = SmoothRate;
    This->NoiseLevel1 = MaxNoise;

    This->SignalOn    = 0;
}

/*==========================================================================*/

void NoiseLevel_RP(float Energy, S_NoiseLevel_RP* This)
{
    if (Energy < This->MinEnergy1)
    {
        This->MinEnergy1 = Energy;
        This->MinAge1 = 0;
        This->MinTemp1 = This->MaxNoise1;
    }
    else
        This->MinAge1++;

    if (This->MinAge1 > (This->RunMinLen1>>1) && Energy < This->MinTemp1)
        This->MinTemp1 = Energy;

    if (This->MinAge1 > ((3*This->RunMinLen1)>>1))
    {
        This->MinEnergy1 = This->MinTemp1;
        This->MinTemp1 = This->MaxNoise1;
        This->MinAge1 = (This->RunMinLen1>>1);
    }

    This->NoiseLevel1 += This->SmoothRate1 * (This->MinEnergy1 - This->NoiseLevel1);

    if (Energy < 10.F * This->NoiseLevel1) // If low enough energy,update second noise estimator
    {
        if (Energy < This->MinNoise1) {
            Energy = This->MinNoise;
        }
        if (Energy < This->MinEnergy)
        {
            This->MinEnergy = Energy;
            This->MinAge = 0;
            This->MinTemp = This->MaxNoise;
        } else {
            This->MinAge++;
        }

        if (This->MinAge > (This->RunMinLen>>1) && Energy < This->MinTemp) {
            This->MinTemp = Energy;
        }
        if (This->MinAge > ((3*This->RunMinLen)>>1)) 
        {
            This->MinEnergy = This->MinTemp;
            This->MinTemp = This->MaxNoise;
            This->MinAge = (This->RunMinLen>>1);
        }
        This->NoiseLevel += This->SmoothRate * (This->MinEnergy - This->NoiseLevel);
    }
}

