#include "audio.h"
#include "common.h"

#include <math.h>

extern const double PI;

void SDLCALL audio_callback_mono(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);
void SDLCALL audio_callback_stereo(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);

void Audio::pause()
{
    paused = true;
    SDL_PauseAudioDevice(m_playback);
}

void Audio::unpause()
{
    paused = false;
    SDL_ResumeAudioDevice(m_playback);
}

void Audio::toggle_pause()
{
    if (paused) {
        unpause();
    }
    else {
        pause();
    }
}

float Audio::get_volume()
{
    return SDL_GetAudioDeviceGain(m_playback);
}

void Audio::set_volume(float volume)
{
    SDL_SetAudioDeviceGain(m_playback, volume);
}

bool Audio::set_channel_count(int channel_count) {
    if (channel_count == 1) {
        SDL_SetAudioStreamGetCallback(m_audio_stream, audio_callback_mono, this);
    }
    else if (channel_count == 2) {
        SDL_SetAudioStreamGetCallback(m_audio_stream, audio_callback_stereo, this);
    }
    else {
        return false;
    }

    m_channel_count = channel_count;
    return true;
}

bool Audio::set_sample_expression(String expr)
{
    bool left = st_sampler_set_expression(sampler, expr.data, expr.size);
    bool right = st_sampler_set_expression(sampler2, expr.data, expr.size);

    return left && right;
}


bool Audio::create_audio_stream(int freq, int channels)
{
    SDL_AudioSpec spec = {};
    spec.freq = freq;
    spec.channels = channels;
    spec.format = SDL_AUDIO_F32;

    SDL_AudioSpec device_spec = {};
    if (!SDL_GetAudioDeviceFormat(m_playback, &device_spec, NULL))
    {
        return false;
    }

    SDL_AudioStream* stream = SDL_CreateAudioStream(&spec, &device_spec);
    if (!stream)
    {
        return false;
    }

    if (!SDL_BindAudioStream(m_playback, stream))
    {
        SDL_DestroyAudioStream(stream);
        return false;
    }

    m_audio_stream = stream;
    m_sample_rate = freq;

    if (!set_channel_count(channels)) {
        return false;
    }

    return true;
}

void Audio::cleanup()
{
    SDL_CloseAudioDevice(m_playback);
    SDL_DestroyAudioStream(m_audio_stream);

    st_sampler_destroy(sampler);
    st_sampler_destroy(sampler2);

    m_audio_stream = NULL;
}

bool Audio::initialize(int freq, int channels)
{
    SDL_AudioSpec spec = {};
    spec.freq = freq;
    spec.channels = channels;
    spec.format = SDL_AUDIO_F32;

    SDL_AudioDeviceID default_device = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;

    m_playback = SDL_OpenAudioDevice(default_device, &spec);
    if (!m_playback)
    {
        return false;
    }

    create_audio_stream(freq, channels);

    SDL_PauseAudioDevice(m_playback);

    SDL_SetAudioDeviceGain(m_playback, 0.0);

    printf("Audio Backend: %s\n", SDL_GetCurrentAudioDriver());
    auto dev_name = SDL_GetAudioDeviceName(m_playback);
    if (dev_name)
    {
        printf("Audio Device Name: %s\n", dev_name);
    }

    m_sample_rate = freq;
    m_channel_count = channels;

    sampler = st_sampler_create(Evaluator_Type::TREE_INTERP, freq);
    sampler2 = st_sampler_create(Evaluator_Type::TREE_INTERP, freq);

    String default_expression = make_string("sin(2*PI*t*440)");
    if (!set_sample_expression(default_expression)) {
        return false;
    }

    return true;
}

bool Audio::reinitialize(int freq, int channels)
{
    SDL_DestroyAudioStream(m_audio_stream);

    st_sampler_set_sample_rate(sampler, freq);
    st_sampler_set_sample_time(sampler, 0.0);
    
    st_sampler_set_sample_rate(sampler2, freq);
    st_sampler_set_sample_time(sampler2, 0.0);

    return create_audio_stream(freq, channels);
}


double Audio::get_sample_time_left() {
    return st_sampler_get_sample_time(sampler);
}

double Audio::get_sample_time_right() {
    return st_sampler_get_sample_time(sampler2);
}

double Audio::get_sample_time_mono() {
    return get_sample_time_left();
}


#define SAMPLE_BUFFER_SIZE 512
float g_sample_buffer[SAMPLE_BUFFER_SIZE];

void SDLCALL audio_callback_mono(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
    total_amount /= sizeof(float);

    Audio* audio = (Audio*)userdata;
    St_Sampler* sampler = audio->sampler;

    for (int turn = 0; turn < total_amount / SAMPLE_BUFFER_SIZE + 1; turn++)
    {
        st_fill(sampler, g_sample_buffer, SAMPLE_BUFFER_SIZE);

        SDL_PutAudioStreamData(stream, g_sample_buffer, SAMPLE_BUFFER_SIZE);
    }
}

void SDLCALL audio_callback_stereo(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
    total_amount /= sizeof(float);
    Audio* audio = (Audio*)userdata;

    St_Sampler* sampler = audio->sampler;
    St_Sampler* sampler2 = audio->sampler2;

    for (int turn = 0; turn < total_amount / SAMPLE_BUFFER_SIZE + 1; turn++)
    {
        st_fill_interleaved(sampler, sampler2, g_sample_buffer, SAMPLE_BUFFER_SIZE);

        SDL_PutAudioStreamData(stream, g_sample_buffer, SAMPLE_BUFFER_SIZE);
    }
}
