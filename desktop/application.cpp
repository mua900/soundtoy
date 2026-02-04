#include "application.h"

#include <iostream>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

/*
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
*/

static const char* org_name = "flying-carpet";
static const char* soundtoy_identifier = "flying-carpet.soundtoy";
static const char* soundtoy_name = "soundtoy";
// @todo versioning
static const char* soundtoy_version = "0.1.0";

#define WAVEFORM_SAMPLE_RATE 64
#define DEFAULT_EXPRESSION "sin(t*tau)"

bool Application::initialize()
{
    SDL_SetAppMetadata(soundtoy_name, soundtoy_version, soundtoy_identifier);

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        std::cerr << "Failed to init SDL\n";
        return false;
    }

    // window
    {
        float scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
        if (!SDL_CreateWindowAndRenderer("soundtoy", 1440, 810, flags, &window, &renderer)) {
            std::cerr << "Failed to create window and renderer\n";
            return false;
        }

        m_window = { window, renderer };

        // setup imgui
		/*
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();

            ImGuiIO& imgui_io = ImGui::GetIO();
            imgui_io.IniFilename = nullptr;
            imgui_io.LogFilename = nullptr;
            imgui_io.IniSavingRate = 0;

            imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            ImGui::StyleColorsDark();
            //ImGui::StyleColorsLight();

            ImGuiStyle& style = ImGui::GetStyle();
            style.ScaleAllSizes(scale);
            style.FontScaleDpi = scale;

            ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
            ImGui_ImplSDLRenderer3_Init(renderer);
        }
		*/
    }

    {
        // accept drop events
        SDL_SetEventEnabled(SDL_EVENT_DROP_FILE, true);
    }

    // ttf
    {
        if (!TTF_Init())
        {
            std::cerr << "Could not initialize TTF\n";
            return false;
        }
    }

    if (!load_assets()) {
        std::cerr << "Could not load assets\n";
        return false;
    }

    const int initial_sample_rate = DESIRED_AUDIO_SAMPLE_RATE;

    {
        if (!st_initialize()) {
            std::cerr << "Could not initialize soundtoy library\n";
            return false;
        }

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

        sampler_audio_left = audio_left;
        sampler_audio_right = audio_right;

		sampler_waveform_left = waveform_left;
		sampler_waveform_right = waveform_right;

        const int sample_buffer_size_audio = 512;
        float* sample_buffer_mem = new float[sample_buffer_size_audio];
        sample_buffer = Array<float>(sample_buffer_mem, sample_buffer_size_audio);

		const int sample_buffer_size_waveform = 128;
        SDL_FPoint* waveform_sample_buffer_left_mem = new SDL_FPoint[sample_buffer_size_waveform];
		SDL_FPoint* waveform_sample_buffer_right_mem = new SDL_FPoint[sample_buffer_size_waveform];
        waveform_sample_buffer_left = Array<SDL_FPoint>(waveform_sample_buffer_left_mem, sample_buffer_size_waveform);
        waveform_sample_buffer_right = Array<SDL_FPoint>(waveform_sample_buffer_right_mem, sample_buffer_size_waveform);

        // set it active to start
        set_event_active(EVENT_RECALCULATE_WAVEFORM_SAMPLES, 1.0);
    }

    if (!m_expr_audio.initialize(sample_buffer, initial_sample_rate, 1, sampler_audio_left, sampler_audio_right)) {
        std::cerr << "Failed to initialize audio\n";
        return false;
    }

    // info string
    {
        auto texts = new Text[TEXT_COUNT];
        m_rendered_text = Array<Text>(texts, TEXT_COUNT);

        gen_static_text(Color{0x44, 0x22, 0x33, 0xff});
        // dynamic text
        m_rendered_text.data[TEXT_VOLUME_VALUE] = create_text(make_string("0.0"), Color(0x54, 0x22, 0x77, 0xff));
    }

    // ui
    {
        ivec2 ws;
        SDL_GetWindowSize(m_window.window, &ws.x, &ws.y);

        update_ui_state(vec2(ws.x, ws.y));

        // @todo show what is selected in the ui
        Text mono = create_text(make_string("mono"), Color(0x44, 0x22, 0x55, 0xff));
        Text stereo = create_text(make_string("stereo"), Color(0x44, 0x22, 0x55, 0xff));
        m_ui.channel_count.set_title(create_text(make_string("Channel Count"), Color(0x44, 0x22, 0x55, 0xff)));
        m_ui.channel_count.add_option(mono, nullptr);
        m_ui.channel_count.add_option(stereo, nullptr);

        m_ui.playback_device.set_title(create_text(make_string("Audio Device"), Color(0x44, 0x22, 0x55, 0xff)));

        int count = 0;
        SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&count);

        printf("Playback devices: \n");
        for (int i = 0; i < count; i++) {
            const char* device_name = SDL_GetAudioDeviceName(devices[i]);
			// this is done in the event handler as existing devices at initialization time are provided as device_added events by SDL
            // m_ui.playback_device.add_option(create_text(make_string(device_name), Color(0x88, 0x33, 0x11, 0xff)));

            printf("%s\n", device_name);
        }
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

