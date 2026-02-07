#include "spectogram.h"
#include "fourier.h"

#define WINDOW_SIZE 128

Spectogram calculate_spectogram(Signal signal)
{
    Spectogram spectogram;

    return spectogram;
}

DArray<Complex> calculate_fourier(Signal signal)
{
    DArray<Complex> frequencies = DArray<Complex>(signal.samples.size);

    dft(signal.samples.data, frequencies.data(), signal.samples.size);

    return frequencies;
}
