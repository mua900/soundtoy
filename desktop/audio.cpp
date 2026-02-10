#include "audio.h"
#include "common.h"

#include <math.h>

SDL_AudioStream* create_audio_stream(SDL_AudioDeviceID device, SDL_AudioSpec spec, int freq, int channels, double volume);

static void SDLCALL expression_audio_callback_mono(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);
static void SDLCALL expression_audio_callback_stereo(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount);

void ExpressionAudio::pause()
{
    paused = true;
    SDL_PauseAudioDevice(m_playback);
}

void ExpressionAudio::unpause()
{
    paused = false;
    SDL_ResumeAudioDevice(m_playback);
}

void ExpressionAudio::toggle_pause()
{
    if (paused) {
        unpause();
    }
    else {
        pause();
    }
}

void ExpressionAudio::set_volume(float volume)
{
    m_volume = volume;
    SDL_SetAudioStreamGain(m_audio_stream, volume);
}

bool ExpressionAudio::set_channel_count(SDL_AudioStream* stream, int channel_count) {
    if (channel_count == 1) {
        SDL_SetAudioStreamGetCallback(stream, expression_audio_callback_mono, this);
    }
    else if (channel_count == 2) {
        SDL_SetAudioStreamGetCallback(stream, expression_audio_callback_stereo, this);
    }
    else {
        fprintf(stderr, "Invalid argument for channel count %d\n", channel_count);
        return false;
    }

    m_channel_count = channel_count;
    return true;
}


void ExpressionAudio::cleanup()
{
    SDL_CloseAudioDevice(m_playback);
    SDL_DestroyAudioStream(m_audio_stream);

    sampler_left = nullptr;
    sampler_right = nullptr;

    m_audio_stream = NULL;
}

bool ExpressionAudio::initialize(Array<float> p_sample_buffer, int freq, int channels, St_Sampler* left, St_Sampler* right)
{
    SDL_AudioSpec spec = {};
    spec.freq = freq;
    spec.channels = channels;
    spec.format = SDL_AUDIO_F32;

    auto device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);

    if (!device) {
        fprintf(stderr, "Could not open audio device %s. %s\n", SDL_GetAudioDeviceName(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK), SDL_GetError());
        return false;
    }

    SDL_PauseAudioDevice(device);

    SDL_AudioStream* stream = create_audio_stream(device, spec, freq, channels, 0.0);
    if (!stream)
    {
        fprintf(stderr, "Could not create audio stream %s\n", SDL_GetError());
        return false;
    }

    if (!set_channel_count(stream, channels)) {
        return false;
    }

    m_playback = device;

    sampler_left = left;
    sampler_right = right;

    m_audio_stream = stream;
    m_format = spec.format;
    m_sample_rate = freq;
    m_channel_count = channels;

    m_volume = 0.0;

    this->sample_buffer = p_sample_buffer;

    printf("Audio Backend: %s\n", SDL_GetCurrentAudioDriver());  // @debug
    auto dev_name = SDL_GetAudioDeviceName(m_playback);
    if (dev_name)
    {
        printf("Audio Device Name: %s\n", dev_name);  // @debug
    }

    return true;
}

bool ExpressionAudio::reinitialize(int freq, int channels) {
    pause();

    SDL_DestroyAudioStream(m_audio_stream);
    m_audio_stream = NULL;

    SDL_AudioSpec spec = {};
    spec.freq = freq;
    spec.channels = channels;
    spec.format = SDL_AUDIO_F32;

    SDL_AudioStream* stream = create_audio_stream(m_playback, spec, freq, channels, m_volume);
    if (!stream)
    {
        return false;
    }

    if (!set_channel_count(stream, channels))
    {
        return false;
    }

    m_audio_stream = stream;

    return true;
}

bool ExpressionAudio::set_playback_device(SDL_AudioDeviceID p_device) {
    if (m_playback) {
        SDL_CloseAudioDevice(m_playback);
        m_playback = 0;
    }

    SDL_AudioSpec spec = {};
    spec.freq = m_sample_rate;
    spec.channels = m_channel_count;
    spec.format = SDL_AUDIO_F32;

    auto device = SDL_OpenAudioDevice(p_device, &spec);

    if (!device) {
        fprintf(stderr, "Could not open audio device. %s\n", SDL_GetError());
        return false;
    }

    SDL_PauseAudioDevice(m_playback);

    m_playback = device;

    if (!reinitialize(m_sample_rate, m_channel_count)) {
        return false;
    }

    return true;
}

// @todo use SDL_PutAudioStreamDataNoCopy instead

