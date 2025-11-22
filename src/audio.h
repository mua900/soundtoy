#pragma once

#include <SDL3/SDL.h>
#include "evaluator.h"
#include "bytecode.h"

struct Audio {
    SDL_AudioDeviceID m_playback = 0;
    SDL_AudioStream* m_audio_stream = nullptr;

    Expr* sample_expression = nullptr;
    Evaluator evaluator = {};
    Bytecode_Program bytecode_program = {};

    double sample_time = 0;  // time variable used for sampling.

    // double m_time = 0;       // real life time since the application startup updated by the application
    int m_sample_rate = 0;
    int m_channel_count = 0;

    bool paused = true;

    bool initialize(int freq, int channels);
    bool reinitialize(int freq, int channels);
    void cleanup();

    void pause();
    void unpause();
    void toggle_pause();

    float get_volume();
    void set_volume(float volume);

    bool set_sample_expression(Expr* expr);

    bool create_audio_stream(int freq, int channels);
};
