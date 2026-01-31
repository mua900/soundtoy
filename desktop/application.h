#pragma once

#include "common.h"
#include "template.h"
#include "audio.h"
#include "api.h"
#include "ui.h"
#include "spectogram.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

enum ApplicationMode {
    AppModeSound,
    AppModeGraph,
};

struct Window {
    SDL_Window* window;
    SDL_Renderer* renderer;
};

struct Assets {
    // @todo multiple font sizes so that we can choose between them
    Font font = {};

    SDL_Texture* pause_texture = NULL;
    SDL_Texture* resume_texture = NULL;
};

typedef SDL_MouseButtonFlags Mouse_Flags;

struct Mouse_State {
    vec2 pos;
    Mouse_Flags flags;
};

#define NS_PER_SECONDS 1'000'000'000

struct Event_Timeout {
    s64 event = 0;
    bool active = false;
};

enum Events {
    EVENT_INVALID_EXPRESSION,
	EVENT_INVALID_SAMPLE_RATE,
    EVENT_RECALCULATE_WAVEFORM_SAMPLES,
    EVENT_COUNT,
};

struct Save_State {
	float volume = 0.0;
	float sample_rate = 0.0;
	String expression = {};
	String playback_device = {};
};

class Application {
public:
    ApplicationMode mode = ApplicationMode::AppModeSound;

    Window m_window = {};
    Mouse_State m_mouse = {};

    Assets m_assets = {};
    ExpressionAudio m_expr_audio = {};
    AudioPlayer m_audio_player = {};
    AudioData m_audio_data = {};

    // audio sample buffer populated by the audio callback with a reference passed to ExpressionAudio
	Array<float> sample_buffer = {};

    // recalculated regularly
	Array<SDL_FPoint> waveform_sample_buffer_left = {};
	Array<SDL_FPoint> waveform_sample_buffer_right = {};

	St_Sampler* sampler_audio_left      = nullptr;
	St_Sampler* sampler_audio_right     = nullptr;
	St_Sampler* sampler_waveform_left   = nullptr;
	St_Sampler* sampler_waveform_right  = nullptr;

	Spectogram spectogram = {};
	
    Ui_State m_ui = {};
    Color m_background_color = DEFAULT_BACKGROUND_COLOR;

    s64 m_time = 0;
    double m_time_seconds = 0;

    Event_Timeout m_events[EVENT_COUNT] = {};

    Array<Text> m_rendered_text = {};

    float m_volume = 0.0;

    bool quit = false;
    bool doing_text_input = false;

    bool initialize();

    void handle_events();
    void update();
    void draw();

    void cleanup();
private:
    void timeout();
    void set_event_active(int event_index, double timeout_seconds);
    void set_event_deactive(int event_index);

    bool load_assets();
    bool st_load_assets(String_Builder& sb);  // helper

    void draw_common_ui();
    void draw_sound_mode_ui();
    void draw_graph_mode_ui();
    void draw_imgui();

    bool mouse_input();
	bool mouse_input_common();
	bool mouse_input_graph_mode();
	bool mouse_input_sound_mode();

    bool gen_static_text(Color color);

	bool save_app_state(String filepath);
	bool load_app_state(String filepath);

    bool load_audio_file(String path);

	bool save_ui_layout(String filepath);
	bool load_ui_layout(String filepath);

    bool update_channel_count(int count);

    void text_input_start();
    void text_input_stop();
    void toggle_text_input();
    bool update_input_string();

    void switch_modes();

    Text create_text(String text, Color color);

    bool set_eval_string(String s);
    bool select_playback_device(SDL_AudioDeviceID device);

	void render_audio_data(vec2 area_center, vec2 area_scale, Color color);
	
    void render_textured_rectangle(Rectangle rect, SDL_Texture* texture, Color color);

    void render_slider(Rectangle area, vec2 knob_scale, float value, Color slider_color, Color knob_color, const Text& text);
    void render_waveform(St_Sampler* sampler, Array<SDL_FPoint> sample_buffer, vec2 area_center, vec2 area_scale);
    void render_dropdown(const Drop_Down_List& list, Color title_color, Color option_color);
};

void render_text_size(SDL_Renderer* renderer, Font font, Text text, vec2 where, vec2 absolute_scale = vec2(0, 0));
void render_text_scale(SDL_Renderer* renderer, Font font, Text text, vec2 where, vec2 scale_factor = vec2(0,0));

void draw_arrowhead(SDL_Renderer* renderer, vec2 position, vec2 direction, float scale, ColorF color);
