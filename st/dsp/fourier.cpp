#include "fourier.h"

Complex calculate_frequency_value(float* signal, int count, int frequency);

void dft(float* signal, Complex* output, int count)
{
    for (int f = 0; f < count; f++)
    {
        Complex freq = calculate_frequency_value(signal, count, f);

        output[f] = freq;
    }
}

Complex calculate_frequency_value(float* signal, int count, int frequency)
{
    Complex accum = {};

    for (int k = 0; k < count; k++)
    {
        float t = (float) k / count;
        float c = cos(CONSTANT_TAU*t*frequency)*signal[k];
        float s = sin(CONSTANT_TAU*t*frequency)*signal[k];

        accum.real += c;
        accum.imaginary += s;
    }

    return accum;
}

void fft(float* signal, Complex* output, int count)
{
    
}

void ifft(float* signal, Complex* output, int count)
{
    if (count <= 1)
    {
        return;
    }

    float* even = output;
    float* odd  = output + stride;

    // divide & conquer
    fft(signal,          output, count / 2, stride + 1);
    fft(signal + stride, output, count / 2, stride + 1);

    // combine
    for (int k = 0; k < count / 2; k++)
    {
        float t = (float) k / count;

        float arg = 2.0*M_PI*k/count;
        float o = output[(k+1) * stride] * Complex(cos(arg), sin(arg));
        output[k]             = output[k * stride] + o;
        output[k + count / 2] = output[k * stride] - o;
    }
}
