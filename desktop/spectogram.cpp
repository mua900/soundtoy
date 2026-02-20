#include "spectogram.h"
#include "fourier.h"

#define WINDOW_SIZE 128

Spectogram calculate_spectogram(Signal signal)
{
    DArray<float> samples;
    int time_window;
    int freq_count;


    return Spectogram(Array<float> (samples.data(), samples.size()));
}

DArray<Complex> calculate_fourier(Signal signal)
{
    DArray<Complex> frequencies = DArray<Complex>(signal.samples.size);

    dft(signal.samples.data, frequencies.data(), signal.samples.size);

    return frequencies;
}
