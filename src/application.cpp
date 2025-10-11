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

    {
        get_default_builtin_functions(m_evaluator.builtin_functions);
        m_evaluator.set(48000, 0.0);
    }

    // info string
    {
        auto texts = new Text[TEXT_COUNT];
        m_rendered_text_cache = Array<Text>(texts, TEXT_COUNT);

        gen_static_text(Color{0x44, 0x22, 0x33, 0xff});
    }

    quit = false;

    return true;
}

Text Application::create_text(String text, Color color)
{
    SDL_Color sdl_color = { color.r, color.g, color.b, color.a };
    SDL_Surface* surface = TTF_RenderText_Solid(m_assets.font.font, text.data, text.size, sdl_color);

    if (!surface)
        return Text();

    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_window.renderer, surface);

    if (!texture)
    {
        SDL_DestroySurface(surface);
        return Text();
    }

    return Text(texture, text);
}

void Application::render_text_at(Text text, vec2 center)
{
    float tex_w, tex_h;
    SDL_GetTextureSize(text.texture, &tex_w, &tex_h);

    SDL_FRect src = { 0,0,tex_w,tex_h };
    SDL_FRect dst = { center.x - tex_w / 2, center.y - tex_h / 2, tex_w, tex_h };

    SDL_RenderTexture(m_window.renderer, text.texture, &src, &dst);
}

bool Application::gen_static_text(Color color)
{
    Text paused = create_text(make_string("Paused"), color);
    Text playing = create_text(make_string("Playing"), color);
    Text invalid_expression = create_text(make_string("Invalid Expression"), color);
    Text valid_expression = create_text(make_string("Valid Expression"), color);

    if (!paused.texture ||
        !playing.texture ||
        !invalid_expression.texture)
    {
        return false;
    }

    m_rendered_text_cache.data[TEXT_TEXT_PAUSED] = paused;
    m_rendered_text_cache.data[TEXT_TEXT_PLAYING] = playing;
    m_rendered_text_cache.data[TEXT_INVALID_EXPRESSION] = invalid_expression;
    m_rendered_text_cache.data[TEXT_VALID_EXPRESSION] = valid_expression;

    return true;
}

bool Application::gen_text(Color color)
{
    char buffer[50];
    int writen = snprintf(buffer, sizeof(buffer), "%.4f", m_audio.get_volume());

    Text volume = create_text(make_string(buffer), color);

    if (!volume.texture)
    {
        return false;
    }

    m_rendered_text_cache.data[TEXT_VOLUME_VALUE] = volume;

    return true;
}

void Application::update_audio_spec()
{
    bool conversion_success = false;
    auto sample_rate = string_to_integer(m_ui.m_sample_rate_box.get_string(), &conversion_success);
    if (conversion_success)
    {
        m_audio.reinitialize(sample_rate, m_audio.m_channel_count);
    }
    else
    {
        m_error_log.add(make_string("Unable to parse sample rate"));
    }
}

void Application::set_volume(float volume)
{
    m_audio.set_volume(volume);

    gen_text(Color{0x22, 0x55, 0x65, 0xff});
}

#define FONT_SIZE 100.0

