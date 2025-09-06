#pragma once

#include <SDL3/SDL.h>

struct Audio {
    SDL_AudioDeviceID playback = 0;
    SDL_AudioStream* audio_stream = NULL;

    double time = 0;
    int sample_rate = 0;

    float volume = 0;
    bool paused = true;

    void pause();
    void unpause();

    float get_volume();
    void set_volume(float volume);
};

Audio initialize_audio(int freq, int channels, bool* success);
