#ifndef _FFT_H
#define _FFT_H

#include "inc.h"

#ifdef _EXTERN_CALL_AUDIOBITMAP
#ifdef DLLEXPORT
#undef DLLEXPORT
#endif
#endif

#ifdef DLLEXPORT
class DLLEXPORT CFft {
#else
class CFft {
#endif
private:
	void FFT(float *s, int, int);
public:
	void Realft (float *s, int);
};

#endif

