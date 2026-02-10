#pragma once

#include "common.h"

void dft(float* signal, Complex* output, int count);
void fft(float* signal, Complex* output, int count);
void ifft(float* signal, Complex* output, int count);
