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
