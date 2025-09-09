#include "application.h"

#include <iostream>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>


bool Application::initialize()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        std::cerr << "Failed to init SDL\n";
        return false;
    }

    // window
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

    // ttf
    {
        if (!TTF_Init())
        {
            std::cerr << "Could not initialize TTF\n";
            return false;
        }
    }

    {
        if (!m_audio.initialize(48000, 1))
        {
            std::cerr << "Failed to initialize audio\n";
            return false;
        }
    }

    if (!load_assets())
    {
        std::cerr << "Could not load assets\n";
        return false;
    }

    m_quit = false;

    return true;
}

#define FONT_SIZE 100.0

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
        m_assets.pause_texture = IMG_LoadTexture(m_window.renderer, sb.c_string());
        if (!m_assets.pause_texture)
        {
            LOG_ERROR("Failed to load pause texture");
            return false;
        }
        sb.remove(pause_texture.size);
    }

    {
        String resume_texture = make_string("resume.png");
        sb.append(resume_texture);
        m_assets.resume_textrue = IMG_LoadTexture(m_window.renderer, sb.c_string());
        if (!m_assets.resume_textrue)
        {
            LOG_ERROR("Failed to load resume texture");
            return false;
        }
        sb.remove(resume_texture.size);
    }

    // font
    {
        const String font_folder = make_string("font");

        sb.append(font_folder);
        sb.append(path_seperator);

        sb.append(make_string("Roboto"));
        sb.append(path_seperator);

        sb.append(make_string("Roboto-VariableFont.ttf"));

        {
            TTF_Font* font = TTF_OpenFont(sb.c_string(), FONT_SIZE);
            if (!font) {
                std::cerr << "Could not load font " << sb.c_string() << "\n";
                std::cerr << SDL_GetError() << "\n";
                return false;
            }

            m_assets.font = { font, FONT_SIZE };
        }
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
            {
                this->m_quit = true;
                break;
            }
            case SDL_EVENT_KEY_DOWN:
            {
                SDL_KeyboardEvent keyboard = e.key;
                switch (keyboard.scancode)
                {
                    case SDL_SCANCODE_ESCAPE:
                    {
                        this->m_quit = true;
                        break;
                    }
                    case SDL_SCANCODE_RETURN:
                    {
                        text_input_stop();
                        break;
                    }
                    default:
                    {
                        break;
                    }
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
            case SDL_EVENT_WINDOW_RESIZED:
            {
                m_ui.update(m_window);
                break;
            }
            case SDL_EVENT_TEXT_INPUT:
            {
                SDL_TextInputEvent text = e.text;
                String input_text = make_string(text.text);

                update_input_string(input_text);
                break;
            }
            case SDL_EVENT_TEXT_EDITING:
            {
                SDL_TextEditingEvent edit = e.edit;
                break;
            }
            default:
            {
                break;
            }
        }
    }
}


#define NS_PER_SECONDS 1'000'000'000

void Application::update()
{
    SDL_Time time = SDL_GetTicksNS();
    m_audio.m_time = (double)time / NS_PER_SECONDS;
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

        float percentage = m_audio.get_volume();
        SDL_SetRenderDrawColor(m_window.renderer, 0x66, 0x55, 0x22, 0xff);
        SDL_FRect slider_knob = { volume_slider.x - (slider_knob_width / 2) + (volume_slider.w * percentage), volume_slider.y - volume_slider.h / 2,
                                    slider_knob_width, slider_knob_height };
        SDL_RenderFillRect(m_window.renderer, &slider_knob);
    }

    // pause/resume button
    {
        Rectangle button = m_ui.pause_button;

        SDL_SetRenderDrawColor(m_window.renderer, 0x66, 0x55, 0x55, 0xff);
        SDL_FRect pbutton = { button.x, button.y, button.w, button.h };
        SDL_RenderFillRect(m_window.renderer, &pbutton);

        float tex_w, tex_h;
        SDL_Texture* texture = m_audio.paused ? m_assets.resume_textrue : m_assets.pause_texture;
        SDL_GetTextureSize(texture, &tex_w, &tex_h);
        SDL_FRect src = {0,0,tex_w,tex_h};
        SDL_FRect dst = pbutton;
        SDL_RenderTexture(m_window.renderer, texture, &src, &dst);
    }

    // text field
    {
        SDL_SetRenderDrawColor(m_window.renderer, 0x66, 0x55, 0x55, 0xff);
        Rectangle text_field_area = m_ui.text_field.area;
        SDL_FRect tf_area = { text_field_area.x, text_field_area.y, text_field_area.w, text_field_area.h };
        SDL_RenderFillRect(m_window.renderer, &tf_area);


        SDL_Texture* text_texture = m_ui.text_field.m_texture;
        if (text_texture)
        {
            int line_count = m_ui.text_field.line_count;
            float font_size = m_assets.font.size;

            SDL_FRect string_area = { tf_area.x, tf_area.y, tf_area.w, line_count * font_size };
            SDL_FRect texture_area = { 0, 0, string_area.w, string_area.h };
            SDL_RenderTexture(m_window.renderer, text_texture, &texture_area, &string_area);
        }
    }
}

