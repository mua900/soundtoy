#include "application.h"

#include <SDL3_image/SDL_image.h>

#include <iostream>

bool Application::initialize()
{
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

        m_window = { window, renderer };
    }

    {
        bool audio_success = false;
        m_audio = initialize_audio(48000, 1, &audio_success);
        if (!audio_success)
        {
            std::cerr << "Failed to initialize audio\n";
            return false;
        }
    }

    if (!load_assets())
    {
        return false;
    }

    m_quit = false;

    return true;
}

bool Application::load_assets()
{
    String_Builder sb(256);
    sb.append(make_string(SDL_GetBasePath()));
#ifdef _WIN32
    String path_seperator = make_string("\\");
#else
    String path_seperator = make_string("/");
#endif
    sb.append(make_string("asset"));
    sb.append(path_seperator);

    {
        String pause_texture = make_string("pause.png");
        sb.append(pause_texture);
        if (!IMG_LoadTexture(m_window.renderer, sb.c_string()))
        {
            LOG_ERROR("Failed to load pause texture");
            return false;
        }
        sb.remove(pause_texture.size);
    }

    {
        String resume_texture = make_string("resume.png");
        sb.append(resume_texture);
        if (!IMG_LoadTexture(m_window.renderer, sb.c_string()))
        {
            LOG_ERROR("Failed to load resume texture");
            return false;
        }
        sb.remove(resume_texture.size);
    }


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
            this->m_quit = true;
            break;
        case SDL_EVENT_KEY_DOWN:
        {
            SDL_KeyboardEvent keyboard = e.key;
            switch (keyboard.scancode)
            {
            case SDL_SCANCODE_ESCAPE:
                this->m_quit = true;
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
            m_mouse.flags = SDL_GetMouseState(&m_mouse.pos.x, &m_mouse.pos.y);
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
    m_audio.time = (double)time / NS_PER_SECONDS;
}

void Application::draw()
{
    SDL_Renderer* renderer = this->m_window.renderer;

    Window window = this->m_window;
    SDL_SetRenderDrawColor(renderer, COLOR_ARG(m_background_color));
    SDL_RenderClear(renderer);

    draw_ui();

    SDL_RenderPresent(renderer);
}

void Application::draw_ui()
{
    // volume slider
    {
        Rectangle volume_slider = m_ui.volume_slider;
        vec2 knob_scale = m_ui.volume_slider_knob_scale;

        const float slider_knob_width = volume_slider.w * knob_scale.x;
        const float slider_knob_height = volume_slider.h * knob_scale.y;

        SDL_SetRenderDrawColor(m_window.renderer, 0x55, 0x44, 0x22, 0xff);
        SDL_FRect slider = { volume_slider.x, volume_slider.y, volume_slider.w, volume_slider.h };
        SDL_RenderFillRect(m_window.renderer, &slider);

        float percentage = m_audio.volume;
        SDL_SetRenderDrawColor(m_window.renderer, 0x66, 0x55, 0x22, 0xff);
        SDL_FRect slider_knob = { volume_slider.x - (slider_knob_width / 2) + (volume_slider.w * percentage), volume_slider.y - volume_slider.h / 2,
                                    slider_knob_width, slider_knob_height };
        SDL_RenderFillRect(m_window.renderer, &slider_knob);
    }

    // pause button
    {
        Rectangle button = m_ui.pause_button;

        SDL_SetRenderDrawColor(m_window.renderer, 0x66, 0x55, 0x55, 0xff);
        SDL_FRect pbutton = { button.x, button.y, button.w, button.h };
        SDL_RenderFillRect(m_window.renderer, &pbutton);

        float tex_w, tex_h;
        SDL_GetTextureSize(m_assets.pause_texture, &tex_w, &tex_h);
        SDL_FRect src = {0,0,tex_w,tex_h};
        SDL_FRect dst = pbutton;
        SDL_RenderTexture(m_window.renderer, m_assets.pause_texture, &src, &dst);
    }

    SDL_RenderTexture(m_window.renderer, m_assets.pause_texture, NULL, NULL);
}

bool Application::mouse_input_ui()
{
    if (m_ui.volume_slider.contains(m_mouse.pos))
    {
        float diff = m_mouse.pos.x - m_ui.volume_slider.x;
        m_audio.volume = diff / m_ui.volume_slider.w;
        return true;
    }

    if (m_ui.pause_button.contains(m_mouse.pos))
    {
        if (m_audio.paused)
        {
            m_audio.unpause();
            m_background_color = { 0x66, 0xaa, 0x33, 0xff };
        }
        else
        {
            m_audio.pause();
            m_background_color = { 0xaa, 0x66, 0x33, 0xff };
        }

        m_audio.paused = !m_audio.paused;
    }

    return false;
}
