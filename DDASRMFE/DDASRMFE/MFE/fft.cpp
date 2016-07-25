/*
This program is for real-FFT
*/

#include	"inc.h"
#include	"fft.h"

/* EXPORT-> Realft: apply fft to real s */
//Realft: 
//Input: 1) float*s : sample point values, 2) fftNum : fft size equal to number of sample point values
//return: fft real&image parts in float*s, i.e. replace sample points. Energy is not calculated,
//format: in *s, assume 256 FFT, so memory size of s 257. So have 129 filter bank k=[1,129]. Note: index from 1, not zero
//FFT bank k at position of s: real 2*k-1, image 2*k
//k=1: energy
//k=2: FFT bank 1
//k=128: FFT bank 127
//k=129: fft bank 128, which corresponding frequency sample_rate/2, in current version, it is not caculated. i.e. 
//RealFt, only return FFT bank 0-127, total 128 bank.
void CFft::Realft(float *s, int fftNum)
{
	int n, n2, i, i1, i2, i3, i4;
	double xr1, xi1, xr2, xi2, wrs, wis;
	double yr, yi, yr2, yi2, yr0, theta, x;

	n = fftNum / 2; 
	n2 = n / 2;
   	theta = M_PI / n;
   	FFT(s, fftNum, false);
   	x = sin(0.5 * theta);
   	yr2 = -2.0 * x * x;
   	yi2 = sin(theta); 
	yr = 1.0 + yr2; 
	yi = yi2;
   	for (i = 2; i <= n2; i++) 
	{
		i1 = i + i - 1;      
		i2 = i1 + 1;
      		i3 = n + n + 3 - i2; 
		i4 = i3 + 1;
      		wrs = yr; 
		wis = yi;
      		xr1 = (s[i1] + s[i3]) / 2.0; 
		xi1 = (s[i2] - s[i4]) / 2.0;
      		xr2 = (s[i2] + s[i4]) / 2.0; 
		xi2 = (s[i3] - s[i1]) / 2.0;
      		s[i1] = xr1 + wrs * xr2 - wis * xi2;
      		s[i2] = xi1 + wrs * xi2 + wis * xr2;
      		s[i3] = xr1 - wrs * xr2 + wis * xi2;
      		s[i4] = -xi1 + wrs * xi2 + wis * xr2;
      		yr0 = yr;
      		yr = yr * yr2 - yi  * yi2 + yr;
      		yi = yi * yr2 + yr0 * yi2 + yi;
   	}
   	xr1 = s[1];
   	s[1] = xr1 + s[2];
   	s[2] = 0.0;
}

/* EXPORT-> FFT: apply fft/invfft to complex s */
void CFft::FFT(float *s, int fftNum, int invert)
{
	int ii, jj, n, nn, limit, m, j, inc, i;
	double wx, wr, wpr, wpi, wi, theta;
   	double xre, xri, x;
   
   	n = fftNum;
   	nn = n / 2; 
	j = 1;
   	for (ii = 1; ii <= nn;ii++) 
	{
		i = 2 * ii - 1;
      		if (j > i) 
		{
         		xre = s[j]; 
			xri = s[j + 1];
         		s[j] = s[i];  
			s[j + 1] = s[i + 1];
         		s[i] = xre; 
			s[i + 1] = xri;
      		}
      		m = n / 2;
      		while (m >= 2  && j > m) 
		{
         		j -= m; 
			m /= 2;
      		}
      		j += m;
   	};
   	limit = 2;
   	while (limit < n) 
	{
      		inc = 2 * limit; 
		theta = 2.0 * M_PI / limit;	//TPI / limit;
      		if (invert) 
		{
			theta = -theta;
		}
      		x = sin(0.5 * theta);
      		wpr = -2.0 * x * x; 
		wpi = sin(theta); 
      		wr = 1.0; 
		wi = 0.0;
      		for (ii = 1; ii <= limit / 2; ii++) 
		{
         		m = 2 * ii - 1;
         		for (jj = 0; jj <= (n - m) / inc; jj++) 
			{
            			i = m + jj * inc;
            			j = i + limit;
            			xre = wr * s[j] - wi * s[j + 1];
            			xri = wr * s[j + 1] + wi * s[j];
            			s[j] = s[i] - xre; 
				s[j + 1] = s[i + 1] - xri;
            			s[i] = s[i] + xre; 
				s[i + 1] = s[i + 1] + xri;
         		}
         		wx = wr;
         		wr = wr * wpr - wi * wpi + wr;
         		wi = wi * wpr + wx * wpi + wi;
      		}
      		limit = inc;
   	}
   	if (invert)
	{
      		for (i = 1; i <= n; i++) 
		{
         		s[i] = s[i] / nn;   
		}
	}
}

