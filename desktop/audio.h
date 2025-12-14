#pragma once

#include <SDL3/SDL.h>
#include "common.h"
#include "api.h"

struct Sampler_List {
	// non-owning pointers
	St_Sampler* sampler_left = nullptr;
	St_Sampler* sampler_right = nullptr;
};

struct Audio {
    SDL_AudioDeviceID m_playback = 0;
    SDL_AudioStream* m_audio_stream = nullptr;

	Sampler_List m_samplers = {};
	
    int m_sample_rate = 0;
    int m_channel_count = 0;

    bool paused = true;

    bool initialize(int freq, int channels, St_Sampler* left, St_Sampler* right);
    bool reinitialize(int freq, int channels);
    void cleanup();

    bool set_channel_count(int channel_count);
    void set_samplers(St_Sampler* left, St_Sampler* right) {
        m_samplers.sampler_left = left;
        m_samplers.sampler_right = right;
    }

    int get_channel_count() const { return m_channel_count; }
    int get_sample_rate() const { return m_sample_rate; }

    void pause();
    void unpause();
    void toggle_pause();

    float get_volume();
    void set_volume(float volume);

    bool create_audio_stream(int freq, int channels);
};
