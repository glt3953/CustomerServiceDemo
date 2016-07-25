 /***************************************************************************
  * 
  * Copyright (c) 2011 Baidu.com, Inc. All Rights Reserved
  * 
  **************************************************************************/
 
 /**
  * @file BaseLib.cpp
  * @author songhui(com@baidu.com)
  * @date 2011/08/30 18:59:36
  * @brief A lot of Basic Functions 
  *  
  **/



#include    <math.h>
#include	"PreDefinition.h"
#include	"BaseLib.h"
#include    <stddef.h>


/** @brief Signal Trasnformation Functions */
/** @brief FFT */
void FFT(COMPLEX *pFFTData, DWORD nFFTOrder)
{
        int n, i, nv2, j, k, le, l, le1, ip, nm1;
        COMPLEX t, u, w;

	    n = 1;
	    for(i = 0; i < (int)nFFTOrder; i++)
        {
                n = n * 2;
        }

	    nv2 = n / 2;
	    nm1 = n - 1;
	    j = 1;

	    for (i = 1; i <= nm1; i ++)
	    {
		        if (i < j)
		        {
			            t.real = pFFTData[i - 1].real;
			            t.image = pFFTData[i - 1].image;
			            pFFTData[i - 1].real = pFFTData[j - 1].real;
			            pFFTData[i - 1].image = pFFTData[j - 1].image;
			            pFFTData[j - 1].real = t.real;
			            pFFTData[j - 1].image = t.image;
		        }

		        k = nv2;

		        while (k < j)
		        {
			            j -= k;
			            k /= 2;
		        }
		        j += k;
	    }

	    le = 1;

	    for (l= 1; l <= (int)nFFTOrder; l++)
	    {
		        le *= 2;
		        le1 = le / 2;
		        u.real = 1.0f;
		        u.image = 0.0f;
		        w.real = (float) cos(PI / le1);
		        w.image =(float) -sin(PI / le1);

		        for (j = 1; j <= le1; j++)
		        {
			            for (i = j; i <= n; i += le)
			            {
				                ip = i + le1;
				                t.real = pFFTData[ip - 1].real * u.real - pFFTData[ip - 1].image * u.image;
				                t.image = pFFTData[ip - 1].real * u.image + pFFTData[ip - 1].image * u.real;
				                pFFTData[ip - 1].real = pFFTData[i - 1].real - t.real;
				                pFFTData[ip - 1].image = pFFTData[i - 1].image - t.image;
				                pFFTData[i - 1].real = t.real + pFFTData[i - 1].real;
				                pFFTData[i - 1].image = t.image + pFFTData[i - 1].image;
			            }

			            t.real = u.real * w.real - u.image * w.image;
			            t.image = u.image * w.real + u.real * w.image;
			            u.real = t.real;
			            u.image = t.image;
		        }
        }
}



/** @brief Sort Processing Functions */
/** @brief Quick Sort Algorithm */
bool Sort_QuickSort(double *pData, DDWORD lDataLength)
{
        if(lDataLength > 1)
        {
                DDWORD lPivotLoc = 0;
                lPivotLoc = OneQuickSort(pData, 0L, lDataLength - 1);
                Sort_QuickSort(pData, lPivotLoc);
                Sort_QuickSort(&pData[lPivotLoc + 1], lDataLength - lPivotLoc - 1);
        }

        return true;
}

/** @brief One-time Quick Sort, Component of Sort_QuickSort */
DDWORD OneQuickSort(double *pData, DDWORD lLow, DDWORD lHigh)
{
        DDWORD lLowIndex = lLow;
        DDWORD lHighIndex = lHigh;
        double dPivotKey = pData[lLowIndex];        /**< @brief Pivot Key */

        while(lLowIndex < lHighIndex)
        {
                while((lLowIndex < lHighIndex) && (pData[lHighIndex] >= dPivotKey))
                {
                        lHighIndex--;
                }
                pData[lLowIndex] = pData[lHighIndex];

                while((lLowIndex < lHighIndex) && (pData[lLowIndex] <= dPivotKey))
                {
                        lLowIndex++;
                }
                pData[lHighIndex] = pData[lLowIndex];
        }
        pData[lLowIndex] = dPivotKey;

        return lLowIndex;
}

/** @brief Quick Sort Algorithm */
/*bool Sort_QuickSort(DDWORD *pData, DDWORD lDataLength)
{
        if(lDataLength > 1)
        {
                DDWORD lPivotLoc = 0;
                lPivotLoc = OneQuickSort(pData, 0L, lDataLength - 1);
                Sort_QuickSort(pData, lPivotLoc);
                Sort_QuickSort(&pData[lPivotLoc + 1], lDataLength - lPivotLoc - 1);
        }

        return true;
}*/

/** @brief One-time Quick Sort, Component of Sort_QuickSort */
/*DDWORD OneQuickSort(DDWORD *pData, DDWORD lLow, DDWORD lHigh)
{
        DDWORD lLowIndex = lLow;
        DDWORD lHighIndex = lHigh;
        DDWORD lPivotKey = pData[lLowIndex];

        while(lLowIndex < lHighIndex)
        {
                while((lLowIndex < lHighIndex) && (pData[lHighIndex] >= lPivotKey))
                {
                        lHighIndex--;
                }
                pData[lLowIndex] = pData[lHighIndex];

                while((lLowIndex < lHighIndex) && (pData[lLowIndex] <= lPivotKey))
                {
                        lLowIndex++;
                }
                pData[lHighIndex] = pData[lLowIndex];
        }
        pData[lLowIndex] = lPivotKey;

        return lLowIndex;
}*/


/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
