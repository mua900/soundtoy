#include "audio.h"
#include "common.h"

#include <math.h>

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
    return m_volume;
}

void Audio::set_volume(float volume)
{
    m_volume = volume;
    SDL_SetAudioDeviceGain(m_playback, volume);  // @fix
}

bool Audio::set_channel_count(int channel_count) {
    if (channel_count == 1) {
        SDL_SetAudioStreamGetCallback(m_audio_stream, audio_callback_mono, this);
    }
    else if (channel_count == 2) {
        SDL_SetAudioStreamGetCallback(m_audio_stream, audio_callback_stereo, this);
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

bool Audio::initialize(Array<float> p_sample_buffer, int freq, int channels, St_Sampler* left, St_Sampler* right)
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

	this->sample_buffer = p_sample_buffer;

    return true;
}

bool Audio::reinitialize(int freq, int channels)
{
    SDL_DestroyAudioStream(m_audio_stream);

    return create_audio_stream(freq, channels);
}


static void SDLCALL audio_callback_mono(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
    total_amount /= sizeof(float);

	Audio* audio = (Audio*) userdata;
	
    St_Sampler* sampler = audio->m_samplers.sampler_left;
	auto sample_buffer = audio->sample_buffer;

    for (int turn = 0; turn < total_amount / sample_buffer.size + 1; turn++)
    {
        st_fill(sampler, sample_buffer.data, sample_buffer.size);

        SDL_PutAudioStreamData(stream, sample_buffer.data, sample_buffer.size);
    }
}

static void SDLCALL audio_callback_stereo(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
    total_amount /= sizeof(float);

	Audio* audio = (Audio*)userdata;

	Sampler_List* samplers = &audio->m_samplers;
	auto sample_buffer = audio->sample_buffer;

    for (int turn = 0; turn < total_amount / sample_buffer.size; turn++)
    {
        st_fill_interleaved(samplers->sampler_left, samplers->sampler_right, sample_buffer.data, sample_buffer.size / 2);

        SDL_PutAudioStreamData(stream, sample_buffer.data, sample_buffer.size);
    }
}
