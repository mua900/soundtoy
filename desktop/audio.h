#pragma once

#include <SDL3/SDL.h>
#include "common.h"
#include "api.h"

struct Audio {
    SDL_AudioDeviceID m_playback = 0;
    SDL_AudioStream* m_audio_stream = nullptr;

    St_Sampler* sampler = nullptr;
    St_Sampler* sampler2 = nullptr;

    int m_sample_rate = 0;
    int m_channel_count = 0;

    bool paused = true;

    bool initialize(int freq, int channels);
    bool reinitialize(int freq, int channels);
    void cleanup();

    // time variable used for sampling.
    // Incremented 1 / sample_rate after each sample.
    // This isn't real world time, this is the the point we are about the sample from the function.
    double get_sample_time_left();
    double get_sample_time_right();
    double get_sample_time_mono();

    bool set_channel_count(int channel_count);

    int get_channel_count() const { return m_channel_count; }
    int get_sample_rate() const { return m_sample_rate; }

    void pause();
    void unpause();
    void toggle_pause();

    float get_volume();
    void set_volume(float volume);

    bool set_sample_expression(String expr);

    bool create_audio_stream(int freq, int channels);
};
