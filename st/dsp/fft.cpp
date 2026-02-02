#include "fft.h"
#include "common.h"

#include <complex.h>
#include <cmath>

void dft(float* input, Complex* output, int count)
{
	for (int f = 0; f < count; f++)
	{
		Complex accum = Complex();
		
		for (int j = 0; j < count; j++)
		{
			float k = (float)j / count;
			float r = - 2.0*M_PI*f*k;

			accum.real      += input[j] * cosf(r);
			accum.imaginary += input[j] * sinf(r);
		}

		output[f] = accum;
	}
}


void _fft(float* input, Complex* output, int count, int stride)
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
		float r = -2.0*M_PI*t;

		Complex even = output[k];
		Complex odd = output[k + count/2];
		
		Complex v = Complex(cosf(r) * (odd.real + odd.imaginary), sinf(r) * (odd.real + odd.imaginary));

		output[k]           = even + v;
		output[k + count/2] = even - v;
	}
}

void fft(float* input, Complex* output, int count)
{
	_fft(input, output, count, 1);
}
