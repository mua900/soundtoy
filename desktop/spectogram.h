#pragma once

#include "template.h"
#include "common.h"

struct Signal {
    Array<float> samples;

    Signal() {}
    Signal(float* p_samples, int p_sample_count)
        : samples(p_samples, p_sample_count)
    {}
};

struct Spectogram {
    Array<float> spectogram;
    int time_window_count;
    int frequency_count;
};

Spectogram calculate_spectogram(Signal signal);
DArray<Complex> calculate_fourier(Signal signal);
