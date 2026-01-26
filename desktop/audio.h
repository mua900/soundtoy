#pragma once

#include <SDL3/SDL.h>
#include "common.h"
#include "template.h"
#include "api.h"

struct Audio {
    SDL_AudioDeviceID m_playback = 0;
    SDL_AudioStream* m_audio_stream = nullptr;
    SDL_AudioFormat m_format = SDL_AUDIO_F32;

	// Non owning pointers
	St_Sampler* sampler_left = nullptr;
	St_Sampler* sampler_right = nullptr;
	
	Array<float> sample_buffer;
	
    float m_volume = 0.0;

    int m_sample_rate = 0;
    int m_channel_count = 0;

    bool paused = true;

    bool initialize(Array<float> p_sample_buffer, int freq, int channels, St_Sampler* left, St_Sampler* right);
    bool reinitialize(int freq, int channels);
    void cleanup();

    void set_samplers(St_Sampler* left, St_Sampler* right) {
        sampler_left = left;
        sampler_right = right;
    }
	bool set_playback_device(SDL_AudioDeviceID device);
    int get_channel_count() const { return m_channel_count; }
    int get_sample_rate() const { return m_sample_rate; }
    void pause();
    void unpause();
    void toggle_pause();
    float get_volume();
    void set_volume(float volume);
private:
    bool set_channel_count(SDL_AudioStream* stream, int channel_count);	
};


#define DESIRED_AUDIO_FORMAT SDL_AUDIO_F32
#define DESIRED_AUDIO_SAMPLE_RATE 48000
#define DESIRED_AUDIO_CHANNEL_COUNT 2

struct AudioData {
    void* samples = nullptr;
    SDL_AudioFormat format = DESIRED_AUDIO_FORMAT;
    int channel_count = 0;
    int frequency = 0;
    int frame_count = 0;

    void reset()
    {
        if (samples) {
            free(samples);
        }

        samples = nullptr;
        format = DESIRED_AUDIO_FORMAT;
        channel_count = 0;
        frequency = 0;
        frame_count = 0;
    }

    bool is_in_desired_spec()
    {
        return (samples != nullptr) && (format == DESIRED_AUDIO_FORMAT) && (channel_count == 2) && (frequency == DESIRED_AUDIO_SAMPLE_RATE);
    }
};


struct AudioPlayer
{
	AudioData audio_data = {};
	SDL_AudioDeviceID device = {};
	SDL_AudioStream* stream = {};

	double volume = 0.0;
	bool paused = true;
	
	bool initialize(int freq, int channels, double vol);
	void destroy();

	bool set_audio_data(AudioData data);

	void pause();
	void resume();
	void toggle_pause();
	void set_volume(double volume);
	double get_volume();
};
