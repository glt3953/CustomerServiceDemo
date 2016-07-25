/*
********************************************************************************
**
**  Module Name:
**      NoiseLevel.h
**
**  Abstract:
**      Full band noise level estimator
**      Finds the noise level using a running minimum of full band noise in
**      the time domain.
**      Not meant to be used to find the noise level of frequency bands. 
**
**      The structure S_NoiseLevel stores the state of the noise level 
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

#ifndef __NOISELEVEL_H
#define __NOISELEVEL_H

typedef struct 
{
   // first noise estimator variables

    float    MaxNoise;       // Max Noise Energy
    float    MinNoise;       // Min Noise Energy
    int      RunMinLen;      // Running minimun window length

    float    MinEnergy;      // Running minimun along a shifting window      
    float    MinTemp;        // Temporary minimum                            
    int      MinAge;         // min age counter used to schedule min update  
    float    SmoothRate;     // final noise smooth rate
    float    NoiseLevel;     // Noise Level (return value)  

    // second noise estimator variables
    
    float    MaxNoise1;       // Max Noise Energy
    float    MinNoise1;       // Min Noise Energy
    int      RunMinLen1;      // Running minimun window length

    float    MinEnergy1;      // Running minimun along a shifting window      
    float    MinTemp1;        // Temporary minimum                            
    int      MinAge1;         // min age counter used to schedule min update  
    float    SmoothRate1;     // final noise smooth rate
    float    NoiseLevel1;     // Noise Level (return value)  
    
    int      SignalOn;        // Counter to declare when signal has been detected
            
} S_NoiseLevel_RP;

void NoiseLevel_Init_RP(S_NoiseLevel_RP* This,
						float            MaxNoise,
                        float            MinNoise,
                        int              RunMinLen
                        );

void NoiseLevel_RP(float Energy,S_NoiseLevel_RP* This);

#endif // __NOISELEVEL_H

