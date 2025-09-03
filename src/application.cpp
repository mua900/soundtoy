#include "application.h"

#include <SDL3_image/SDL_image.h>

#include <iostream>

#define SAMPLE_BUFFER_SIZE 1024
float sample_buffer[SAMPLE_BUFFER_SIZE];

void audio_callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
    additional_amount /= sizeof(float);

    Audio* audio = (Audio*)userdata;

    const float PI = 3.1415;

    for (int turn = 0; turn < additional_amount / SAMPLE_BUFFER_SIZE; turn++)
    {
        for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++)
        {
            sample_buffer[i] = sinf(audio->time * audio->sample_rate * 2 * PI);
        }
        SDL_PutAudioStreamData(stream, sample_buffer, SAMPLE_BUFFER_SIZE);
    }
}

Audio initialize_audio(int freq, int channels, bool* success)
{
    Audio audio = {};

    SDL_AudioDeviceID default_device = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;

    SDL_AudioSpec spec = {};
    spec.freq = freq;
    spec.channels = channels;
    spec.format = SDL_AUDIO_F32;

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

    return audio;
}

bool Application::load_assets()
{
    

    return true;
}

bool initialize(Application* app)
{
    app->quit = true;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        std::cerr << "Failed to init SDL\n";
        return false;
    }

    {
        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
        if (!SDL_CreateWindowAndRenderer("soundtoy", 1440, 810, flags, &window, &renderer))
        {
            std::cerr << "Failed to create window and renderer\n";
            return false;
        }

        app->window = { window, renderer };
    }

    {
        bool audio_success = false;
        app->audio = initialize_audio(48000, 1, &audio_success);
        if (!audio_success)
        {
            std::cerr << "Failed to initialize audio\n";
            return false;
        }
    }

    app->quit = false;

    return true;
}

void Application::handle_events()
{
    SDL_Event e = {};
    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
        case SDL_EVENT_QUIT:
            this->quit = true;
            break;
        case SDL_EVENT_KEY_DOWN:
        {
            SDL_KeyboardEvent keyboard = e.key;
            switch (keyboard.scancode)
            {
            case SDL_SCANCODE_ESCAPE:
                this->quit = true;
                break;
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        {
            SDL_MouseButtonEvent mouse = e.button;
            if (mouse_input_ui())
            {
                break;
            }

            break;
        }
        case SDL_EVENT_MOUSE_MOTION:
        {
            mouse.flags = SDL_GetMouseState(&mouse.pos.x, &mouse.pos.y);
            break;
        }
        default:
            break;
        }
    }
}


#define NS_PER_SECONDS 1'000'000'000

void Application::update()
{
    SDL_Time time = SDL_GetTicksNS();
    audio.time = (double)time / NS_PER_SECONDS;
    printf("%f\n", audio.time);
}

void Application::draw()
{
    SDL_Renderer* renderer = this->window.renderer;

    Window window = this->window;
    SDL_SetRenderDrawColor(renderer, 0xaa, 0x66, 0x33, 0xff);
    SDL_RenderClear(renderer);

    draw_ui();

    SDL_RenderPresent(renderer);
}

void Application::draw_ui()
{
    // volume slider
    {
        Rectangle volume_slider = ui.volume_slider;
        vec2 knob_scale = ui.volume_slider_knob_scale;

        const float slider_knob_width = volume_slider.w * knob_scale.x;
        const float slider_knob_height = volume_slider.h * knob_scale.y;

        SDL_SetRenderDrawColor(window.renderer, 0x55, 0x44, 0x22, 0xff);
        SDL_FRect slider = { volume_slider.x, volume_slider.y, volume_slider.w, volume_slider.h };
        SDL_RenderFillRect(window.renderer, &slider);

        float percentage = audio.volume;
        SDL_SetRenderDrawColor(window.renderer, 0x66, 0x55, 0x22, 0xff);
        SDL_FRect slider_knob = { volume_slider.x - (slider_knob_width / 2) + (volume_slider.w * percentage), volume_slider.y - volume_slider.h / 2,
                                    slider_knob_width, slider_knob_height };
        SDL_RenderFillRect(window.renderer, &slider_knob);
    }

    // pause button
    {
        Rectangle button = ui.pause_button;

        SDL_SetRenderDrawColor(window.renderer, 0x66, 0x55, 0x55, 0xff);
        SDL_FRect pbutton = {button.x, button.y, button.w, button.h};
        SDL_RenderFillRect(window.renderer, &pbutton);

        SDL_FRect src;
        SDL_FRect dst;
        SDL_RenderTexture(window.renderer, assets.pause_texture, &src, &dst);
    }
}

bool Application::mouse_input_ui()
{
    if (ui.volume_slider.contains(mouse.pos))
    {
        float diff = mouse.pos.x - ui.volume_slider.x;
        audio.volume = diff / ui.volume_slider.w;
        return true;
    }

    if (ui.pause_button.contains(mouse.pos))
    {
        if (audio.paused)
            audio.unpause();
        else
            audio.pause();

        audio.paused = !audio.paused;
    }

    return false;
}

void Audio::pause()
{
    SDL_PauseAudioDevice(playback);
}

void Audio::unpause()
{
    SDL_ResumeAudioDevice(playback);
}