bool Application::load_assets()
{
    // @todo cleanup

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
        m_assets.resume_texture = IMG_LoadTexture(m_window.renderer, sb.c_string());
        if (!m_assets.resume_texture)
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
                quit = true;
                break;
            }
            case SDL_EVENT_KEY_DOWN:
            {
                SDL_KeyboardEvent keyboard = e.key;
                switch (keyboard.scancode)
                {
                    case SDL_SCANCODE_ESCAPE:
                    {
                        quit = true;
                        break;
                    }
                    case SDL_SCANCODE_RETURN:
                    {
                        bool set = set_eval_string(m_ui.m_text_field.get_string());
                        if (!set)
                        {
                            printf("Could not set sample expression\n");
                        }
                        text_input_stop();
                        break;
                    }
                    case SDL_SCANCODE_BACKSPACE:
                    {
                        if (doing_text_input)
                        {
                            auto text_field = m_ui.get_selected_text_field();
                            if (text_field)
                            {
                                text_field->delete_last();
                                text_field->render_text_field_texture(m_window.renderer, text_field->get_string(), m_assets.font, false);
                            }
                        }
                        break;
                    }
                    case SDL_SCANCODE_SPACE:
                    {
                        if (!doing_text_input)
                        {
                            m_audio.toggle_pause();
                        }
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

                auto text_field = m_ui.get_selected_text_field();
                if (text_field)
                {
                    text_field->m_text.append(input_text);
                    update_input_string();
                }
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
    double time_sec = (double)time / NS_PER_SECONDS;
    m_audio.m_time = time_sec;

    m_evaluator.update(time_sec);
}

void Application::cleanup()
{
    m_audio.cleanup();
    SDL_Quit();
}

void Application::draw()
{
    SDL_Renderer* renderer = m_window.renderer;

    SDL_SetRenderDrawColor(renderer, COLOR_ARG(m_background_color));
    SDL_RenderClear(renderer);

    draw_ui();

    SDL_RenderPresent(renderer);
}

void Application::draw_ui()
{
    // volume slider
    {
        Rectangle volume_slider = m_ui.m_volume_slider;
        vec2 knob_scale = m_ui.m_volume_slider_knob_scale;

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

        // volume text
        {
            Text text = m_rendered_text_cache.data[TEXT_VOLUME_VALUE];

            int measured_width = 0;
            TTF_MeasureString(m_assets.font.font, text.string.data, text.string.size, 0, &measured_width, NULL);

            float tex_w, tex_h;
            SDL_GetTextureSize(text.texture, &tex_w, &tex_h);

            SDL_FRect src = { 0, 0, tex_w, tex_h };
            int margin = 10;
            SDL_FRect dst = { volume_slider.x - tex_w / 2, volume_slider.y + volume_slider.h + margin, tex_w, tex_h};

            SDL_RenderTexture(m_window.renderer, text.texture, &src, &dst);
        }
    }

    // pause/resume button
    {
        Rectangle button = m_ui.m_pause_button;

        SDL_SetRenderDrawColor(m_window.renderer, 0x66, 0x55, 0x55, 0xff);
        SDL_FRect pbutton = { button.x, button.y, button.w, button.h };
        SDL_RenderFillRect(m_window.renderer, &pbutton);

        float tex_w, tex_h;
        SDL_Texture* texture = m_audio.paused ? m_assets.resume_texture : m_assets.pause_texture;
        SDL_GetTextureSize(texture, &tex_w, &tex_h);
        SDL_FRect src = {0,0,tex_w,tex_h};
        SDL_FRect dst = pbutton;
        SDL_RenderTexture(m_window.renderer, texture, &src, &dst);
    }

    // paused/playing text
    {
        Text text = m_rendered_text_cache.data[(m_audio.paused) ? TEXT_TEXT_PAUSED : TEXT_TEXT_PLAYING];

        Rectangle pause_button = m_ui.m_pause_button;

        int measured_width = 0;
        TTF_MeasureString(m_assets.font.font, text.string.data, text.string.size, 0, &measured_width, nullptr);

        float tex_w, tex_h;
        SDL_GetTextureSize(text.texture, &tex_w, &tex_h);

        SDL_FRect src = { 0,0,tex_w,tex_h };
        const int margin = 5;
        SDL_FRect dst = {pause_button.x - measured_width/2, pause_button.y + pause_button.h + margin, tex_w, tex_h};

        SDL_RenderTexture(m_window.renderer, text.texture, &src, &dst);
    }

    // text field
    {
        SDL_SetRenderDrawColor(m_window.renderer, 0x66, 0x55, 0x55, 0xff);
        Rectangle text_field_area = m_ui.m_text_field.m_area;
        SDL_FRect tf_area = { text_field_area.x, text_field_area.y, text_field_area.w, text_field_area.h };
        SDL_RenderFillRect(m_window.renderer, &tf_area);

        SDL_Texture* text_texture = m_ui.m_text_field.m_texture;
        if (text_texture)
        {
            int line_count = m_ui.m_text_field.m_line_count;
            float font_size = m_assets.font.size;

            SDL_FRect string_area = { tf_area.x, tf_area.y, tf_area.w, line_count * font_size };
            SDL_FRect texture_area = { 0, 0, string_area.w, string_area.h };
            SDL_RenderTexture(m_window.renderer, text_texture, &texture_area, &string_area);
        }
    }

    // sample rate text box
    {
        SDL_SetRenderDrawColor(m_window.renderer, 0xaa, 0x22, 0x11, 0xff);
        Rectangle box_area = m_ui.m_sample_rate_box.m_area;
        SDL_FRect area = {box_area.x, box_area.y, box_area.w, box_area.h};
        SDL_RenderFillRect(m_window.renderer, &area);

        SDL_Texture* text_texture = m_ui.m_sample_rate_box.m_texture;
        if (text_texture)
        {
            const int line_count = 1;
            float font_size = m_assets.font.size;

            SDL_FRect string_area = {area.x, area.y, area.w, font_size * line_count};
            SDL_FRect texture_area = {0, 0, string_area.w, string_area.h};
            SDL_RenderTexture(m_window.renderer, text_texture, &texture_area, &string_area);
        }
    }
}

bool Application::mouse_input_ui()
{
    
    
    if (m_ui.m_volume_slider.contains(m_mouse.pos))
    {
        float diff = m_mouse.pos.x - m_ui.m_volume_slider.x;
        float volume = diff / m_ui.m_volume_slider.w;

        volume = snap_value(volume, 0.0, 1.0, 0.09);

        set_volume(volume);

        printf("%f\n", m_audio.get_volume());
        return true;
    }

    if (m_ui.m_pause_button.contains(m_mouse.pos))
    {
        m_audio.toggle_pause();

        return true;
    }

    if (m_ui.m_text_field.m_area.contains(m_mouse.pos))
    {
        const SDL_Rect area = {m_ui.m_text_field.m_area.x, m_ui.m_text_field.m_area.y, m_ui.m_text_field.m_area.w, m_ui.m_text_field.m_area.h};
        SDL_SetTextInputArea(m_window.window, &area, m_ui.m_text_field.m_cursor_pixel);

        toggle_text_input();

        m_ui.m_text_input_target = TEXT_INPUT_TEXT_FIELD;

        return true;
    }

    if (m_ui.m_sample_rate_box.m_area.contains(m_mouse.pos))
    {
        const SDL_Rect area = {m_ui.m_sample_rate_box.m_area.x, m_ui.m_sample_rate_box.m_area.y, m_ui.m_sample_rate_box.m_area.w, m_ui.m_sample_rate_box.m_area.h};
        SDL_SetTextInputArea(m_window.window, &area, m_ui.m_sample_rate_box.m_cursor_pixel);

        toggle_text_input();

        m_ui.m_text_input_target = TEXT_INPUT_SAMPLE_RATE;

        return true;
    }

    return false;
}

void Application::text_input_stop()
{
    SDL_StopTextInput(m_window.window);
    doing_text_input = false;

    m_background_color = DEFAULT_BACKGROUND_COLOR;

    if (m_ui.m_text_input_target == TEXT_INPUT_SAMPLE_RATE)
    {
        update_audio_spec();
    }
}

void Application::text_input_start()
{
    SDL_StartTextInput(m_window.window);
    doing_text_input = true;

    m_background_color = {0, 0x22, 0x11, 0xff};
}

void Application::toggle_text_input()
{
    if (!doing_text_input)
    {
        text_input_start();
    }
    else
    {
        text_input_stop();
    }
}

void UiState::update(Window window)
{
    ivec2 window_size;
    SDL_GetWindowSize(window.window, &window_size.x, &window_size.y);

    m_pause_button.x = (window_size.x - m_pause_button.w) / 2;
    m_pause_button.y = (window_size.y - m_pause_button.h) / 2;

    m_text_field.m_area.x = (window_size.x - m_text_field.m_area.w) / 2;
    m_text_field.m_area.y = ((float)window_size.y * (4.0 / 5.0)) - m_text_field.m_area.h/2;

    m_sample_rate_box.m_area.x = window_size.x * (3.0 / 5.0);
    m_sample_rate_box.m_area.y = window_size.y * (1.0 / 5.0);
    m_sample_rate_box.m_area.w = window_size.x * (1.0 / 5.0);
    m_sample_rate_box.m_area.h = window_size.y * (1.0 / 5.0);
}

Text_Field* UiState::get_selected_text_field()
{
    return (m_text_input_target == TEXT_INPUT_TEXT_FIELD) ? &m_text_field : &m_sample_rate_box;
}

bool Text_Field::render_text_field_texture(SDL_Renderer* renderer, String s, Font font, bool wrapped)
{
    if (s.size == 0)
    {
        SDL_DestroyTexture(m_texture);
        m_texture = NULL;
        return true;
    }

    Rectangle area = m_area;

    int measure_pixels = 0;
    size_t measure_characters = 0;
    TTF_MeasureString(font.font, get_string().data, get_string().size, area.w, &measure_pixels, &measure_characters);

    const SDL_Color text_color = { 0x11, 0x22, 0x11, 0xff };
    SDL_Surface* text_surface;
    
    if (wrapped) {
        text_surface = TTF_RenderText_Solid_Wrapped(font.font, get_string().data, get_string().size, text_color, area.w);
        m_line_count = (int)(area.h / font.size);
    } else {
        text_surface = TTF_RenderText_Solid(font.font, get_string().data, get_string().size, text_color);
        m_line_count = 1;  // unwrapped text
    }

    SDL_Texture* text = SDL_CreateTextureFromSurface(renderer, text_surface);
    SDL_DestroySurface(text_surface);

    if (text)
    {
        SDL_DestroyTexture(m_texture);

        m_texture = text;
    }

    return (text) ? true : false;
}

bool Application::update_input_string()
{
    auto text_field = m_ui.get_selected_text_field();
    if (!text_field)
        return false;
    String s = text_field->get_string();
    return text_field->render_text_field_texture(m_window.renderer, s, m_assets.font,
                                               (m_ui.m_text_input_target == TEXT_INPUT_TEXT_FIELD));
}

bool Application::set_eval_string(String eval_string)
{
    Parser parser = {};

    auto parsed = parser.parse(m_ui.m_text_field.get_string());
    if (parsed.size == 0)
    {
        return false;
    }

    for (int i = 0; i < parsed.size; i++)
    {
        Expr* expr = parsed.get(i);
        print_expr(expr, 0);
        auto eval = m_evaluator.evaluate(expr);
        if (eval.success)
        {
            printf("%f\n", eval.value);
        }

        if (!eval.success)
        {
            return false;
        }
    }

    if (!m_audio.set_sample_expression(parsed.get(0)))
    {
        printf("Failed to set sample expression");
        return false;
    }

    return true;
}
