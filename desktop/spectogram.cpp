#include "spectogram.h"
#include "fourier.h"

#define WINDOW_SIZE 128

Spectogram calculate_spectogram(Signal signal)
{
    Spectogram spectogram;

    return spectogram;
}

Complex* calculate_fourier(Signal signal)
{
    // short time fourier transform
    Complex* frequencies = new Complex[signal.samples.size];

    dft(signal.samples.data, frequencies, signal.samples.size);

    return frequencies;
}

