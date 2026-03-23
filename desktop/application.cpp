#include "application.h"

#include <iostream>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

static const char* org_name = "flying-carpet";
static const char* soundtoy_identifier = "flying-carpet.soundtoy";
static const char* soundtoy_name = "soundtoy";
static const char* soundtoy_version = "0.1.0";

#define WAVEFORM_SAMPLE_RATE 256
#define DEFAULT_EXPRESSION "sin(t*tau*440)"

bool Application::initialize()
{
    SDL_SetAppMetadata(soundtoy_name, soundtoy_version, soundtoy_identifier);

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        fprintf(stderr, "Failed to init SDL\n");
        return false;
    }

    // window
    {
        float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
        if (!SDL_CreateWindowAndRenderer("soundtoy", 1440, 810, flags, &window, &renderer)) {
            fprintf(stderr, "Failed to create window and renderer\n");
            return false;
        }

        m_window = { window, renderer };
    }

    // accept drop events
    SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, true);

    // ttf
    {
        if (!TTF_Init())
        {
            fprintf(stderr, "Could not initialize TTF\n");
            return false;
        }
    }

    if (!load_assets()) {
        fprintf(stderr, "Could not load assets\n");
        return false;
    }

    const int initial_sample_rate = DESIRED_AUDIO_SAMPLE_RATE;

    {
        const int waveform_sample_rate = WAVEFORM_SAMPLE_RATE;

        St_Sampler* audio_left = st_sampler_create(initial_sample_rate);
        St_Sampler* audio_right = st_sampler_create(initial_sample_rate);

        St_Sampler* waveform_left = st_sampler_create(waveform_sample_rate);
        St_Sampler* waveform_right = st_sampler_create(waveform_sample_rate);

        if (!(audio_left && audio_right && waveform_left && waveform_right)) {
            fprintf(stderr, "Could not create a samplers\n");
            return false;
        }

        String expression_default = make_string(DEFAULT_EXPRESSION);
        ASSERT(st_sampler_set_expression(audio_left, expression_default.data, expression_default.size));
        ASSERT(st_sampler_set_expression(audio_right, expression_default.data, expression_default.size));
        ASSERT(st_sampler_set_expression(waveform_left, expression_default.data, expression_default.size));
        ASSERT(st_sampler_set_expression(waveform_right, expression_default.data, expression_default.size));

        m_samplers.audio_left = audio_left;
        m_samplers.audio_right = audio_right;

        m_samplers.waveform_left = waveform_left;
        m_samplers.waveform_right = waveform_right;

        const int sample_buffer_size_audio = 512;
        float* sample_buffer_mem = new float[sample_buffer_size_audio];
        m_buffers.audio_samples = Array<float>(sample_buffer_mem, sample_buffer_size_audio);

        const int sample_buffer_size_waveform = 128;
        SDL_FPoint* waveform_sample_buffer_left_mem = new SDL_FPoint[sample_buffer_size_waveform];
        SDL_FPoint* waveform_sample_buffer_right_mem = new SDL_FPoint[sample_buffer_size_waveform];
        m_buffers.waveform_samples_left = Array<SDL_FPoint>(waveform_sample_buffer_left_mem, sample_buffer_size_waveform);
        m_buffers.waveform_samples_right = Array<SDL_FPoint>(waveform_sample_buffer_right_mem, sample_buffer_size_waveform);
    }

    if (!m_audio.expr_audio.initialize(m_buffers.audio_samples, initial_sample_rate, 1, m_samplers.audio_left, m_samplers.audio_right)) {
        fprintf(stderr, "Failed to initialize expression audio\n");
        return false;
    }

    if (!m_audio.audio_player.initialize(DESIRED_AUDIO_SAMPLE_RATE, 2, 0.5)) {
        fprintf(stderr, "Failed to initialize audio player: %s\n", SDL_GetError());
        return false;
    }

    // info string
    {
        auto texts = new Text[TEXT_COUNT];
        m_rendered_text = Array<Text>(texts, TEXT_COUNT);

        gen_static_text(Color{0x44, 0x22, 0x33, 0xff});

        // dynamic text
        m_rendered_text.data[TEXT_VOLUME_VALUE] = create_text(make_string("0.0"), m_assets.font_large, Color(0x54, 0x22, 0x77, 0xff));
    }

    // ui
    {
        ivec2 ws;
        SDL_GetWindowSize(m_window.window, &ws.x, &ws.y);

        update_ui_state(vec2(ws.x, ws.y));

        // @todo show what is selected in the ui
        Text mono = create_text(make_string("mono"), m_assets.font_large, Color(0x44, 0x77, 0x22, 0xff));
        Text stereo = create_text(make_string("stereo"), m_assets.font_large, Color(0x44, 0x77, 0x22, 0xff));
        m_ui.channel_count.set_title(create_text(make_string("Channel Count"), m_assets.font_large, Color(0x44, 0x77, 0x22, 0xff)));
        m_ui.channel_count.add_option(mono, nullptr);
        m_ui.channel_count.add_option(stereo, nullptr);

        m_ui.playback_device.set_title(create_text(make_string("Audio Device"), m_assets.font_large, Color(0x44, 0x88, 0x22, 0xff)));

        int count = 0;
        SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&count);

        printf("Playback devices: \n");
        for (int i = 0; i < count; i++) {
            const char* device_name = SDL_GetAudioDeviceName(devices[i]);
            // this is done in the event handler as existing devices at initialization time are provided as device_added events by SDL
            // m_ui.playback_device.add_option(create_text(make_string(device_name), Color(0x88, 0x33, 0x11, 0xff)));

            printf("%s\n", device_name);
        }

        m_ui.graph_to_show.set_title(create_text(make_string("Graph"), m_assets.font_large, Color(0x66, 0x33, 0x55, 0xff)));
        m_ui.graph_to_show.add_option(create_text(make_string("Audio Data"), m_assets.font_large, Color(0x66, 0x44, 0x66, 0xff)), 0);
        m_ui.graph_to_show.add_option(create_text(make_string("Fourier"), m_assets.font_large, Color(0x66, 0x44, 0x66, 0xff)), 0);
    }

    quit = false;

    return true;
}