bool Application::gen_static_text(Color color)
{
    Text paused = create_text(make_string("Paused"), color);
    Text playing = create_text(make_string("Playing"), color);
	Text pause = create_text(make_string("pause"), color);
	Text resume = create_text(make_string("resume"), color);
    Text invalid_expression = create_text(make_string("Invalid Expression"), color);
	Text invalid_sample_rate = create_text(make_string("Invalid Sample Rate"), color);
    Text valid_expression = create_text(make_string("Valid Expression"), color);
    Text sample_rate = create_text(make_string("sample rate"), color);
    Text text_sound_mode = create_text(make_string("sound mode"), color);
    Text text_graph_mode = create_text(make_string("graph mode"), color);

    if (!
		(paused.texture &&
		 playing.texture &&
		 pause.texture &&
		 resume.texture &&
		 invalid_expression.texture &&
		 invalid_expression.texture &&
		 valid_expression.texture &&
		 sample_rate.texture
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

void Application::draw_imgui() {
    int window_x, window_y;
    SDL_GetWindowSize(m_window.window, &window_x, &window_y);
    vec2 window_size = vec2(window_x, window_y);
}

bool Application::update_channel_count(int channels)
{
    bool success = m_expr_audio.reinitialize(m_expr_audio.get_sample_rate(), channels);

    int w_x, w_y;
    SDL_GetWindowSize(m_window.window, &w_x, &w_y);
    update_ui_state(vec2(w_x, w_y));
    return success;
}

#define FONT_SIZE 100.0

bool Application::load_assets()
{
    String_Builder sb(256);
    const char* base_path = SDL_GetBasePath();
    char* pref_path = SDL_GetPrefPath(org_name, soundtoy_name);  // need to free

    sb.append(make_string(base_path));

    bool load_from_base_path = st_load_assets(sb);
    if (load_from_base_path) {
        // success

        SDL_free(pref_path);
        pref_path = nullptr;

        return true;
    }
    else {
        // failed to load from base path. Try pref path
        sb.clear_and_append(String(pref_path));

        SDL_free(pref_path);
        pref_path = nullptr;

        bool load_from_pref_path = st_load_assets(sb);
        return load_from_pref_path;
    }
}

bool Application::st_load_assets(String_Builder& sb) {
    printf("Searching for assets in %s\n", sb.c_string());

#ifdef _WIN32
    String path_seperator = make_string("\\");
#else
    String path_seperator = make_string("/");
#endif

    sb.append(make_string("asset"));
    sb.append(path_seperator);

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
        const String font_folder = make_string("font");
		const String font_name = make_string("Roboto");
		const String font_file = make_string("Roboto-VariableFont.ttf");

        sb.append(font_folder);
        sb.append(path_seperator);

        sb.append(font_name);
        sb.append(path_seperator);

        sb.append(font_file);

        {
            TTF_Font* font = TTF_OpenFont(sb.c_string(), FONT_SIZE);
            if (!font) {
                std::cerr << "Could not load font " << sb.c_string() << "\n";
                std::cerr << SDL_GetError() << "\n";
                return false;
            }

            m_assets.font = { font, FONT_SIZE };
        }

		sb.remove(font_folder.size + 1 + font_name.size + 1 + font_file.size);
    }

    return true;
}

void Application::handle_events()
{
    SDL_Event e = {};
    while (SDL_PollEvent(&e))
    {
        // ImGui_ImplSDL3_ProcessEvent(&e);

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
                        Text_Field* text_field = m_ui.get_selected_text_field();

                        if (text_field)
                        {
                            bool set = false;

                            if (m_expr_audio.get_channel_count() == 1)
                            {
                                set = set_eval_string(text_field->get_string());
                            }
                            else if (m_expr_audio.get_channel_count() == 2)
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

                            if (!set)
                            {
                                set_event_active(EVENT_INVALID_EXPRESSION, 10.0);
                            }
                            else
                            {
                                set_event_deactive(EVENT_INVALID_EXPRESSION);
                            }

                            text_input_stop();
                        }

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
                                text_field->update_text(m_window.renderer, m_assets.font, false);
                            }
                        }
                        break;
                    }
                    case SDL_SCANCODE_SPACE:
                    {
                        if (!doing_text_input)
                        {
                            m_expr_audio.toggle_pause();
                        }
                        break;
                    }
                    case SDL_SCANCODE_S: {
                        // ctrl + s to save
                        if (keyboard.mod & SDL_KMOD_LCTRL) {
                            const char* save_file = "save.st";
                            printf("Saving to file %s\n", save_file);
                            if (!save_app_state(String(save_file))) {
                                fprintf(stderr, "Could not save state to file\n");
                            }
                        }

                        break;
                    }
                    case SDL_SCANCODE_L: {
                        // ctrl + l to load
                        if (keyboard.mod & SDL_KMOD_CTRL) {
                            const char* save_file = "save.st";
                            printf("Loading file %s\n", save_file);
                            if (!load_app_state(String(save_file))) {
                                fprintf(stderr, "Could not load file %s\n", save_file);
                            }
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
                    text_field->m_text.append(input_text);
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

                m_ui.playback_device.add_option(create_text(make_string(SDL_GetAudioDeviceName(device)), Color(0x88, 0x33, 0x11, 0xff)),
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
            case SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED: { break; }

            case SDL_EVENT_DROP_FILE: {
                SDL_DropEvent drop = e.drop;

                printf("Received drop event\n");

                if (drop.data) {
                    m_audio_data.reset();

                    if (!load_audio_file(String(drop.data))) {
                        fprintf(stderr, "Could not load file %s\n", drop.data);
                    }
                }

                if (m_audio_data.is_in_desired_spec())
                {
                    m_expr_audio.pause();

                    st_set_input_stream(sampler_audio_left,  ((float*) m_audio_data.samples) + 0, m_audio_data.frame_count, 2);
                    st_set_input_stream(sampler_audio_right, ((float*) m_audio_data.samples) + 1, m_audio_data.frame_count, 2);

                    st_set_input_stream(sampler_waveform_left,  ((float*) m_audio_data.samples) + 0, m_audio_data.frame_count, 2);
                    st_set_input_stream(sampler_waveform_right, ((float*) m_audio_data.samples) + 1, m_audio_data.frame_count, 2);
                }
                else
                {
                    fprintf(stderr, "Audio data is not in the desired format\n");
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
		m_audio_player.put_audio_data();
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
    st_sampler_destroy(sampler_audio_left);
    st_sampler_destroy(sampler_audio_right);
	st_sampler_destroy(sampler_waveform_left);
	st_sampler_destroy(sampler_waveform_right);
    m_expr_audio.cleanup();

	/*
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
	*/

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

void Application::draw_sound_mode_ui()
{
    int window_x, window_y;
    SDL_GetWindowSize(m_window.window, &window_x, &window_y);
        int waveform_sample_count = waveform_sample_buffer_left.size / 2;


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
            render_waveform(sampler_waveform_left, waveform_sample_buffer_left,
                            vec2(area_left.x + area_left.w / 2, area_left.y + area_left.h / 2), vec2(area_left.w, area_left.h));
            render_waveform(sampler_waveform_right, waveform_sample_buffer_right,
                            vec2(area_right.x + area_right.w / 2, area_right.y + area_right.h / 2), vec2(area_right.w, area_right.h));

            // recalculate regularly
            set_event_active(EVENT_RECALCULATE_WAVEFORM_SAMPLES, 1.0);
        }

        const Color waveform_color = Color(0xBB, 0x44, 0x77, 0xff);

        // @todo if we are going to only recalculate samples a couple of times a second
        // maybe do some interpolation or something so that it looks better?

        SDL_SetRenderDrawColor(m_window.renderer, COLOR_ARG(waveform_color));

		SDL_RenderLines(m_window.renderer, waveform_sample_buffer_left.data, waveform_sample_buffer_left.size);
 		SDL_RenderLines(m_window.renderer, waveform_sample_buffer_right.data, waveform_sample_buffer_right.size);
	}


    // volume slider
    {
        vec2 knob_scale = { 0.1, 2 };
        Color slider_color = Color(0x55, 0x44, 0x22, 0xff);
        Color knob_color = Color(0x66, 0x55, 0x22, 0xff);

        render_slider(m_ui.volume_slider, knob_scale, m_expr_audio.get_volume(), slider_color, knob_color, m_rendered_text.data[TEXT_VOLUME_VALUE]);
    }

    // pan slider
    {
        vec2 knob_scale = { 0.02, 1 };
        Color slider_color = Color(0x55, 0x44, 0x22, 0xff);
        Color knob_color = Color(0x88, 0xBB, 0xAA, 0xff);

        render_slider(m_ui.pan_slider, knob_scale, m_expr_audio.get_pan(), slider_color, knob_color, m_rendered_text.data[TEXT_PAN_VALUE]);
    }

    // pause/resume button
    {
        render_textured_rectangle(m_ui.pause_button,
            m_expr_audio.paused ? m_assets.resume_texture : m_assets.pause_texture,
            Color(0x66, 0x55, 0x55, 0xff)
        );
    }

    // paused/playing text
    {
        Rectangle pause_button = m_ui.pause_button;
        const int margin = 5;
        Font font = m_assets.font;
        render_text_size(m_window.renderer, font,
            m_rendered_text.data[(m_expr_audio.paused) ? TEXT_PAUSED : TEXT_PLAYING],
            vec2(pause_button.x, pause_button.y + pause_button.h + font.size/2));
    }

    // text field
    {
        SDL_SetRenderDrawColor(m_window.renderer, 0x66, 0x66, 0x55, 0xff);
		if (m_expr_audio.get_channel_count() == 1)
		{
			render_text_field(m_ui.expression_input_left);
		}
		else if (m_expr_audio.get_channel_count() == 2)
		{
			render_text_field(m_ui.expression_input_left);
			render_text_field(m_ui.expression_input_right);
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

    Text_Id text = m_audio_player.paused ? TEXT_RESUME : TEXT_PAUSE;
	render_textured_rectangle(Rectangle(0,0,100,100), m_rendered_text.get(text).texture, Color(0x22,0x55,0x33,0xff));

	if (m_audio_data.samples)
	{
		vec2 audio_data_scale = vec2(window_size.x * (5.0 / 8.0), window_size.y * (5.0 / 8.0));
		vec2 audio_data_position = vec2(window_size.x/2, window_size.y/2 + window_size.y/3);
		render_audio_data(audio_data_position, vec2(audio_data_scale.x, audio_data_scale.y), Color(0x44, 0x22, 0x77, 0xff));

		// playback position
		float playback_position = m_audio_player.playback_position / m_audio_player.audio_data.frame_count;
		float audio_data_start = audio_data_position.x - audio_data_scale.x / 2;
		draw_arrowhead(m_window.renderer, vec2(audio_data_start + audio_data_scale.x * playback_position, audio_data_position.y - audio_data_scale.y / 2), vec2(0, 1), 80, ColorF(0.4, 0.2, 0.7, 1.0));
	}

	// @todo spectogram
	{

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

    // event text
    {
        const vec2 text_scale = vec2(300, 100);

		// @todo split this into invalid for left and right
        if (m_events[EVENT_INVALID_EXPRESSION].active)
        {
            auto tf_area = m_ui.expression_input_left.m_area;
            render_text_size(m_window.renderer, m_assets.font, m_rendered_text.get(TEXT_INVALID_EXPRESSION),
                    vec2(tf_area.x + tf_area.w/2, tf_area.y + tf_area.h), text_scale);
        }
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
        render_text_scale(m_window.renderer, m_assets.font, text,
            vec2(slider.x + slider.w / 2, slider.y + slider.h * 2 + margin), vec2(0.6, 0.6));
    }
}

void Application::render_text_field(const Text_Field& text_field)
{
	Rectangle text_field_area = text_field.m_area;
	SDL_FRect tf_area = { text_field_area.x, text_field_area.y, text_field_area.w, text_field_area.h };
	SDL_RenderFillRect(m_window.renderer, &tf_area);

	SDL_Texture* text_texture = text_field.m_texture;
	if (text_texture)
	{
		int line_count = text_field.m_line_count;
		float font_size = m_assets.font.size;

		SDL_FRect string_area = { tf_area.x, tf_area.y, tf_area.w, line_count * font_size };
		SDL_FRect texture_area = { 0, 0, string_area.w, string_area.h };
		SDL_RenderTexture(m_window.renderer, text_texture, &texture_area, &string_area);
	}
}

void Application::render_dropdown(const Drop_Down_List& list, Color title_color, Color option_color) {
    SDL_SetRenderDrawColor(m_window.renderer, COLOR_ARG(title_color));

    SDL_FRect header_area = {
        list.pos.x - list.scale.x/2, list.pos.y - list.scale.y / 2,
        list.scale.x, list.scale.y
    };
    SDL_RenderFillRect(m_window.renderer, &header_area);
    render_text_size(m_window.renderer, m_assets.font, list.title,
        vec2(header_area.x + header_area.w / 2, header_area.y + header_area.h / 2), vec2(header_area.w, header_area.h));

    if (list.open) {
        SDL_SetRenderDrawColor(m_window.renderer, COLOR_ARG(option_color));

        for (int i = 0; i < list.options.size(); i++) {
            SDL_FRect area = header_area;
            area.y += area.h * (i + 1);
            SDL_RenderFillRect(m_window.renderer, &area);
            render_text_size(m_window.renderer, m_assets.font, list.get_option_label(i),
                vec2(area.x + area.w/2, area.y + area.h/2), vec2(area.w, area.h));
        }
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
		m_audio_player.toggle_pause();
		return true;
	}

	return false;
}

bool Application::mouse_input_sound_mode()
{
    if (m_ui.volume_slider.contains(m_mouse.pos))
    {
        float diff = m_mouse.pos.x - m_ui.volume_slider.x;
        float volume = diff / m_ui.volume_slider.w;

        volume = snap_value(volume, 0.0, 1.0, 0.06);

        m_expr_audio.set_volume(volume);

        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%.3f", volume);
        m_rendered_text.data[TEXT_VOLUME_VALUE].clear();
        m_rendered_text.data[TEXT_VOLUME_VALUE] = create_text(make_string(buffer), Color(0x54, 0x22, 0x77, 0xff));

        printf("%f\n", m_expr_audio.get_volume());
        return true;
    }

	if (m_ui.pan_slider.contains(m_mouse.pos))
	{
		float diff = m_mouse.pos.x - m_ui.pan_slider.x;
		float pan = diff / m_ui.pan_slider.w;

		pan = snap_value(pan, 0.0, 1.0, 0.04);

		m_expr_audio.set_pan(pan);

		char buffer[64];
		snprintf(buffer, sizeof(buffer), "%.3f", pan);
		m_rendered_text.data[TEXT_PAN_VALUE].clear();
		m_rendered_text.data[TEXT_PAN_VALUE] = create_text(make_string(buffer), Color(0x55, 0x22, 0x88, 0xff));

		printf("%s\n", buffer);
		return true;
	}

    if (m_ui.pause_button.contains(m_mouse.pos))
    {
        m_expr_audio.toggle_pause();

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
					m_expr_audio.set_playback_device(m_ui.playback_device.get_option_data_index(i));

                    got_clikcked = true;
                }
            }

            if (got_clikcked) {
                m_ui.playback_device.open = false;
                return true;
            }
        }
    }

    if (m_ui.expression_input_left.m_area.contains(m_mouse.pos))
    {
      const SDL_Rect area = {(int)m_ui.expression_input_left.m_area.x, (int)m_ui.expression_input_left.m_area.y, (int)m_ui.expression_input_left.m_area.w, (int)m_ui.expression_input_left.m_area.h};
        SDL_SetTextInputArea(m_window.window, &area, m_ui.expression_input_left.m_cursor_pixel);

        toggle_text_input();

        m_ui.text_input_target = EXPRESSION_INPUT_LEFT;

        return true;
    }


	if (m_expr_audio.get_channel_count() == 2)
	{
		if (m_ui.expression_input_right.m_area.contains(m_mouse.pos))
		{
			const SDL_Rect area = {(int)m_ui.expression_input_right.m_area.x, (int)m_ui.expression_input_right.m_area.y, (int)m_ui.expression_input_right.m_area.w, (int)m_ui.expression_input_right.m_area.h};
			SDL_SetTextInputArea(m_window.window, &area, m_ui.expression_input_right.m_cursor_pixel);

			toggle_text_input();

			m_ui.text_input_target = EXPRESSION_INPUT_RIGHT;

			return true;
		}
	}

    return false;
}

void Application::switch_modes() {
    if (mode == ApplicationMode::AppModeSound) {
        mode = ApplicationMode::AppModeGraph;

        m_expr_audio.pause();
    }
    else if (mode == ApplicationMode::AppModeGraph) {
        mode = ApplicationMode::AppModeSound;

		m_audio_player.pause();
    }
}

void Application::text_input_stop()
{
    SDL_StopTextInput(m_window.window);
    doing_text_input = false;

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
    m_ui.pause_button.x = (window_size.x - m_ui.pause_button.w) / 2;
    m_ui.pause_button.y = (window_size.y - m_ui.pause_button.h) / 2;

    m_ui.pan_slider.w = window_size.x * (5.0 / 8.0);
    m_ui.pan_slider.h = window_size.y * (1.0 / 32.0);
    m_ui.pan_slider.x = window_size.x * (1.0 / 2.0) - m_ui.pan_slider.w / 2;
    m_ui.pan_slider.y = 0;

    m_ui.graphs_button.x = window_size.x * (4.0 / 5.0);
    m_ui.graphs_button.y = window_size.y * (1.0 / 5.0);

	if (m_expr_audio.get_channel_count() == 1)
	{
		m_ui.expression_input_left.m_area.w = window_size.x * (1.0 / 3.0);
		m_ui.expression_input_left.m_area.h = window_size.y * (1.0 / 5.0);
		m_ui.expression_input_left.m_area.x = window_size.x / 2 - m_ui.expression_input_left.m_area.w / 2;
		m_ui.expression_input_left.m_area.y = window_size.y / 2 + m_ui.expression_input_left.m_area.h;
	}
	else if (m_expr_audio.get_channel_count() == 2)
	{
		m_ui.expression_input_left.m_area.w = window_size.x * (1.0 / 4.0);
		m_ui.expression_input_left.m_area.h = window_size.y * (1.0 / 5.0);
		m_ui.expression_input_left.m_area.x = window_size.x * (1.0 / 8.0);
		m_ui.expression_input_left.m_area.y = window_size.y / 2 + m_ui.expression_input_left.m_area.h;

		m_ui.expression_input_right.m_area.w = window_size.x * (1.0 / 4.0);
		m_ui.expression_input_right.m_area.h = window_size.y * (1.0 / 5.0);
		m_ui.expression_input_right.m_area.x = window_size.x * (5.0 / 8.0);
		m_ui.expression_input_right.m_area.y = window_size.y / 2 + m_ui.expression_input_right.m_area.h;
	}

    m_ui.channel_count.set_area(vec2(window_size.x / 2,
									 window_size.y * (1.0 / 5.0)),
								vec2((float)window_size.x * (1.0 / 5.0),
									 (float)window_size.y * (1.0 / 10.0))
								);

    m_ui.playback_device.set_area(vec2(window_size.x * (0.8 / 3.0),
									   window_size.y * (1.5 / 5.0)),
								  vec2((float)window_size.x * (1.0 / 5.0),
									   (float)window_size.y * (1.0 / 10.0))
								  );
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
	default:
		// error?
		return NULL;
	}
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
	{
        return false;
	}
    return text_field->update_text(m_window.renderer, m_assets.font,
								   (m_ui.text_input_target == EXPRESSION_INPUT_LEFT) || (m_ui.text_input_target == EXPRESSION_INPUT_RIGHT));
}


bool Application::set_eval_string(String eval_string)
{
    bool left =  st_sampler_set_expression(sampler_audio_left,  eval_string.data, eval_string.size);
    bool right = st_sampler_set_expression(sampler_audio_right, eval_string.data, eval_string.size);

    bool success = left && right;
    if (!success) {
        fprintf(stderr, "Failed to set sample expression\n");
    }

    sampler_waveform_left = st_sampler_copy(sampler_audio_left);
    sampler_waveform_right = st_sampler_copy(sampler_audio_right);

    ASSERT(sampler_waveform_left && sampler_waveform_right);

    st_sampler_set_sample_rate(sampler_waveform_left, WAVEFORM_SAMPLE_RATE);
    st_sampler_set_sample_rate(sampler_waveform_right, WAVEFORM_SAMPLE_RATE);

    return success;
}

bool Application::set_eval_string_left(String eval_string)
{
    bool success = st_sampler_set_expression(sampler_audio_left, eval_string.data, eval_string.size);
    if (!success)
    {
        fprintf(stderr, "Failed to set left sample expression\n");
    }

    sampler_waveform_left = st_sampler_copy(sampler_audio_left);
    ASSERT(sampler_waveform_left);
    st_sampler_set_sample_rate(sampler_waveform_left, WAVEFORM_SAMPLE_RATE);

    return success;
}

bool Application::set_eval_string_right(String eval_string)
{
    bool success = st_sampler_set_expression(sampler_audio_right, eval_string.data, eval_string.size);
    if (!success)
    {
        fprintf(stderr, "Failed to set right sample expression\n");
    }

    sampler_waveform_right = st_sampler_copy(sampler_audio_right);
    ASSERT(sampler_waveform_right);
    st_sampler_set_sample_rate(sampler_waveform_right, WAVEFORM_SAMPLE_RATE);

    return success;
}

// @todo we can do cooler things with rendering waveforms or audio data
// @todo do this rendering once, not every frame

void Application::render_audio_data(vec2 area_center, vec2 area_scale, Color color) {
	if (!(m_audio_data.samples && m_audio_data.is_in_desired_spec())) {
		return;
	}

    SDL_SetRenderDrawColor(m_window.renderer, COLOR_ARG(color));

    float* samples = (float*) m_audio_data.samples;

    #define BLOCK_SIZE 100
    static SDL_FPoint points[DESIRED_AUDIO_CHANNEL_COUNT][BLOCK_SIZE];

    float vertical_step = area_scale.y / DESIRED_AUDIO_CHANNEL_COUNT;
	float step = (area_scale.x / float(m_audio_data.frame_count));
	float base_height = area_center.y;
	int sample_count = m_audio_data.frame_count;

    int iter_count = m_audio_data.frame_count / BLOCK_SIZE;

    for (int block = 0; block < iter_count; block++)
	{
        int block_start = block * BLOCK_SIZE;

        for (int i = 0; i < BLOCK_SIZE; i+=1)
        {
            int frame_index = block * BLOCK_SIZE + i;
            int sample_index = (block_start + i) * DESIRED_AUDIO_CHANNEL_COUNT;

            for (int ch = 0; ch < DESIRED_AUDIO_CHANNEL_COUNT; ch++)
            {
                points[ch][i].x = area_center.x - (area_scale.x / 2) + (step * frame_index);
                points[ch][i].y = (base_height + samples[sample_index + ch] * (area_scale.y / 2.0)) / DESIRED_AUDIO_CHANNEL_COUNT + (ch * vertical_step);
            }
        }

        for (int ch = 0; ch < DESIRED_AUDIO_CHANNEL_COUNT; ch++)
        {
            SDL_RenderPoints(m_window.renderer, points[ch], BLOCK_SIZE);
        }
	}

    int remaining = m_audio_data.frame_count - (iter_count * BLOCK_SIZE);

    ASSERT(remaining < BLOCK_SIZE);

    for (int i = 0; i < remaining; i+=1)
    {
        int frame_index = iter_count * BLOCK_SIZE + i;
        int sample_index = (iter_count * BLOCK_SIZE + i) * DESIRED_AUDIO_CHANNEL_COUNT;

        for (int ch = 0; ch < DESIRED_AUDIO_CHANNEL_COUNT; ch++)
        {
            points[ch][i].x = area_center.x - (area_scale.x / 2) + (step * frame_index);
            points[ch][i].y = base_height + samples[sample_index + ch] * (area_scale.y / 2.0) + (ch * vertical_step);
        }
    }

    for (int ch = 0; ch < DESIRED_AUDIO_CHANNEL_COUNT; ch++)
    {
        SDL_RenderPoints(m_window.renderer, points[ch], remaining);
    }
}

void Application::render_waveform(St_Sampler* sampler, Array<SDL_FPoint> sample_buffer, vec2 area_center, vec2 area_scale) {
    int sample_count = sample_buffer.size;

    st_fill_strided(sampler, &sample_buffer.data[0].y, sample_count);

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


bool Application::load_audio_file(String path) {
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

        bool convert_success = SDL_ConvertAudioSamples(&spec, buffer, audio_length, &desired_spec, &output_buffer, &output_length);

        SDL_free(buffer);
        audio_length = 0;

        if (!convert_success)
        {
            fprintf(stderr, "Couldn't convert audio samples to desired spec. %s\n", SDL_GetError());
            return false;
        }
    }

    // @update
    m_audio_data.samples = output_buffer;
    m_audio_data.channel_count = desired_spec.channels;
    m_audio_data.format = desired_spec.format;
    m_audio_data.frequency = desired_spec.freq;
    m_audio_data.frame_count = output_length / (SDL_AUDIO_BYTESIZE(desired_spec.format) * desired_spec.channels);

    ASSERT(m_audio_data.is_in_desired_spec());

	// ---
	m_audio_player.set_audio_data(m_audio_data);

    return true;
}

bool Application::save_app_state(String filepath) {
	File file = File(filepath, "w");

    Save_State save;

	save.volume = m_expr_audio.get_volume();
	save.sample_rate = m_expr_audio.get_sample_rate();
	save.expression_left = m_ui.expression_input_left.get_string();
    save.expression_right = m_ui.expression_input_right.get_string();
	save.playback_device = m_ui.playback_device.get_selected_option_name();


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

	memcpy(buffer, save.playback_device.data, save.playback_device.size);
	buffer[save.playback_device.size] = '\0';
	fprintf(file.handle, "playback_device:%s\n", buffer);

	return true;
}

bool Application::load_app_state(String filepath) {
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
	String playback = {};

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
			volume = string_to_real(value);
		}
		else if (string_compare(name, String("sample_rate"))) {
			sample_rate = string_to_real(value);
		}
		else if (string_compare(name, String("expression_left"))) {
            expr_left = value;
        }
        else if (string_compare(name, String("expression_right"))) {
        	expr_right = value;
        }
		else if (string_compare(name, String("playback_device"))) {
			playback = value;
		}
		else {
			return false;
		}
	}

	save.volume = volume;
	save.sample_rate = sample_rate;
	save.expression_left = expr_left;
	save.expression_right = expr_right;
	save.playback_device = playback;

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

void render_text_size(SDL_Renderer* renderer, Font font, Text text, vec2 where, vec2 absolute_scale)
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

void render_text_scale(SDL_Renderer* renderer, Font font, Text text, vec2 where, vec2 scale_factor)
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
