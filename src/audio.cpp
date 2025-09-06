#include "audio.h"
#include "common.h"

void Audio::pause()
{
    SDL_PauseAudioDevice(playback);
}

void Audio::unpause()
{
    SDL_ResumeAudioDevice(playback);
}

float Audio::get_volume()
{
    return SDL_GetAudioDeviceGain(playback);
}

void Audio::set_volume(float volume)
{
    SDL_SetAudioDeviceGain(playback, volume);
}


#define SAMPLE_BUFFER_SIZE 1024
float sample_buffer[SAMPLE_BUFFER_SIZE];

void SDLCALL audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
    additional_amount /= sizeof(float);

    Audio* audio = (Audio*)userdata;

    const float PI = 3.1415;

    for (int turn = 0; turn < additional_amount / SAMPLE_BUFFER_SIZE + 1; turn++)
    {
        for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++)
        {
            sample_buffer[i] = 1.0; //  sinf(audio->time * audio->sample_rate * 2 * PI);
        }

        SDL_PutAudioStreamData(stream, sample_buffer, SAMPLE_BUFFER_SIZE);
    }
}

Audio initialize_audio(int freq, int channels, bool* success)
{
    Audio audio = {};
    SDL_AudioSpec spec = {};
    spec.freq = freq;
    spec.channels = channels;
    spec.format = SDL_AUDIO_F32;

    SDL_AudioDeviceID default_device = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;


    audio.playback = SDL_OpenAudioDevice(default_device, &spec);
    if (!audio.playback)
    {
        *success = false;
        return audio;
    }

    SDL_AudioSpec device_spec = {};
    if (!SDL_GetAudioDeviceFormat(audio.playback, &device_spec, NULL))
    {
        *success = false;
        return audio;
    }

    audio.audio_stream = SDL_CreateAudioStream(&spec, &device_spec);
    if (!audio.audio_stream)
    {
        *success = false;
        return audio;
    }

    *success = SDL_BindAudioStream(audio.playback, audio.audio_stream);

    SDL_SetAudioStreamGetCallback(audio.audio_stream, audio_callback, &audio);

    *success = audio.audio_stream ? true : false;

    SDL_PauseAudioDevice(audio.playback);

    SDL_SetAudioDeviceGain(audio.playback, audio.volume);  // start with the default value

    printf("Audio Backend: %s\n", SDL_GetCurrentAudioDriver());
    auto dev_name = SDL_GetAudioDeviceName(audio.playback);
    if (dev_name)
    {
        printf("Audio Device Name: %s\n", dev_name);
    }

    return audio;
}