static void SDLCALL expression_audio_callback_mono(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
    total_amount /= sizeof(float);

    ExpressionAudio* audio = (ExpressionAudio*) userdata;

    St_Sampler* sampler = audio->sampler_left;
    auto sample_buffer = audio->sample_buffer;

    for (int turn = 0; turn < total_amount / sample_buffer.size; turn++)
    {
        st_fill(sampler, sample_buffer.data, sample_buffer.size);

        SDL_PutAudioStreamData(stream, sample_buffer.data, sample_buffer.size);
    }

    int remaining = total_amount % sample_buffer.size;
    if (remaining > 0) {
        st_fill(audio->sampler_left, sample_buffer.data, remaining);
        SDL_PutAudioStreamData(stream, sample_buffer.data, remaining);
    }
}

static void SDLCALL expression_audio_callback_stereo(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
    total_amount /= sizeof(float) * 2;

    ExpressionAudio* audio = (ExpressionAudio*)userdata;

    auto sample_buffer = audio->sample_buffer;

    for (int turn = 0; turn < total_amount / sample_buffer.size; turn++)
    {
        st_fill_interleaved(audio->sampler_left, audio->sampler_right, sample_buffer.data, sample_buffer.size / 2);

        SDL_PutAudioStreamData(stream, sample_buffer.data, sample_buffer.size);
    }

    int remaining = total_amount % sample_buffer.size;
    st_fill_interleaved(audio->sampler_left, audio->sampler_right, sample_buffer.data, remaining / 2);
    SDL_PutAudioStreamData(stream, sample_buffer.data, remaining);
}



#define QUEUE_SAMPLE_SIZE 44100
#define QUEUE_FRAME_SIZE QUEUE_SAMPLE_SIZE * DESIRED_AUDIO_CHANNEL_COUNT

void AudioPlayer::put_audio_data()
{
    if (!this->audio_data.samples)
        return;

    if (paused)
        return;

    int queued = SDL_GetAudioStreamQueued(stream);

    // @todo variable names

    // dependent on desired format
    float* audio_samples = (float*) this->audio_data.samples;
    int queued_samples = queued / sizeof(float);
    int queued_frames = queued_samples / DESIRED_AUDIO_CHANNEL_COUNT;

    if (queued_frames < QUEUE_FRAME_SIZE)
    {
        int put_amount = MIN(QUEUE_FRAME_SIZE, audio_data.frame_count - playback_position);
        SDL_PutAudioStreamData(stream, audio_samples + playback_position, QUEUE_FRAME_SIZE * sizeof(float));
        playback_position += put_amount;

        if (playback_position >= audio_data.frame_count)
        {
            // we are done playing the track
            playback_position = 0;

            // @todo pause
        }
    }
}

bool AudioPlayer::initialize(int freq, int channels, double vol)
{
    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_F32;
    spec.freq = freq;
    spec.channels = channels;

    SDL_AudioDeviceID device_id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);

    if (!device_id)
    {
        fprintf(stderr, "Could not open audio device %s. %s\n", SDL_GetAudioDeviceName(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK), SDL_GetError());
        return false;
    }

    SDL_PauseAudioDevice(device_id);
    SDL_SetAudioDeviceGain(device_id, 1.0);

    SDL_AudioStream* astream = create_audio_stream(device, spec, freq, channels, vol);
    if (!astream)
    {
        return false;
    }

    this->playback_position = 0;
    this->device = device_id;
    this->stream = astream;
    this->volume = vol;

    return true;
}

void AudioPlayer::destroy()
{
    SDL_CloseAudioDevice(device);
    SDL_DestroyAudioStream(stream);
}

bool AudioPlayer::set_audio_data(AudioData data)
{
    if (!data.is_in_desired_spec())
    {
        fprintf(stderr, "Unexpected audio data format\n");
        return false;
    }

    SDL_PauseAudioDevice(device);
    SDL_FlushAudioStream(stream);

    audio_data = data;

    // automatically resume with the new data?

    return true;
}

void AudioPlayer::pause()
{
    paused = true;
    SDL_PauseAudioDevice(device);
}

void AudioPlayer::resume()
{
    paused = false;
    SDL_ResumeAudioDevice(device);
}

void AudioPlayer::toggle_pause()
{
    if (paused) {
        resume();
    }
    else {
        pause();
    }
}

double AudioPlayer::get_volume() const
{
    return volume;
}

void AudioPlayer::set_volume(double p_volume)
{
    volume = p_volume;
    SDL_SetAudioStreamGain(stream, volume);
}


SDL_AudioStream* create_audio_stream(SDL_AudioDeviceID device, SDL_AudioSpec spec, int freq, int channels, double volume)
{
    SDL_AudioSpec device_spec = {};
    if (!SDL_GetAudioDeviceFormat(device, &device_spec, NULL))
    {
        return nullptr;
    }

    SDL_AudioStream* stream = SDL_CreateAudioStream(&spec, &device_spec);
    SDL_SetAudioStreamGain(stream, volume);

    if (!stream)
    {
        return nullptr;
    }

    if (!SDL_BindAudioStream(device, stream))
    {
        SDL_DestroyAudioStream(stream);
        return nullptr;
    }

    return stream;
}
