#pragma once

#include <SDL3/SDL.h>
#include "evaluator.h"

struct Audio {
    SDL_AudioDeviceID m_playback = 0;
    SDL_AudioStream* m_audio_stream = nullptr;

    Evaluator m_evaluator = {};

    double m_time = 0;
    int m_sample_rate = 0;
    int m_channel_count = 0;

    bool paused = true;

    bool initialize(int freq, int channels);
    bool reinitialize(int freq, int channels);

    void pause();
    void unpause();

    float get_volume();
    void set_volume(float volume);
};
