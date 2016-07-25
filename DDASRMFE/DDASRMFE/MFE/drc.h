/*================================================================*/
/* This is the main source file for Dynamic Range Control		  */
/* Module Name: drc.h				                              */
/* Modified date: Auguest 1, 2014                                 */
/* Written by: Weiwei Cui                                         */
/* (CopyRight) Baidu Corporation							      */
/*================================================================*/
#ifndef _DRC_H_
#define _DRC_H_

#include <stdlib.h>
//#include <malloc.h>
#include <math.h>
#include <stdio.h>

#define NFRM 256				// frame length

typedef void* DRC_HANDLE; 

DRC_HANDLE drc_create(); 
void drc_init(DRC_HANDLE drc_handle);
void drc_process(DRC_HANDLE drc_handle, short *input, short *output);
void drc_reset(DRC_HANDLE drc_handle); 
void drc_destroy(DRC_HANDLE drc_handle); 

#endif 

