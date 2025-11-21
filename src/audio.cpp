#include "audio.h"
#include "common.h"

#include <math.h>

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
    if (paused)
    {
        unpause();
    }
    else
    {
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


void SDLCALL audio_callback_default(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
    total_amount /= sizeof(float);

    Audio* audio = (Audio*)userdata;

#define BUFFER_SIZE 512
    float buffer[BUFFER_SIZE];

    for (int turn = 0; turn < total_amount / BUFFER_SIZE + 1; turn++)
    {
        for (int i = 0; i < BUFFER_SIZE; i++)
        {
            buffer[i] = sinf(audio->sample_time);
        }

        SDL_PutAudioStreamData(stream, buffer, BUFFER_SIZE);
    }
}

#define SAMPLE_BUFFER_SIZE 512
float g_sample_buffer[SAMPLE_BUFFER_SIZE];

void SDLCALL audio_callback_sample(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
    total_amount /= sizeof(float);

    Audio* audio = (Audio*)userdata;
    Expr* expr = audio->sample_expression;

    double inv_sample_rate = 1.0 / audio->m_sample_rate;

    for (int turn = 0; turn < total_amount / SAMPLE_BUFFER_SIZE + 1; turn++)
    {
        for (int sample = 0; sample < SAMPLE_BUFFER_SIZE; sample++)
        {
            Eval eval = audio->evaluator.evaluate(expr);

            /*
                we assume the evaluator is able to evaluate the expression here since
                we are in the callback and we shouldn't be here if we have an invalid expression.
                And we don't want to check in the callback.
            */ 
            float value = SDL_clamp(eval.value, 0.0, 1.0);
            g_sample_buffer[sample] = value;

            audio->evaluator.step_time(inv_sample_rate);
        }

        SDL_PutAudioStreamData(stream, g_sample_buffer, SAMPLE_BUFFER_SIZE);
    }
}

bool Audio::set_sample_expression(Expr* expr)
{
    Eval eval = evaluator.evaluate(expr);
    if (eval.success) {
        printf("%f\n", eval.value);

        sample_expression = expr;
        evaluator.reset(m_sample_rate, 0.0);

        SDL_SetAudioStreamGetCallback(m_audio_stream, audio_callback_sample, this);

        return true;
    }
    else {
        return false;
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

    SDL_SetAudioStreamGetCallback(stream, audio_callback_default, this);

    m_audio_stream = stream;

    return true;
}

void Audio::cleanup()
{
    SDL_CloseAudioDevice(m_playback);
    SDL_DestroyAudioStream(m_audio_stream);

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

    evaluator.set(freq, 0.0);

    return true;
}

bool Audio::reinitialize(int freq, int channels)
{
    SDL_DestroyAudioStream(m_audio_stream);

    evaluator.set(freq, 0.0);

    return create_audio_stream(freq, channels);
}
