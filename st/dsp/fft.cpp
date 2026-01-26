#include "fft.h"
#include "common.h"

#include <cmath>

void dft(Complex* input, Complex* output, int count)
{
	for (int f = 0; f < count; f++)
	{
		Complex accum = Complex();
		
		for (int j = 0; j < count; j++)
		{
			float k = - (float)j / count;
			accum.real += cosf(2.0*MATH_PI*f*k);
			accum.imaginary += sinf(2.0*MATH_PI*f*k);
		}

		output[f] = accum;
	}
}


void _fft(float* input, float* output, int count, int stride)
{
	if (count <= 1)
		return;

	// even
	_fft(input,          output,             count / 2, stride*2);
	// odd
	_fft(input + stride, output + count / 2, count / 2, stride*2);

	for (int k = 0; k < count/2; k++)
	{
		float t = (float) k / count;
		float r = -2.0*MATH_PI*t;

		float even = output[k];
		float odd = output[k + count/2];
		
		float v = sinf(odd*r);

		output[k]           = even + v;
		output[k + count/2] = even - v;
	}
}

void fft(float* input, float* output, int count)
{
	_fft(input, output, count, 1);
}
