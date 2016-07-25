 /***************************************************************************
  * 
  * Copyright (c) 2011 Baidu.com, Inc. All Rights Reserved
  * 
  **************************************************************************/
 
 /**
  * @file BaseLib.h
  * @author songhui(com@baidu.com)
  * @date 2011/08/30 18:56:56
  * @brief A lot of useful basic functions 
  *  
  **/


#ifndef  __BASELIB_H_
#define  __BASELIB_H_


#include	"PreDefinition.h"


/** @brief Signal Trasnformation Functions */

/** @brief FFT */
void FFT(COMPLEX *pFFTData, DWORD nFFTOrder);


/** @brief Sort Processing Functions */

/** @brief Quick Sort Algorithm */
bool Sort_QuickSort(double *pData, DDWORD lDataLength);
/** @brief One-time Quick Sort, Component of Sort_QuickSort */
DDWORD OneQuickSort(double *pData, DDWORD lLow, DDWORD lHigh);
/** @brief Quick Sort Algorithm */
//bool Sort_QuickSort(DDWORD *pData, DDWORD lDataLength);
/** @brief One-time Quick Sort, Component of Sort_QuickSort */
//DDWORD OneQuickSort(DDWORD *pData, DDWORD lLow, DDWORD lHigh);



#endif  //__BASELIB_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
