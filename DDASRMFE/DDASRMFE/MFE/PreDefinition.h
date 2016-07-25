 /***************************************************************************
  * 
  * Copyright (c) 2011 Baidu.com, Inc. All Rights Reserved
  * 
  **************************************************************************/  
 
 /**
  * @file PreDefinition.h
  * @author songhui(com@baidu.com)
  * @date 2011/08/23 13:28:58
  * @brief 
  */  


#ifndef  __PREDEFINITION_H_
#define  __PREDEFINITION_H_


typedef unsigned int DWORD;
typedef unsigned short WORD;
typedef unsigned long DDWORD;


typedef struct WavHead                  /**< @brief Old Wav Head, 44 Bytes */
{
        char Riff[4];
        DWORD FileLength;
        char Wave[4];
        char fmt[4];
        DWORD TempBytes;
        WORD PCM;
        WORD nChannel;
        DWORD SampleRate;
        DWORD TransRate;
        WORD Adjust;
        WORD nBitPerSample;
        char cData[4];
        DWORD nBytes;
} WAVE_HEAD;

typedef struct
{
	float	real;
	float	image;
} COMPLEX;							/**< @brief Complex Data Structure */


/** @brief Some Const Definition */
#define     PI                      3.14159265358979f

//#define     MAX_SPEECH_DURATION     20     /**< @brief Maximum Speech Length: 8s */
//#define     MIN_SPEECH_DURATION     0.5    /**< @brief Minimum Speech Length: 0.5s */

/** @brief VAD Parameters */
//#define     IN_SPEECH_THRESHOLD     8
//#define     OUT_SPEECH_THRESHOLD    7



#endif  //__PREDEFINITION_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
