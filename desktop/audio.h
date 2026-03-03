#pragma once

#include <SDL3/SDL.h>

#include "spectogram.h"
#include "common.h"
#include "template.h"
#include "api.h"

// @todo speed control for playback maybe? Take a look at FrequencyRatio.

struct ExpressionAudio {
    SDL_AudioDeviceID m_playback = 0;
    SDL_AudioStream* m_audio_stream = nullptr;
    SDL_AudioFormat m_format = SDL_AUDIO_F32;

    // Non owning pointers
    St_Sampler* sampler_left = nullptr;
    St_Sampler* sampler_right = nullptr;

    Array<float> sample_buffer;

    float m_volume = 0.0;
    float m_pan = 0.0;

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
    float get_volume() const { return m_volume; }
    void set_volume(float volume);
    float get_pan() const { return m_pan; }
    void set_pan(float pan) { m_pan = pan; }
private:
    bool set_channel_count(SDL_AudioStream* stream, int channel_count);
};

#define AUDIO_MAX_CHANNELS 8

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
        return (samples != nullptr) && (format == DESIRED_AUDIO_FORMAT) && (channel_count == DESIRED_AUDIO_CHANNEL_COUNT) && (frequency == DESIRED_AUDIO_SAMPLE_RATE);
    }

    Signal as_signal()
    {
        if (format != SDL_AUDIO_F32)
        {
            // we want floats so fail
            return Signal();
        }

        // @todo do this properly with respecting channels
        return Signal((float*)samples, frame_count * channel_count);
    }
};


struct AudioPlayer
{
    AudioData* audio_data = {};  // reference
    SDL_AudioDeviceID device = {};
    SDL_AudioStream* stream = {};
    int playback_position = 0;

    double volume = 0.0;
    double pan = 0.0;
    bool paused = true;

    bool initialize(int freq, int channels, double vol);
    void destroy();

    bool set_audio_data(AudioData* data);
    void reset_audio_data();

    void put_audio_data();

    void pause();
    void resume();
    void toggle_pause();
    void set_volume(double volume);
    double get_volume() const;
    double get_pan() const { return pan; }
    void set_pan(float p) { pan = p; }
};
