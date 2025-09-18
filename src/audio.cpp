#include "audio.h"
#include "common.h"

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

float Audio::get_volume()
{
    return SDL_GetAudioDeviceGain(m_playback);
}

void Audio::set_volume(float volume)
{
    SDL_SetAudioDeviceGain(m_playback, volume);
}


#define SAMPLE_BUFFER_SIZE 1024
float sample_buffer[SAMPLE_BUFFER_SIZE];

void SDLCALL audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
    total_amount /= sizeof(float);

    Audio* audio = (Audio*)userdata;
    auto evaluator = audio->m_evaluator;

    const double PI = 3.1415;

    for (int turn = 0; turn < total_amount / SAMPLE_BUFFER_SIZE + 1; turn++)
    {
        for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++)
        {
            sample_buffer[i] = sinf(audio->m_time * audio->m_sample_rate * 2 * PI);
        }

        SDL_PutAudioStreamData(stream, sample_buffer, SAMPLE_BUFFER_SIZE);
    }
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

    SDL_SetAudioStreamGetCallback(stream, audio_callback, this);

    m_audio_stream = stream;

    return true;
}

void Audio::cleanup()
{
    SDL_CloseAudioDevice(m_playback);
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

    return true;
}

bool Audio::reinitialize(int freq, int channels)
{
    SDL_DestroyAudioStream(m_audio_stream);

    return create_audio_stream(freq, channels);
}
