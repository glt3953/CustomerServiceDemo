#ifndef _IIR_HPF_H_
#define _IIR_HPF_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void* IIR_HANDLE; 

IIR_HANDLE iir_init();
void iir_proc(IIR_HANDLE iir_handle, float *Input, int NumSigPts);
void iir_reset(IIR_HANDLE iir_handle);
void iir_free(IIR_HANDLE iir_handle);

#ifdef __cplusplus
}
#endif


#endif

