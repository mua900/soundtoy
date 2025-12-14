#include "audio.h"
#include "common.h"

#include <math.h>

extern const double PI;

static void SDLCALL audio_callback_mono(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);
static void SDLCALL audio_callback_stereo(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);

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
        SDL_SetAudioStreamGetCallback(m_audio_stream, audio_callback_mono, m_samplers.sampler_left);
    }
    else if (channel_count == 2) {
        SDL_SetAudioStreamGetCallback(m_audio_stream, audio_callback_stereo, &m_samplers);
    }
    else {
        fprintf(stderr, "Invalid argument for channel count %d\n", channel_count);
        return false;
    }

    m_channel_count = channel_count;
    return true;
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

	m_samplers = Sampler_List{};
	
    m_audio_stream = NULL;
}

bool Audio::initialize(int freq, int channels, St_Sampler* left, St_Sampler* right)
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

    m_samplers.sampler_left = left;
    m_samplers.sampler_right = right;
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


#define SAMPLE_BUFFER_SIZE 512
float g_sample_buffer[SAMPLE_BUFFER_SIZE];

static void SDLCALL audio_callback_mono(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
    total_amount /= sizeof(float);

    St_Sampler* sampler = (St_Sampler*) userdata;

    for (int turn = 0; turn < total_amount / SAMPLE_BUFFER_SIZE + 1; turn++)
    {
        st_fill(sampler, g_sample_buffer, SAMPLE_BUFFER_SIZE);

        SDL_PutAudioStreamData(stream, g_sample_buffer, SAMPLE_BUFFER_SIZE);
    }
}

static void SDLCALL audio_callback_stereo(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
    total_amount /= sizeof(float);

	Sampler_List* samplers = (Sampler_List*) userdata;

    for (int turn = 0; turn < total_amount / SAMPLE_BUFFER_SIZE + 1; turn++)
    {
        st_fill_interleaved_double(samplers->sampler_left, samplers->sampler_right, g_sample_buffer, SAMPLE_BUFFER_SIZE / 2);

        SDL_PutAudioStreamData(stream, g_sample_buffer, SAMPLE_BUFFER_SIZE);
    }
}