bool Application::mouse_input_ui()
{
    if (m_ui.volume_slider.contains(m_mouse.pos))
    {
        float diff = m_mouse.pos.x - m_ui.volume_slider.x;
        float volume = diff / m_ui.volume_slider.w;
        m_audio.set_volume(volume);
        return true;
    }

    if (m_ui.pause_button.contains(m_mouse.pos))
    {
        if (m_audio.paused)
        {
            m_audio.unpause();
        }
        else
        {
            m_audio.pause();
        }

        return true;
    }

    if (m_ui.text_field.area.contains(m_mouse.pos))
    {
        const SDL_Rect area = {m_ui.text_field.area.x, m_ui.text_field.area.y, m_ui.text_field.area.w, m_ui.text_field.area.h};
        SDL_SetTextInputArea(m_window.window, &area, m_ui.text_field.cursor_character);

        if (!doing_text_input)
        {
            text_input_start();
        }
        else
        {
            text_input_stop();
        }

        return true;
    }

    return false;
}

void Application::text_input_start()
{
    vec2 relative_mouse_pos = { m_mouse.pos.x - m_ui.text_field.area.x, m_mouse.pos.y - m_ui.text_field.area.y };

    m_background_color = Color{ 0, 0, 0x22, 0xff };

    SDL_StartTextInput(m_window.window);

    int line_count = (int)(relative_mouse_pos.y / FONT_SIZE);
    String tf_string = m_ui.text_field.get_string();

    size_t character_offset = 0;

    size_t width_characters = 0;
    int width_pixels = 0;
    for (int i = 0; i < line_count - 1; i++)
    {
        TTF_MeasureString(m_assets.font.font, tf_string.data + character_offset, tf_string.size - character_offset, m_ui.text_field.area.w, &width_pixels, &width_characters);
        character_offset += width_characters;
    }

    TTF_MeasureString(m_assets.font.font, tf_string.data + character_offset, tf_string.size - character_offset, m_ui.text_field.area.w - relative_mouse_pos.x, &width_pixels, &width_characters);
    character_offset += width_characters;

    m_ui.text_field.cursor_character = character_offset;
    m_ui.text_field.cursor_pixel = width_pixels;

    doing_text_input = true;
}

void Application::text_input_stop()
{
    SDL_StopTextInput(m_window.window);

    m_background_color = DEFAULT_BACKGROUND_COLOR;

    doing_text_input = false;
}

void UiState::update(Window window)
{
    ivec2 window_size;
    SDL_GetWindowSize(window.window, &window_size.x, &window_size.y);

    pause_button.x = (window_size.x - pause_button.w) / 2;
    pause_button.y = (window_size.y - pause_button.h) / 2;

    text_field.area.x = (window_size.x - text_field.area.w) / 2;
    text_field.area.y = ((float)window_size.y * (4.0 / 5.0)) - text_field.area.h/2;
}

bool Application::update_input_string(String s)
{
    m_ui.text_field.text.append(s);

    // re-render and update the texture
    Rectangle text_field_area = m_ui.text_field.area;
    Font font = m_assets.font;

    const float font_size = font.size;
    const int line_count = text_field_area.h / font_size;

    String string = m_ui.text_field.get_string();

    int measure_pixels = 0;
    size_t measure_characters = 0;
    TTF_MeasureString(font.font, string.data, string.size, text_field_area.w, &measure_pixels, &measure_characters);

    const SDL_Color text_color = { 0x11, 0x22, 0x11, 0xff };
    SDL_Surface* text_surface = TTF_RenderText_Solid_Wrapped(m_assets.font.font, string.data, string.size, text_color, text_field_area.w);
    SDL_Texture* text = SDL_CreateTextureFromSurface(m_window.renderer, text_surface);
    SDL_DestroySurface(text_surface);

    if (text)
    {
        m_ui.text_field.m_texture = text;
        m_ui.text_field.line_count = line_count;
    }

    return (text) ? true : false;
}
