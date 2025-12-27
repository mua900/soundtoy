#pragma once

#include <SDL3/SDL.h>
#include "common.h"
#include "template.h"
#include "api.h"

struct Audio {
    SDL_AudioDeviceID m_playback = 0;
    SDL_AudioStream* m_audio_stream = nullptr;

	St_Sampler* m_sampler_left = nullptr;
	St_Sampler* m_sampler_right = nullptr;
	Array<float> sample_buffer;
	
    float m_volume = 0.0;

    int m_sample_rate = 0;
    int m_channel_count = 0;

    bool paused = true;

    bool initialize(Array<float> p_sample_buffer, int freq, int channels, St_Sampler* left, St_Sampler* right);
    bool reinitialize(int freq, int channels);
    void cleanup();

    bool set_channel_count(int channel_count);
    void set_samplers(St_Sampler* left, St_Sampler* right) {
        m_sampler_left = left;
        m_sampler_right = right;
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