Text Application::create_text(String text, Font font, Color color)
{
    SDL_Color sdl_color = { color.r, color.g, color.b, color.a };
    SDL_Surface* surface = TTF_RenderText_Solid(font.font, text.data, text.size, sdl_color);

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

void Application::destroy_text(Text& text)
{
    SDL_DestroyTexture(text.texture);
    text.texture = nullptr;
    text.string = {};
}

bool Application::gen_static_text(Color color)
{
    Text paused = create_text(make_string("Paused"), m_assets.font_large, color);
    Text playing = create_text(make_string("Playing"), m_assets.font_large, color);
    Text pause = create_text(make_string("pause"), m_assets.font_medium, color);
    Text resume = create_text(make_string("resume"), m_assets.font_medium, color);
    Text invalid_expression = create_text(make_string("Invalid Expression"), m_assets.font_large, color);
    Text invalid_sample_rate = create_text(make_string("Invalid Sample Rate"), m_assets.font_large, color);
    Text valid_expression = create_text(make_string("Valid Expression"), m_assets.font_large, color);
    Text sample_rate = create_text(make_string("Sample Rate"), m_assets.font_large, color);
    Text text_sound_mode = create_text(make_string("sound mode"), m_assets.font_large, color);
    Text text_graph_mode = create_text(make_string("graph mode"), m_assets.font_large, color);

    if (!
        (paused.texture &&
         playing.texture &&
         pause.texture &&
         resume.texture &&
         invalid_expression.texture &&
         invalid_sample_rate.texture &&
         valid_expression.texture &&
         sample_rate.texture &&
         text_sound_mode.texture &&
         text_graph_mode.texture
         )
        )
    {
        return false;
    }

    m_rendered_text.data[TEXT_PAUSED] = paused;
    m_rendered_text.data[TEXT_PLAYING] = playing;
    m_rendered_text.data[TEXT_PAUSE] = pause;
    m_rendered_text.data[TEXT_RESUME] = resume;
    m_rendered_text.data[TEXT_SAMPLE_RATE] = sample_rate;
    m_rendered_text.data[TEXT_INVALID_EXPRESSION] = invalid_expression;
    m_rendered_text.data[TEXT_INVALID_SAMPLE_RATE] = invalid_sample_rate;
    m_rendered_text.data[TEXT_VALID_EXPRESSION] = valid_expression;
    m_rendered_text.data[TEXT_SOUND_MODE] = text_sound_mode;
    m_rendered_text.data[TEXT_GRAPH_MODE] = text_graph_mode;

    return true;
}

bool Application::update_channel_count(int channels)
{
    bool success = m_audio.expr_audio.reinitialize(m_audio.expr_audio.get_sample_rate(), channels);

    int w_x, w_y;
    SDL_GetWindowSize(m_window.window, &w_x, &w_y);
    update_ui_state(vec2(w_x, w_y));  // so that we get 2 expression inputs and their positions calculated properly
    return success;
}

#define FONT_SIZE_SMALL   18.0
#define FONT_SIZE_MEDIUM  32.0
#define FONT_SIZE_LARGE   72.0
#define FONT_SIZE_EDITOR  28.0

#ifdef _WIN32
    String path_separator = make_string("\\");
#else
    String path_separator = make_string("/");
#endif

bool Application::load_assets()
{
    String_Builder sb(256);
    const char* base_path = SDL_GetBasePath();

    sb.append(make_string(base_path));

    bool load_from_base_path = st_load_assets(sb);
    if (!load_from_base_path) {
        // success
        return false;
    }

    return true;
}

bool Application::st_load_assets(String_Builder& sb) {
    printf("Searching for assets in %s\n", sb.c_string());

    sb.append(make_string("asset"));
    sb.append(path_separator);

    // textures
    {
        String pause_texture = make_string("pause.png");
        sb.append(pause_texture);
        m_assets.pause_texture = IMG_LoadTexture(m_window.renderer, sb.c_string());
        if (!m_assets.pause_texture)
        {
            fprintf(stderr, "Failed to load pause texture\n");
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
            fprintf(stderr, "Failed to load resume texture\n");
            return false;
        }
        sb.remove(resume_texture.size);
    }

    // font

    {
        String folder = make_string("font");
        sb.append(folder);
        sb.append(path_separator);

        bool small_font = load_font(&m_assets.font_small, sb, make_string("Fira_Sans"), make_string("FiraSans-Regular.ttf"), FONT_SIZE_SMALL);
        bool medium_font = load_font(&m_assets.font_medium, sb, make_string("Fira_Sans"), make_string("FiraSans-Regular.ttf"), FONT_SIZE_MEDIUM);
        bool large_font = load_font(&m_assets.font_large, sb, make_string("Fira_Sans"), make_string("FiraSans-Regular.ttf"), FONT_SIZE_LARGE);
        bool editor_font = load_font(&m_assets.font_editor, sb, make_string("Fira_Code"), make_string("FiraCode-Regular.ttf"), FONT_SIZE_EDITOR);

        sb.remove(folder.size + 1);

        if (!(small_font && medium_font && large_font))
        {
            fprintf(stderr, "Could not load fonts\n");
            return false;
        }
    }

    return true;
}

bool load_font(Font* font, String_Builder& path, String font_folder, String font_file, float size)
{
    path.append(font_folder);
    path.append(path_separator);

    path.append(font_file);

    TTF_Font* ttf_font = TTF_OpenFont(path.c_string(), size);

    path.remove(font_folder.size + 1 + font_file.size);

    if (!ttf_font) {
        SCOPE_STRING(font_file, font_name);
        fprintf(stderr, "Could not load font %s\n", font_name);
        fprintf(stderr, "%s\n", SDL_GetError());
        return false;
    }

    font->font = ttf_font;
    font->size = size;

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

                bool consumed = keyboard_input_common(keyboard);
                if (!consumed)
                {
                    if (mode == AppModeSound)
                    {
                        keyboard_input_sound_mode(keyboard);
                    }
                    else if (mode == AppModeGraph)
                    {
                        keyboard_input_graph_mode(keyboard);
                    }
                }

                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
                SDL_MouseButtonEvent mouse = e.button;
                if (mouse_input())
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
                ivec2 ws;
                SDL_GetWindowSize(m_window.window, &ws.x, &ws.y);

                update_ui_state(vec2(ws.x, ws.y));
                break;
            }
            case SDL_EVENT_TEXT_INPUT:
            {
                SDL_TextInputEvent text = e.text;
                String input_text = make_string(text.text);

                auto text_field = m_ui.get_selected_text_field();
                if (text_field)
                {
                    text_field->append_string(input_text);
                    update_input_string();
                }
                break;
            }
            case SDL_EVENT_AUDIO_DEVICE_ADDED: {
                // get added playback devices
                SDL_AudioDeviceEvent device_event = e.adevice;
                if (device_event.recording) {
                    break;
                }

                SDL_AudioDeviceID device = device_event.which;

                m_ui.playback_device.add_option(create_text(make_string(SDL_GetAudioDeviceName(device)),
                                                m_assets.font_medium,
                                                Color(0x44, 0x77, 0x22, 0xff)),
                                                device);
                break;
            }
            case SDL_EVENT_AUDIO_DEVICE_REMOVED: {
                // get removed playback devices
                SDL_AudioDeviceEvent device_event = e.adevice;
                if (device_event.recording) {
                    break;
                }

                SDL_AudioDeviceID device = device_event.which;
                String device_name = make_string(SDL_GetAudioDeviceName(device));

                for (int i = 0; i < m_ui.playback_device.options.size(); i++) {
                    String device_text = m_ui.playback_device.get_option_name(i);
                    if (string_compare(device_text, device_name)) {
                        m_ui.playback_device.remove_option(i);
                        break;
                    }
                }
                break;
            }
            case SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED: {
                // @todo maybe we need to reinitialize mixers?
                break;
            }

            case SDL_EVENT_DROP_FILE: {
                SDL_DropEvent drop = e.drop;

                if (drop.data) {
                    m_audio.audio_data.reset();

                    if (!m_audio.audio_data.load_audio_file(String(drop.data))) {
                        fprintf(stderr, "Could not load file %s\n", drop.data);
                    }

                    m_audio.audio_player.set_audio_data(&m_audio.audio_data);

                    m_audio.expr_audio.pause();

                    st_set_input_stream(m_samplers.audio_left,  ((float*) m_audio.audio_data.samples) + 0, m_audio.audio_data.frame_count, m_audio.audio_data.channel_count);
                    st_set_input_stream(m_samplers.audio_right, ((float*) m_audio.audio_data.samples) + 1, m_audio.audio_data.frame_count, m_audio.audio_data.channel_count);

                    st_set_input_stream(m_samplers.waveform_left,  ((float*) m_audio.audio_data.samples) + 0, m_audio.audio_data.frame_count, m_audio.audio_data.channel_count);
                    st_set_input_stream(m_samplers.waveform_right, ((float*) m_audio.audio_data.samples) + 1, m_audio.audio_data.frame_count, m_audio.audio_data.channel_count);
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


void Application::update()
{
    // update time
    SDL_Time time = SDL_GetTicksNS();
    double time_sec = (double)time / NS_PER_SECONDS;
    m_time = time;
    m_time_seconds = time_sec;

    timeout();

    if (mode == AppModeGraph)
    {
        m_audio.audio_player.put_audio_data();
    }
}

void Application::timeout()
{
    for (int i = 0; i < ARRAY_SIZE(m_events); i++)
    {
        if (m_events[i].active)
        {
            if (m_events[i].event < m_time)
            {
                m_events[i].active = false;
            }
        }
    }
}

void Application::set_event_active(int event_index, double timeout_seconds)
{
    s64 timeout = (s64)(timeout_seconds * NS_PER_SECONDS);
    m_events[event_index].active = true;
    m_events[event_index].event = m_time + timeout;
}

void Application::set_event_deactive(int event_index)
{
    m_events[event_index].active = false;
}

void Application::cleanup()
{
    st_sampler_destroy(m_samplers.audio_left);
    st_sampler_destroy(m_samplers.audio_right);
    st_sampler_destroy(m_samplers.waveform_left);
    st_sampler_destroy(m_samplers.waveform_right);
    m_audio.expr_audio.cleanup();

    SDL_Quit();
}

void Application::draw()
{
    SDL_Renderer* renderer = m_window.renderer;

    if (SDL_GetWindowFlags(m_window.window) & SDL_WINDOW_MINIMIZED) {
        // don't draw anything if the window is minimized
        return;
    }

    SDL_SetRenderDrawColor(renderer, COLOR_ARG(m_background_color));
    SDL_RenderClear(renderer);

    draw_common_ui();

    if (mode == AppModeSound) {
        draw_sound_mode_ui();
    }
    else if (mode == AppModeGraph) {
        draw_graph_mode_ui();
    }

    SDL_RenderPresent(renderer);
}

const float Variable_Table_Horizontal_Element_Size = (1.0 / 12.0);
const float Variable_Table_Vertical_Element_Size = (1.0 / 16.0);

void Application::draw_sound_mode_ui()
{
    int window_x, window_y;
    SDL_GetWindowSize(m_window.window, &window_x, &window_y);
    int waveform_sample_count = m_buffers.waveform_samples_left.size / 2;


    vec2 window_size = vec2((float) window_x, (float) window_y);

    // waveform visualization
    {
        SDL_SetRenderDrawColor(m_window.renderer, 0x22, 0xAA, 0x11, 0xff);
        SDL_FRect area_left = {window_size.x * (float)(1.0 / 20.0), window_size.y * (float)(1.0 / 2.0), window_size.x * (float)(1.0 / 3.0), window_size.y * (float)(1.0 / 5.0)};
        SDL_FRect area_right = {window_size.x - (area_left.x + area_left.w), area_left.y, area_left.w, area_left.h};

        SDL_RenderFillRect(m_window.renderer, &area_left);
        SDL_RenderFillRect(m_window.renderer, &area_right);

        if (m_events[EVENT_RECALCULATE_WAVEFORM_SAMPLES].active == false)
        {
            update_waveform(m_samplers.waveform_left, m_buffers.waveform_samples_left,
                            vec2(area_left.x + area_left.w / 2, area_left.y + area_left.h / 2), vec2(area_left.w, area_left.h));
            update_waveform(m_samplers.waveform_right, m_buffers.waveform_samples_right,
                            vec2(area_right.x + area_right.w / 2, area_right.y + area_right.h / 2), vec2(area_right.w, area_right.h));

            // recalculate regularly
            set_event_active(EVENT_RECALCULATE_WAVEFORM_SAMPLES, 1.0);
        }

        const Color waveform_color = Color(0xBB, 0x44, 0x77, 0xff);

        // @todo if we are going to only recalculate samples a couple of times a second
        // maybe do some interpolation or something so that it looks better?

        SDL_SetRenderDrawColor(m_window.renderer, COLOR_ARG(waveform_color));

        SDL_RenderLines(m_window.renderer, m_buffers.waveform_samples_left.data, m_buffers.waveform_samples_left.size);
        SDL_RenderLines(m_window.renderer, m_buffers.waveform_samples_right.data, m_buffers.waveform_samples_right.size);
    }


    // volume slider
    {
        vec2 knob_scale = { 0.1, 2 };
        Color slider_color = Color(0x55, 0x44, 0x22, 0xff);
        Color knob_color = Color(0x66, 0x55, 0x22, 0xff);

        render_slider(m_ui.volume_slider, knob_scale, m_audio.expr_audio.get_volume(), slider_color, knob_color, m_rendered_text.data[TEXT_VOLUME_VALUE]);
    }

    // pause/resume button
    {
        render_textured_rectangle(m_ui.pause_button,
            m_audio.expr_audio.paused ? m_assets.resume_texture : m_assets.pause_texture,
            Color(0x66, 0x55, 0x55, 0xff)
        );
    }

    // variables
    {
        vec2 pos = vec2(m_ui.add_variable_button.x,m_ui.add_variable_button.y);
        vec2 scale = vec2(m_ui.add_variable_button.w,m_ui.add_variable_button.h);
        SDL_FRect area = { pos.x - scale.x / 2, pos.y - scale.y / 2, scale.x, scale.y };
        SDL_SetRenderDrawColor(m_window.renderer, 0x66, 0x44, 0x66, 0xff);
        SDL_RenderFillRect(m_window.renderer, &area);
        draw_plus(m_window.renderer, pos, scale, 20, ColorF(0.5,0.4,0.6,1.0));

        render_text_field(m_ui.variable_name);

        // variables
        {
            // @todo make this a scrollable area if there are too many items
            Rectangle start = { window_size.x * float(1.0 / 8.0), window_size.y * (0),
                                window_size.x * Variable_Table_Horizontal_Element_Size, window_size.y * Variable_Table_Vertical_Element_Size };
            int index = 0;
            for (const auto& var : m_variable_text)
            {
                SDL_SetRenderDrawColor(m_window.renderer, 0x11, 0x77, 0x44, 0xff);
                SDL_FRect area = {start.x, start.y + index * start.h, start.w, start.h};
                SDL_RenderFillRect(m_window.renderer, &area);
                render_text_size(m_window.renderer, var, vec2(start.x + start.w / 2, start.y + index * start.h + start.h / 2), vec2(start.w, start.h));

                render_text_field(m_ui.variable_values.get_ref(index));

                index += 1;
            }
        }
    }

    // paused/playing text
    {
        Rectangle pause_button = m_ui.pause_button;
        const int margin = 5;
        Font font = m_assets.font_large;
        render_text_size(m_window.renderer,
            m_rendered_text.data[(m_audio.expr_audio.paused) ? TEXT_PAUSED : TEXT_PLAYING],
            vec2(pause_button.x, pause_button.y + pause_button.h + font.size/2));
    }

    // text field
    {
        if (m_audio.expr_audio.get_channel_count() == 1)
        {
            render_text_field(m_ui.expression_input_left);
        }
        else if (m_audio.expr_audio.get_channel_count() == 2)
        {
            render_text_field(m_ui.expression_input_left);
            render_text_field(m_ui.expression_input_right);
        }

        // error messages
        {
            const vec2 text_scale = vec2(300, 100);

            // @todo split this into invalid for left and right
            if (m_events[EVENT_INVALID_EXPRESSION].active)
            {
                // get the relevant error message or a generic default fallback
                Text error_text = (m_error_message.texture) ? m_error_message : m_rendered_text.get(TEXT_INVALID_EXPRESSION);

                auto tf_area = m_ui.expression_input_left.m_area;
                render_text_size(m_window.renderer, error_text,
                        vec2(tf_area.x + tf_area.w/2, tf_area.y + tf_area.h), text_scale);
            }
        }
    }

    {
        render_dropdown(m_ui.channel_count, Color(0x55, 0x33, 0x88, 0xff), Color(0x33, 0x55, 0x88, 0xff));
        render_dropdown(m_ui.playback_device, Color(0x55, 0x33, 0x88, 0xff), Color(0x33, 0x55, 0x88, 0xff));
    }
}

void Application::draw_graph_mode_ui()
{
    int window_x, window_y;
    SDL_GetWindowSize(m_window.window, &window_x, &window_y);

    vec2 window_size = vec2((float) window_x, (float) window_y);

    Text_Id text = m_audio.audio_player.paused ? TEXT_RESUME : TEXT_PAUSE;
    render_textured_rectangle(Rectangle(0,0,100,100), m_rendered_text.get(text).texture, Color(0x22,0x55,0x33,0xff));

    render_dropdown(m_ui.graph_to_show, Color(0x66, 0x44, 0x55, 0xff), Color(0x22, 0x77, 0x55, 0xff));

    switch (m_graph_to_show)
    {
        case GRAPH_AUDIO_DATA: {
            if (m_audio.audio_data.samples)
            {
                vec2 audio_data_scale = vec2(window_size.x * (5.0 / 8.0), window_size.y * (5.0 / 8.0));
                vec2 audio_data_position = vec2(window_size.x/2, window_size.y/2 + window_size.y/3);
                render_audio_data(audio_data_position, vec2(audio_data_scale.x, audio_data_scale.y), Color(0x44, 0x22, 0x77, 0xff));

                // playback position
                float playback_position = (float) m_audio.audio_player.playback_position / m_audio.audio_player.audio_data->frame_count;
                float audio_data_start = audio_data_position.x - audio_data_scale.x / 2;
                draw_arrowhead(m_window.renderer, vec2(audio_data_start + audio_data_scale.x * playback_position, audio_data_position.y - audio_data_scale.y), vec2(0, 1), 50, ColorF(0.4, 0.2, 0.7, 1.0));
            }

            if (m_signal.samples.data)
            {
                render_signal(vec2(window_size.x / 2, window_size.y / 2), vec2(window_size.x * (2.0 / 3.0), window_size.y * (1.0 / 4.0)), m_signal, Color(0x55, 0x99, 0x55, 0xff));
            }

            break;
        }
        case GRAPH_FOURIER_TRANSFORM: {
            break;
        }
    }
}

void Application::draw_common_ui() {
    // graphs button
    {
        int text = (mode == ApplicationMode::AppModeSound) ? TEXT_GRAPH_MODE : TEXT_SOUND_MODE;
        render_textured_rectangle(m_ui.graphs_button,
            m_rendered_text[text].texture,
            Color(0x66, 0x55, 0x55, 0xff)
        );
    }
}

void Application::render_slider(Rectangle area, vec2 knob_scale, float value, Color slider_color, Color knob_color, const Text& text)
{
    float slider_knob_width = area.w * knob_scale.x;
    float slider_knob_height = area.h * knob_scale.y;

    SDL_SetRenderDrawColor(m_window.renderer, COLOR_ARG(slider_color));
    SDL_FRect slider = { area.x, area.y, area.w, area.h };
    SDL_RenderFillRect(m_window.renderer, &slider);
    float percentage = value;
    SDL_SetRenderDrawColor(m_window.renderer, COLOR_ARG(knob_color));
    SDL_FRect slider_knob = {
        slider.x - (slider_knob_width / 2) + (slider.w * percentage), slider.y + slider.h / 2 - slider_knob_height / 2,
        slider_knob_width, slider_knob_height
    };
    SDL_RenderFillRect(m_window.renderer, &slider_knob);

    // text
    {
        const int margin = 10;
        render_text_scale(m_window.renderer, text,
            vec2(slider.x + slider.w / 2, slider.y + slider.h * 2 + margin), vec2(0.6, 0.6));
    }
}

void Application::render_text_field(const Text_Field& text_field)
{
    SDL_SetRenderDrawColor(m_window.renderer, 0x66, 0x66, 0x55, 0xff);
    SDL_FRect tf_area = { text_field.m_area.x, text_field.m_area.y, text_field.m_area.w, text_field.m_area.h };
    SDL_RenderFillRect(m_window.renderer, &tf_area);

    SDL_Texture* text_texture = text_field.m_texture;
    float texture_width;
    float texture_height;
    SDL_GetTextureSize(text_texture, &texture_width, &texture_height);

    if (text_texture)
    {
        int line_count = text_field.m_line_count;
        float font_size = text_field.m_font_size;

        SDL_FRect string_area = { tf_area.x, tf_area.y, texture_width, texture_height };
        SDL_FRect texture_area = { 0, 0, texture_width, texture_height };
        SDL_RenderTexture(m_window.renderer, text_texture, &texture_area, &string_area);

        SDL_SetRenderDrawColor(m_window.renderer, 0x33, 0x56, 0x74, 0xff);

        SDL_FRect cursor = SDL_FRect{   tf_area.x + text_field.m_cursor_pixel_x,
                                        tf_area.y + text_field.m_cursor_pixel_y,
                                        tf_area.w / 100, font_size };

        if (doing_text_input)
        {
            SDL_RenderFillRect(m_window.renderer, &cursor);
        }
    }
}

void Application::render_dropdown(const Drop_Down_List& list, Color title_color, Color option_color) {
    SDL_SetRenderDrawColor(m_window.renderer, COLOR_ARG(title_color));

    SDL_FRect header_area = {
        list.pos.x - list.scale.x/2, list.pos.y - list.scale.y / 2,
        list.scale.x, list.scale.y
    };
    SDL_RenderFillRect(m_window.renderer, &header_area);
    render_text_size(m_window.renderer, list.title,
        vec2(header_area.x + header_area.w / 2, header_area.y + header_area.h / 2), vec2(header_area.w, header_area.h));

    if (list.open) {
        SDL_SetRenderDrawColor(m_window.renderer, COLOR_ARG(option_color));

        for (int i = 0; i < list.options.size(); i++) {
            SDL_FRect area = header_area;
            area.y += area.h * (i + 1);
            SDL_RenderFillRect(m_window.renderer, &area);
            render_text_size(m_window.renderer, list.get_option_label(i),
                vec2(area.x + area.w/2, area.y + area.h/2), vec2(area.w, area.h));
        }
    }
}

bool Application::keyboard_input_common(SDL_KeyboardEvent keyboard) {
    switch (keyboard.scancode)
    {
        case SDL_SCANCODE_ESCAPE:
        {
            quit = true;
            return true;
        }

        case SDL_SCANCODE_T:
        {
            if (m_audio.audio_data.samples)
            {
                Signal audio = m_audio.audio_data.as_signal();
                if (audio.samples.data) {
                    DArray<Complex> fourier_result = calculate_fourier(audio);
                }
            }

            return true;
        }
        default: {
            return false;
        }
    }
}

bool Application::keyboard_input_sound_mode(SDL_KeyboardEvent keyboard) {
    switch (keyboard.scancode)
    {
        case SDL_SCANCODE_RETURN:
        {
            Text_Field* text_field = m_ui.get_selected_text_field();

            if (text_field)
            {
                bool set = false;

                if (m_ui.text_input_target == EXPRESSION_INPUT_LEFT && m_audio.expr_audio.get_channel_count() == 1)
                {
                    set = set_eval_string(text_field->get_string());
                }
                else if (m_audio.expr_audio.get_channel_count() == 2)
                {
                    if (m_ui.text_input_target == EXPRESSION_INPUT_LEFT)
                    {
                        set = set_eval_string_left(text_field->get_string());
                    }
                    else if (m_ui.text_input_target == EXPRESSION_INPUT_RIGHT)
                    {
                        set = set_eval_string_right(text_field->get_string());
                    }
                }

                if (m_ui.text_input_target == VARIABLE_NAME)
                {
                    // do nothing
                }

                if (m_ui.text_input_target == VARIABLE_VALUE)
                {
                    // id should match the index
                    int index = m_ui.selected_variable_value_index;

                    String value_string = m_ui.variable_values.get_ref(index).get_string();
                    bool convertion_success = false;
                    double value = string_to_real(value_string, &convertion_success);

                    if (convertion_success)
                    {
                        printf("%.2f\n", value);
                        st_sampler_set_variable_value(m_samplers.audio_left, index, value);
                    }
                    else
                    {
                        // @todo display error
                    }
                }

                text_input_stop();
            }

            return true;
        }
        case SDL_SCANCODE_BACKSPACE:
        {
            if (doing_text_input)
            {
                auto text_field = m_ui.get_selected_text_field();
                if (text_field)
                {

                    text_field->delete_at_cursor();
                    update_input_string();
                }
            }
            return true;
        }
        case SDL_SCANCODE_DELETE:
        {
            if (doing_text_input)
            {
                auto text_field = m_ui.get_selected_text_field();
                if (text_field)
                {
                    text_field->delete_after_cursor();
                    update_input_string();
                }
            }
        }
        case SDL_SCANCODE_SPACE:
        {
            if (!doing_text_input)
            {
                m_audio.expr_audio.toggle_pause();
            }
            return true;
        }
        case SDL_SCANCODE_V:
        {
            if (keyboard.mod & SDL_KMOD_LCTRL)
            {
                if (doing_text_input)
                {
                    printf("Paste\n");

                    char* clipboard = SDL_GetClipboardText();
                    if (strlen(clipboard) == 0)
                    {
                        fprintf(stderr, "Failed to get clipboard: %s\n", SDL_GetError());
                        return true;
                    }

                    auto field = m_ui.get_selected_text_field();
                    ASSERT(field);  // doing_text_input is set
                    field->append_string(make_string(clipboard));
                    update_input_string();

                    SDL_free(clipboard);
                }
            }

            return true;
        }
        case SDL_SCANCODE_C:
        {
            // @todo implement copying after implementing proper text selection
            // SDL_SetClipboardText();
            return true;
        }
        case SDL_SCANCODE_R:
        {
            // @todo assign this to ui buttons instead

            if (keyboard.mod & SDL_KMOD_LCTRL)
            {
                int sample_rate = st_sampler_get_sample_rate(m_samplers.waveform_left);
                // a second worth of samples
                Signal signal = create_signal(m_samplers.waveform_left, 0.0, sample_rate, sample_rate);
                if (signal.samples.data)
                {
                    printf("Saved signal\n");

                    if (m_signal.samples.data)
                    {
                        m_signal.samples.free_data();
                    }
                    m_signal = signal;

                    printf("\n");
                }
            }

            return true;
        }
        case SDL_SCANCODE_S:
        {
            // ctrl + s to save
            if (keyboard.mod & SDL_KMOD_LCTRL) {
                const char* save_file = "save.st";
                printf("Saving to file %s\n", save_file);
                if (!save_app_state(String(save_file))) {
                    fprintf(stderr, "Could not save state to file\n");
                }
            }

            return true;
        }
        case SDL_SCANCODE_L:
        {
            // ctrl + l to load
            if (keyboard.mod & SDL_KMOD_CTRL) {
                const char* save_file = "save.st";
                printf("Loading file %s\n", save_file);
                if (!load_app_state(String(save_file))) {
                    fprintf(stderr, "Could not load file %s\n", save_file);
                }
            }

            return true;
        }
        default:
        {
            return false;
        }
    }
}

bool Application::keyboard_input_graph_mode(SDL_KeyboardEvent keyboard) {
    switch (keyboard.scancode)
    {
        case SDL_SCANCODE_SPACE: {
            m_audio.audio_player.toggle_pause();
            return true;
        }
        default: return false;
    }
}

bool Application::mouse_input()
{
    if (mouse_input_common())
    {
        return true;
    }

    if (mode == AppModeSound)
    {
        return mouse_input_sound_mode();
    }
    else if (mode == AppModeGraph)
    {
        return mouse_input_graph_mode();
    }
    else
    {
        ASSERT(false); // bug case
        return false;
    }
}

bool Application::mouse_input_common()
{
    if (m_ui.graphs_button.contains(m_mouse.pos))
    {
        switch_modes();
        return true;
    }

    return false;
}

bool Application::mouse_input_graph_mode()
{
    if (m_ui.playback_pause.contains(m_mouse.pos))
    {
        m_audio.audio_player.toggle_pause();
        return true;
    }

    {
        Rectangle graph_header = Rectangle(
            m_ui.graph_to_show.pos.x - m_ui.graph_to_show.scale.x / 2, m_ui.graph_to_show.pos.y - m_ui.graph_to_show.scale.y / 2,
            m_ui.graph_to_show.scale.x, m_ui.graph_to_show.scale.y
        );

        if (graph_header.contains(m_mouse.pos)) {
            m_ui.graph_to_show.toggle();
            return true;
        }

        if (m_ui.graph_to_show.open) {
            Rectangle audio_data_area = graph_header;
            Rectangle fourier_area = graph_header;

            audio_data_area.y += graph_header.h;
            fourier_area.y += graph_header.h * 2;

            bool got_clicked = true;
            if (audio_data_area.contains(m_mouse.pos)) {
                m_graph_to_show = GRAPH_AUDIO_DATA;
            }
            else if (fourier_area.contains(m_mouse.pos)) {
                m_graph_to_show = GRAPH_FOURIER_TRANSFORM;
            }
            else {
                got_clicked = false;
            }

            if (got_clicked) {
                m_ui.graph_to_show.open = false;
                return true;
            }
        }
    }


    return false;
}

bool Application::mouse_input_sound_mode()
{
    int window_x; int window_y;
    SDL_GetWindowSize(m_window.window, &window_x, &window_y);
    vec2 window_size = vec2(window_x, window_y);

    if (m_ui.volume_slider.contains(m_mouse.pos))
    {
        float diff = m_mouse.pos.x - m_ui.volume_slider.x;
        float volume = diff / m_ui.volume_slider.w;

        volume = snap_value(volume, 0.0, 1.0, 0.06);

        m_audio.expr_audio.set_volume(volume);

        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%.3f", volume);
        m_rendered_text.data[TEXT_VOLUME_VALUE].clear();
        m_rendered_text.data[TEXT_VOLUME_VALUE] = create_text(make_string(buffer), m_assets.font_large, Color(0x54, 0x22, 0x77, 0xff));

        return true;
    }

    if (m_ui.pause_button.contains(m_mouse.pos))
    {
        m_audio.expr_audio.toggle_pause();

        return true;
    }

    {
        Rectangle channel_count_header = Rectangle(
            m_ui.channel_count.pos.x - m_ui.channel_count.scale.x / 2, m_ui.channel_count.pos.y - m_ui.channel_count.scale.y / 2,
            m_ui.channel_count.scale.x, m_ui.channel_count.scale.y
        );

        if (channel_count_header.contains(m_mouse.pos)) {
            m_ui.channel_count.toggle();
            return true;
        }

        if (m_ui.channel_count.open) {
            Rectangle mono_area = channel_count_header;
            Rectangle stereo_area = channel_count_header;
            mono_area.y += channel_count_header.h;
            stereo_area.y += channel_count_header.h * 2;

            bool got_clikcked = true;
            if (mono_area.contains(m_mouse.pos)) {
                if (!update_channel_count(1)) {
                    fprintf(stderr, "Could not update channel count\n");
                }
            }
            else if (stereo_area.contains(m_mouse.pos)) {
                if (!update_channel_count(2)) {
                    fprintf(stderr, "Could not update channel count\n");
                }
            }
            else {
                got_clikcked = false;
            }

            if (got_clikcked) {
                m_ui.channel_count.open = false;
                return true;
            }
        }
    }

    {
        Rectangle playback_device_header = Rectangle(
            m_ui.playback_device.pos.x - m_ui.playback_device.scale.x / 2, m_ui.playback_device.pos.y - m_ui.playback_device.scale.y / 2,
            m_ui.playback_device.scale.x, m_ui.playback_device.scale.y
        );

        if (playback_device_header.contains(m_mouse.pos)) {
            m_ui.playback_device.toggle();
            return true;
        }

        if (m_ui.playback_device.open) {
            bool got_clikcked = false;
            for (int i = 0; i < m_ui.playback_device.options.size(); i++) {
                Rectangle area = playback_device_header;
                area.y += playback_device_header.h * (i+1);

                if (area.contains(m_mouse.pos)) {
                    m_audio.expr_audio.set_playback_device(m_ui.playback_device.get_option_data_index(i));
                    m_ui.playback_device.selected = i;

                    got_clikcked = true;
                }
            }

            if (got_clikcked) {
                m_ui.playback_device.open = false;
                return true;
            }
        }
    }

    if (m_ui.add_variable_button.center().contains(m_mouse.pos)) {
        String variable_name = m_ui.variable_name.get_string();
        if (variable_name.size > 0)
        {
            // register the variable
            st_sampler_register_variable(m_samplers.audio_left, variable_name.data, variable_name.size, Var_Type_Real);
            st_sampler_register_variable(m_samplers.audio_right, variable_name.data, variable_name.size, Var_Type_Real);

            reinit_samplers();

            m_variable_text.add(create_text(variable_name, m_assets.font_editor, Color(0x88, 0x33, 0x66, 0xff)));

            int var_count = st_sampler_get_variable_count(m_samplers.audio_left);
            Rectangle value_area = Rectangle(window_size.x * float(2.0 / 8.0),
                                            window_size.y * Variable_Table_Vertical_Element_Size * (var_count - 1),
                                             window_size.x * Variable_Table_Horizontal_Element_Size,
                                             window_size.y * Variable_Table_Vertical_Element_Size);
            m_ui.variable_values.add(Text_Field(value_area));

            SCOPE_STRING(variable_name, var_name);
            printf("Added variable: %s\n", var_name);
            m_ui.variable_name.clear();
        }
        return true;
    }

    if (m_ui.variable_name.m_area.contains(m_mouse.pos)) {
        if (!(doing_text_input && m_ui.text_input_target == VARIABLE_NAME)) {
            // not wrapped
            m_ui.variable_name.set_text_input_area(m_window.window, m_ui.variable_name.m_area.h);
            toggle_text_input();
            m_ui.text_input_target = VARIABLE_NAME;
        }

        vec2 relative = m_mouse.pos - vec2(m_ui.variable_name.m_area.x, m_ui.variable_name.m_area.y);
        String string = m_ui.variable_name.get_string();
        m_ui.variable_name.m_selection_start = m_ui.variable_name.calculate_cursor_from_mouse(relative, string, m_assets.font_editor);
        m_ui.variable_name.m_selection_end = m_ui.variable_name.m_selection_start;
    }
    else if (m_ui.text_input_target == VARIABLE_NAME) {
        clear_text_input_selection();
        return true;
    }

    int value_field_index = 0;
    for (auto& var_value : m_ui.variable_values)
    {
        if (var_value.m_area.contains(m_mouse.pos))
        {
            if (!doing_text_input)
            {
                var_value.set_text_input_area(m_window.window, var_value.m_area.h);
                toggle_text_input();
                m_ui.text_input_target = VARIABLE_VALUE;
            }

            vec2 relative = m_mouse.pos - vec2(var_value.m_area.x, var_value.m_area.y);
            String string = var_value.get_string();
            var_value.m_selection_start = var_value.calculate_cursor_from_mouse(relative, string, m_assets.font_editor);
            var_value.m_selection_end = var_value.m_selection_start;

            m_ui.selected_variable_value_index = value_field_index;

            return true;
        }

        value_field_index += 1;
    }

    // no variable value field was clicked
    if (m_ui.text_input_target == VARIABLE_VALUE)
    {
        clear_text_input_selection();
        return true;
    }


    if (m_ui.expression_input_left.m_area.contains(m_mouse.pos))
    {
        int line_skip = TTF_GetFontLineSkip(m_assets.font_editor.font);
        if (!(doing_text_input && m_ui.text_input_target == EXPRESSION_INPUT_LEFT))
        {
            m_ui.expression_input_left.set_text_input_area(m_window.window, line_skip);
            toggle_text_input();
            m_ui.text_input_target = EXPRESSION_INPUT_LEFT;
        }

        vec2 relative = m_mouse.pos - vec2(m_ui.expression_input_left.m_area.x, m_ui.expression_input_left.m_area.y);
        String string = m_ui.expression_input_left.get_string();
        m_ui.expression_input_left.m_selection_start = m_ui.expression_input_left.calculate_cursor_from_mouse(relative, string, m_assets.font_editor);
        m_ui.expression_input_left.m_selection_end = m_ui.expression_input_left.m_selection_start;

        return true;
    }
    else if (m_ui.text_input_target == EXPRESSION_INPUT_LEFT) {
        clear_text_input_selection();
        return true;
    }

    if (m_audio.expr_audio.get_channel_count() == 2)
    {
        if (m_ui.expression_input_right.m_area.contains(m_mouse.pos))
        {
            int line_skip = TTF_GetFontLineSkip(m_assets.font_editor.font);
            m_ui.expression_input_right.set_text_input_area(m_window.window, line_skip);

            toggle_text_input();

            m_ui.text_input_target = EXPRESSION_INPUT_RIGHT;

            return true;
        }
        else if (m_ui.text_input_target == EXPRESSION_INPUT_LEFT) {
            clear_text_input_selection();
            return true;
        }
    }

    return false;
}

void Application::clear_text_input_selection()
{
    m_ui.text_input_target = NO_TARGET;
    toggle_text_input();
}

void Application::switch_modes() {
    if (mode == ApplicationMode::AppModeSound) {
        mode = ApplicationMode::AppModeGraph;

        m_audio.expr_audio.pause();
    }
    else if (mode == ApplicationMode::AppModeGraph) {
        mode = ApplicationMode::AppModeSound;

        m_audio.audio_player.pause();
    }
}

void Application::text_input_stop()
{
    SDL_StopTextInput(m_window.window);
    doing_text_input = false;

    m_ui.text_input_target = NO_TARGET;  // ?

    m_background_color = DEFAULT_BACKGROUND_COLOR;
}

void Application::text_input_start()
{
    SDL_StartTextInput(m_window.window);
    doing_text_input = true;

    m_background_color = {0, 0x44, 0x66, 0xff};
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

void Application::update_ui_state(vec2 window_size)
{
    // @todo deduplicate calculations

    m_ui.volume_slider.x = window_size.x * (1.0 / 20.0);
    m_ui.volume_slider.y = window_size.y * (1.0 / 8.0);

    m_ui.pause_button.x = (window_size.x - m_ui.pause_button.w) / 2;
    m_ui.pause_button.y = (window_size.y - m_ui.pause_button.h) / 2;

    m_ui.graphs_button.x = window_size.x * (4.0 / 5.0);
    m_ui.graphs_button.y = window_size.y * (1.0 / 5.0);

    m_ui.add_variable_button.x = window_size.x * (15.0 / 16.0);
    m_ui.add_variable_button.y = window_size.y * (1.0 / 16.0);
    m_ui.add_variable_button.w = window_size.x * (1.0 / 16.0);
    m_ui.add_variable_button.h = m_ui.add_variable_button.w;

    m_ui.variable_name.m_area.x = window_size.x * (1.0 / 2.0);
    m_ui.variable_name.m_area.y = window_size.y * (1.0 / 32.0);
    m_ui.variable_name.m_area.w = window_size.x * (1.0 / 3.0);
    m_ui.variable_name.m_area.h = window_size.y * (1.0 / 16.0);

    if (m_audio.expr_audio.get_channel_count() == 1)
    {
        m_ui.expression_input_left.m_area.w = window_size.x * (1.0 / 2.0);
        m_ui.expression_input_left.m_area.h = window_size.y * (1.0 / 5.0);
        m_ui.expression_input_left.m_area.x = window_size.x / 2 - m_ui.expression_input_left.m_area.w / 2;
        m_ui.expression_input_left.m_area.y = window_size.y / 2 + m_ui.expression_input_left.m_area.h;
    }
    else if (m_audio.expr_audio.get_channel_count() == 2)
    {
        m_ui.expression_input_left.m_area.w = window_size.x * (1.0 / 3.0);
        m_ui.expression_input_left.m_area.h = window_size.y * (1.0 / 5.0);
        m_ui.expression_input_left.m_area.x = window_size.x * (1.0 / 9.0);
        m_ui.expression_input_left.m_area.y = window_size.y / 2 + m_ui.expression_input_left.m_area.h;

        m_ui.expression_input_right.m_area.w = window_size.x * (1.0 / 3.0);
        m_ui.expression_input_right.m_area.h = window_size.y * (1.0 / 5.0);
        m_ui.expression_input_right.m_area.x = window_size.x * (5.0 / 9.0);
        m_ui.expression_input_right.m_area.y = window_size.y / 2 + m_ui.expression_input_right.m_area.h;
    }

    m_ui.channel_count.set_area(vec2(window_size.x / 2,
                                     window_size.y * (1.0 / 5.0)),
                                vec2(window_size.x * (1.0 / 5.0),
                                     window_size.y * (1.0 / 10.0))
    );

    m_ui.playback_device.set_area(vec2(window_size.x * (0.8 / 3.0),
                                       window_size.y * (1.5 / 5.0)),
                                  vec2(window_size.x * (1.0 / 5.0),
                                       window_size.y * (1.0 / 10.0))
    );

    m_ui.graph_to_show.set_area(vec2(window_size.x * (1.0 / 2.0),
                                window_size.y * (1.0 / 16.0)),
                                vec2(window_size.x * (1.0 / 4.0),
                                window_size.y * (1.0 / 8.0))
    );

    int var_value_index = 0;
    for (auto& var_value : m_ui.variable_values)
    {
        Rectangle area = Rectangle( window_size.x * float(2.0 / 8.0),
                                    window_size.y * Variable_Table_Vertical_Element_Size * var_value_index,
                                    window_size.x * Variable_Table_Horizontal_Element_Size,
                                    window_size.y * Variable_Table_Vertical_Element_Size);

        var_value.m_area = area;

        var_value_index += 1;
    }
}

Text_Field* Ui_State::get_selected_text_field()
{
    switch (text_input_target)
    {
    case NO_TARGET:
        return NULL;
    case EXPRESSION_INPUT_LEFT:
        return &expression_input_left;
    case EXPRESSION_INPUT_RIGHT:
        return &expression_input_right;
    case VARIABLE_NAME:
        return &variable_name;
    case VARIABLE_VALUE:
    {
        return variable_values.get_ptr(selected_variable_value_index);
    }
    default:
        // error?
        return NULL;
    }
}

bool Text_Field::render_text_field_texture(SDL_Renderer* renderer, Font font, Color color, bool wrapped)
{
    SDL_DestroyTexture(m_texture);  // old texture
    m_texture = nullptr;

    String str = get_string();
    if (str.size == 0)
    {
        return true;
    }

    const SDL_Color text_color = {color.r, color.g, color.b, color.a};
    SDL_Surface* text_surface;

    if (wrapped) {
        text_surface = TTF_RenderText_Solid_Wrapped(font.font, str.data, str.size, text_color, m_area.w);
    } else {
        text_surface = TTF_RenderText_Solid(font.font, str.data, str.size, text_color);
    }

    if (!text_surface)
    {
        return false;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    SDL_DestroySurface(text_surface);

    if (!texture)
    {
        return false;
    }

    float texture_width, texture_height;
    SDL_GetTextureSize(texture, &texture_width, &texture_height);
    int line_skip = TTF_GetFontLineSkip(font.font);
    int line_count = (wrapped) ? MAX(1, (int)(texture_height / line_skip)) : (1);

    calculate_cursor_from_selection(str, font);

    m_line_count = line_count;
    m_texture = texture;
    m_font_size = font.size;

    return true;
}

bool Application::update_input_string()
{
    auto text_field = m_ui.get_selected_text_field();
    if (!text_field)
    {
        return false;
    }
    return text_field->update_text(m_window.renderer, m_assets.font_editor,
                                   (m_ui.text_input_target == EXPRESSION_INPUT_LEFT) || (m_ui.text_input_target == EXPRESSION_INPUT_RIGHT));
}

bool Application::reinit_samplers()
{
    String eval_string = m_ui.expression_input_left.get_string();
    return set_eval_string(eval_string);
}

bool Application::set_eval_string(String eval_string)
{
    bool left =  st_sampler_set_expression(m_samplers.audio_left,  eval_string.data, eval_string.size);
    bool right = st_sampler_set_expression(m_samplers.audio_right, eval_string.data, eval_string.size);

    bool success = left && right;
    if (!success) {
        fprintf(stderr, "Failed to set sample expression\n");

        const char* error = st_get_last_error();
        if (error)
        {
            if (m_error_message.texture)
            {
                destroy_text(m_error_message);
            }
            m_error_message = create_text(make_string(error), m_assets.font_medium, Color(0xAA, 0x55, 0x44, 0xFF));
        }

        set_event_active(EVENT_INVALID_EXPRESSION, 15.0);
        return false;
    }
    else {
        set_event_deactive(EVENT_INVALID_EXPRESSION);
    }

    if (m_samplers.waveform_left)
    {
        st_sampler_destroy(m_samplers.waveform_left);
    }
    if (m_samplers.waveform_right)
    {
        st_sampler_destroy(m_samplers.waveform_right);
    }

    m_samplers.waveform_left = st_sampler_copy(m_samplers.audio_left);
    m_samplers.waveform_right = st_sampler_copy(m_samplers.audio_right);

    ASSERT(m_samplers.waveform_left && m_samplers.waveform_right);

    st_sampler_set_sample_rate(m_samplers.waveform_left, WAVEFORM_SAMPLE_RATE);
    st_sampler_set_sample_rate(m_samplers.waveform_right, WAVEFORM_SAMPLE_RATE);

    return success;
}

bool Application::set_eval_string_left(String eval_string)
{
    bool success = st_sampler_set_expression(m_samplers.audio_left, eval_string.data, eval_string.size);
    if (!success)
    {
        fprintf(stderr, "Failed to set left sample expression\n");
    }

    m_samplers.waveform_left = st_sampler_copy(m_samplers.audio_left);
    ASSERT(m_samplers.waveform_left);
    st_sampler_set_sample_rate(m_samplers.waveform_left, WAVEFORM_SAMPLE_RATE);

    return success;
}

bool Application::set_eval_string_right(String eval_string)
{
    bool success = st_sampler_set_expression(m_samplers.audio_right, eval_string.data, eval_string.size);
    if (!success)
    {
        fprintf(stderr, "Failed to set right sample expression\n");
    }

    m_samplers.waveform_right = st_sampler_copy(m_samplers.audio_right);
    ASSERT(m_samplers.waveform_right);
    st_sampler_set_sample_rate(m_samplers.waveform_right, WAVEFORM_SAMPLE_RATE);

    return success;
}

float get_signal_sample(void* user, int frame, int channel) {
    Signal* signal = (Signal*) user;
    return signal->samples.data[frame];
}

float get_audio_sample(void* user, int frame, int channel) {
    AudioData* data = (AudioData*) user;
    if (data->format != SDL_AUDIO_F32)  // panic maybe?
        return 0.0;
    float* samples = (float*)data->samples;
    return samples[frame * data->channel_count + channel];
}

void Application::render_waveform(vec2 area_center, vec2 area_scale, int frame_count, int channel_count, Color color, SampleGetter sample_getter, void* user_data, bool draw_lines)
{
    SDL_SetRenderDrawColor(m_window.renderer, COLOR_ARG(color));

    #define BLOCK_SIZE 256
    static SDL_FPoint points[AUDIO_MAX_CHANNELS][BLOCK_SIZE];

    float vertical_step = area_scale.y / channel_count;
    float step = (area_scale.x / float(frame_count));
    float base_height = area_center.y;

    int iter_count = frame_count / BLOCK_SIZE;

    for (int block = 0; block < iter_count; block++)
    {
        int block_start = block * BLOCK_SIZE;

        for (int i = 0; i < BLOCK_SIZE; i+=1)
        {
            int frame_index = block * BLOCK_SIZE + i;

            for (int ch = 0; ch < channel_count; ch++)
            {
                float sample = sample_getter(user_data, frame_index, ch);

                points[ch][i].x = area_center.x - (area_scale.x / 2) + (step * frame_index);
                points[ch][i].y = (base_height + sample * (area_scale.y / 2.0)) / channel_count + (ch * vertical_step);
            }
        }

        for (int ch = 0; ch < channel_count; ch++)
        {
            if (draw_lines) {
                SDL_RenderLines(m_window.renderer, points[ch], BLOCK_SIZE);
            }
            else {
                SDL_RenderPoints(m_window.renderer, points[ch], BLOCK_SIZE);
            }
        }
    }

    int remaining = frame_count - (iter_count * BLOCK_SIZE);

    ASSERT(remaining < BLOCK_SIZE);

    for (int i = 0; i < remaining; i+=1)
    {
        int frame_index = iter_count * BLOCK_SIZE + i;

        for (int ch = 0; ch < channel_count; ch++)
        {
            float sample = sample_getter(user_data, frame_index, ch);

            points[ch][i].x = area_center.x - (area_scale.x / 2) + (step * frame_index);
            points[ch][i].y = base_height + sample * (area_scale.y / 2.0) / channel_count + (ch * vertical_step);
        }
    }

    for (int ch = 0; ch < channel_count; ch++)
    {
        if (draw_lines) {
            SDL_RenderLines(m_window.renderer, points[ch], remaining);
        }
        else {
            SDL_RenderPoints(m_window.renderer, points[ch], remaining);
        }
    }

    #undef BLOCK_SIZE
}

// @todo we can do cooler things with rendering waveforms or audio data

void Application::render_audio_data(vec2 area_center, vec2 area_scale, Color color) {

    if (!(m_audio.audio_data.samples && m_audio.audio_data.is_in_desired_spec())) {
        return;
    }

    render_waveform(area_center, area_scale, m_audio.audio_data.frame_count, m_audio.audio_data.channel_count, color, get_audio_sample, &m_audio.audio_data, false);
}

void Application::render_signal(vec2 area_center, vec2 area_scale, Signal signal, Color color) {
    render_waveform(area_center, area_scale, m_signal.samples.size, 1, color, get_signal_sample, &signal, true);
}

void Application::update_waveform(St_Sampler* sampler, Array<SDL_FPoint> sample_buffer, vec2 area_center, vec2 area_scale) {
    int sample_count = sample_buffer.size;

    st_fill_strided(sampler, &sample_buffer.data[0].y, sample_count, 2);

    const SDL_FRect area = SDL_FRect{ area_center.x - area_scale.x / 2, area_center.y - area_scale.y / 2, area_scale.x, area_scale.y };

    float half_h = area.h / 2;
    float middle_y = area.y + half_h;

    float step_size = (area.w / (float)sample_count);
    float start_x = area.x + step_size / 2;
    for (int i = 0; i < sample_count; i++) {
        sample_buffer[i].x = start_x + i * step_size;
        float sample = sample_buffer[i].y;
        sample_buffer[i].y = middle_y - (sample * half_h);
    }
}

bool AudioData::load_audio_file(String path) {
    SCOPE_STRING(path, path_c_str);

    u8* output_buffer;
    int output_length = 0;

    // the spec we want
    SDL_AudioSpec desired_spec;
    desired_spec.channels = 2;
    desired_spec.format = SDL_AUDIO_F32;
    desired_spec.freq = 48000;

    {
        // @todo other file formats than wav

        SDL_AudioSpec spec;  // output parameter
        u8* buffer = nullptr;
        u32 audio_length = 0;
        if (!SDL_LoadWAV(path_c_str, &spec, &buffer, &audio_length)) {
            fprintf(stderr, "Couldn't load audio file %s: %s\n", path_c_str, SDL_GetError());
            return false;
        }

        printf("%s\n", SDL_GetAudioFormatName(spec.format));

        // accept the channel count of the input file
        desired_spec.channels = spec.channels;
        // except we may not be ready for surround setups
        if (desired_spec.channels > 2)
            desired_spec.channels = 2;

        bool convert_success = SDL_ConvertAudioSamples(&spec, buffer, audio_length, &desired_spec, &output_buffer, &output_length);

        SDL_free(buffer);
        audio_length = 0;

        if (!convert_success)
        {
            fprintf(stderr, "Couldn't convert audio samples to desired spec. %s\n", SDL_GetError());
            return false;
        }
    }

    samples = output_buffer;
    channel_count = desired_spec.channels;
    format = desired_spec.format;
    frequency = desired_spec.freq;
    frame_count = output_length / (SDL_AUDIO_BYTESIZE(desired_spec.format) * desired_spec.channels);

    return true;
}

bool Application::save_app_state(String filepath) {
    File file = File(filepath, "w");

    Save_State save;

    save.volume = m_audio.expr_audio.get_volume();
    save.sample_rate = m_audio.expr_audio.get_sample_rate();
    save.expression_left = m_ui.expression_input_left.get_string();
    save.expression_right = m_ui.expression_input_right.get_string();

#define BUFFER_SIZE 1024
    char buffer[BUFFER_SIZE];

    fprintf(file.handle, "volume:%f\n", save.volume);
    fprintf(file.handle, "sample_rate:%f\n", save.sample_rate);

    memcpy(buffer, save.expression_left.data, save.expression_left.size);
    buffer[save.expression_left.size] = '\0';
    fprintf(file.handle, "expression_left:%s\n", buffer);

    memcpy(buffer, save.expression_right.data, save.expression_right.size);
    buffer[save.expression_right.size] = '\0';
    fprintf(file.handle, "expression_left:%s\n", buffer);

    return true;
}

bool Application::load_app_state(String filepath) {
    // @todo error handling

    BinaryData file_raw;
    String file_content;

    Save_State save = {};

    {
        SCOPE_STRING(filepath, filepath_cstr);
        if (!load_file(filepath_cstr, file_raw)) {
            return false;
        }

        file_content = String(file_raw);
    }

    float volume = 0;
    float sample_rate = 0;
    String expr_left = {};
    String expr_right = {};

    for (int i = 0; i < 4; i++) {
        auto line = string_cut_from_character(file_content, '\n');
        String option = line.at(0);
        file_content = line.at(1);

        auto sep = string_cut_from_character(option, ':');
        String name = sep.at(0);
        String value = string_copy(sep.at(1));

        name.trim();
        value.trim();

        if (string_compare(name, String("volume"))) {
            volume = string_to_real(value, nullptr);
        }
        else if (string_compare(name, String("sample_rate"))) {
            sample_rate = string_to_real(value, nullptr);
        }
        else if (string_compare(name, String("expression_left"))) {
            expr_left = value;
        }
        else if (string_compare(name, String("expression_right"))) {
            expr_right = value;
        }
        else {
            return false;
        }
    }

    save.volume = volume;
    save.sample_rate = sample_rate;
    save.expression_left = expr_left;
    save.expression_right = expr_right;

    return true;
}

void Application::render_textured_rectangle(Rectangle rect, SDL_Texture* texture, Color color) {
    SDL_SetRenderDrawColor(m_window.renderer, COLOR_ARG(color));
    SDL_FRect area = { rect.x, rect.y, rect.w, rect.h };
    SDL_RenderFillRect(m_window.renderer, &area);

    float tex_w, tex_h;
    SDL_GetTextureSize(texture, &tex_w, &tex_h);
    SDL_FRect src = {0,0,tex_w,tex_h};
    SDL_FRect dst = area;
    SDL_RenderTexture(m_window.renderer, texture, &src, &dst);
}

void render_text_size(SDL_Renderer* renderer, Text text, vec2 where, vec2 absolute_scale)
{
    float tex_w, tex_h;
    SDL_GetTextureSize(text.texture, &tex_w, &tex_h);

    if (!absolute_scale.x)
    {
        absolute_scale = vec2(tex_w, tex_h);
    }

    SDL_FRect src = { 0,0,tex_w,tex_h };
    SDL_FRect dst = {where.x - absolute_scale.x/2, where.y - absolute_scale.y/2, absolute_scale.x, absolute_scale.y};

    SDL_RenderTexture(renderer, text.texture, &src, &dst);
}

void render_text_scale(SDL_Renderer* renderer, Text text, vec2 where, vec2 scale_factor)
{
    float tex_w, tex_h;
    SDL_GetTextureSize(text.texture, &tex_w, &tex_h);

    if (!scale_factor.x)
    {
        scale_factor = vec2(1,1);
    }

    vec2 scale = vec2(tex_w * scale_factor.x, tex_h * scale_factor.y);

    SDL_FRect src = { 0,0,tex_w,tex_h };
    SDL_FRect dst = {where.x - scale.x/2, where.y - scale.y/2, scale.x, scale.y};

    SDL_RenderTexture(renderer, text.texture, &src, &dst);
}

void draw_arrowhead(SDL_Renderer* renderer, vec2 position, vec2 direction, float scale, ColorF color)
{
    SDL_Vertex vertices[3] = {};

    vec2 dir = direction.normalized();
    vec2 dir_ortho = vec2(-dir.y, dir.x);

    vertices[0].position.x = position.x + dir.x * scale;
    vertices[0].position.y = position.y + dir.y * scale;
    vertices[0].color = SDL_FColor{ color.r, color.g, color.b, color.a };

    vertices[1].position.x = position.x + dir_ortho.x * scale;
    vertices[1].position.y = position.y + dir_ortho.y * scale;
    vertices[1].color = SDL_FColor{ color.r, color.g, color.b, color.a };

    vertices[2].position.x = position.x - dir_ortho.x * scale;
    vertices[2].position.y = position.y - dir_ortho.y * scale;
    vertices[2].color = SDL_FColor{ color.r, color.g, color.b, color.a };

    int indices[3] = {0, 1, 2};

    SDL_RenderGeometry(renderer, NULL, vertices, 3, indices, 3);
}

void draw_plus(SDL_Renderer* renderer, vec2 position, vec2 scale, float thickness, ColorF color)
{
    SDL_Vertex vertices[8] = {};

    for (int i = 0; i < 8; i++) {
        vertices[i].color = SDL_FColor { color.r, color.g, color.b, color.a };
    }

    float half_x = scale.x / 2;
    float half_y = scale.y / 2;
    float half_thick = thickness / 2;

    vertices[0].position = { position.x - half_thick, position.y - half_y };
    vertices[1].position = { position.x - half_thick, position.y + half_y };
    vertices[2].position = { position.x + half_thick, position.y - half_y };
    vertices[3].position = { position.x + half_thick, position.y + half_y };
    vertices[4].position = { position.x - half_x, position.y - half_thick };
    vertices[5].position = { position.x - half_x, position.y + half_thick };
    vertices[6].position = { position.x + half_x, position.y - half_thick };
    vertices[7].position = { position.x + half_x, position.y + half_thick };

    int indices[12] = { 0, 1, 2,
                        1, 2, 3,
                        4, 5, 6,
                        5, 6, 7 };

    SDL_RenderGeometry(renderer, NULL, vertices, 8, indices, 12);
}

Signal create_signal(St_Sampler* sampler, float time_start, int sample_count, int sample_rate)
{
    Signal signal;
    float* buffer = (float*) malloc(sizeof(float) * sample_count);

    st_sampler_set_sample_time(sampler, time_start);
    st_sampler_set_sample_rate(sampler, sample_rate);
    st_fill(sampler, buffer, sample_count);

    signal.samples = Array<float>(buffer, sample_count);

    return signal;
}
