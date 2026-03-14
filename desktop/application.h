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
    Font font_small = {};
    Font font_medium = {};
    Font font_large = {};
    Font font_editor = {};

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
    float pan = 0.0;
    String expression_left = {};
    String expression_right = {};

};

struct AudioContext {
    ExpressionAudio expr_audio = {};
    AudioPlayer audio_player = {};
    AudioData audio_data = {};
};

struct Buffers {
    // audio sample buffer used by the audio callback with a reference passed to ExpressionAudio
    Array<float> audio_samples = {};

    // recalculated regularly
    Array<SDL_FPoint> waveform_samples_left = {};
    Array<SDL_FPoint> waveform_samples_right = {};
};

struct Samplers {
    // owned by application referenced by ExpressionAudio
    St_Sampler* audio_left      = nullptr;
    St_Sampler* audio_right     = nullptr;

    St_Sampler* waveform_left   = nullptr;
    St_Sampler* waveform_right  = nullptr;
};

typedef float (*SampleGetter)(void* user, int frame, int channel);

class Application {
public:
    ApplicationMode mode = ApplicationMode::AppModeSound;

    Window m_window = {};
    Mouse_State m_mouse = {};

    Assets m_assets = {};
    AudioContext m_audio;

    Buffers m_buffers;

    Samplers m_samplers;

    Spectogram spectogram = {};
    Signal m_signal = {};

    GraphToShow m_graph_to_show = GRAPH_AUDIO_DATA;

    Ui_State m_ui = {};
    Color m_background_color = DEFAULT_BACKGROUND_COLOR;

    s64 m_time = 0;
    double m_time_seconds = 0;

    Event_Timeout m_events[EVENT_COUNT] = {};

    Array<Text> m_rendered_text = {};
    Text m_error_message = {};

    DArray<Text> m_variable_text = {};          // user registered variables

    bool quit = false;
    bool doing_text_input = false;

    bool initialize();

    void handle_events();
    void update();
    void draw();

    void cleanup();
private:
    void timeout();
    void update_ui_state(vec2 window_size);
    void set_event_active(int event_index, double timeout_seconds);
    void set_event_deactive(int event_index);

    bool load_assets();
    bool st_load_assets(String_Builder& sb);  // helper

    void draw_common_ui();
    void draw_sound_mode_ui();
    void draw_graph_mode_ui();

    bool mouse_input();
    bool mouse_input_common();
    bool mouse_input_graph_mode();
    bool mouse_input_sound_mode();

    bool keyboard_input_common(SDL_KeyboardEvent keyboard);
    bool keyboard_input_sound_mode(SDL_KeyboardEvent keyboard);
    bool keyboard_input_graph_mode(SDL_KeyboardEvent keyboard);

    bool gen_static_text(Color color);

    bool save_app_state(String filepath);
    bool load_app_state(String filepath);

    bool load_audio_file(String path);

    bool save_ui_layout(String filepath);
    bool load_ui_layout(String filepath);

    bool update_channel_count(int count);

    bool reinit_samplers();

    void text_input_start();
    void text_input_stop();
    void toggle_text_input();
    bool update_input_string();
    void update_waveform(St_Sampler* sampler, Array<SDL_FPoint> sample_buffer, vec2 area_center, vec2 area_scale);

    void switch_modes();

    Text create_text(String text, Font font, Color color);
    void destroy_text(Text& text);

    bool set_eval_string(String s);
    bool set_eval_string_left(String s);
    bool set_eval_string_right(String s);
    bool select_playback_device(SDL_AudioDeviceID device);

    void render_waveform(vec2 area_center, vec2 area_scale, int frame_count, int channel_count, Color color, SampleGetter sample_getter, void* user_data, bool draw_lines);

    void render_audio_data(vec2 area_center, vec2 area_scale, Color color);
    void render_signal(vec2 area_center, vec2 area_scale, Signal signal, Color color);

    void render_textured_rectangle(Rectangle rect, SDL_Texture* texture, Color color);

    void render_slider(Rectangle area, vec2 knob_scale, float value, Color slider_color, Color knob_color, const Text& text);
    void render_text_field(const Text_Field& text_field);
    void render_dropdown(const Drop_Down_List& list, Color title_color, Color option_color);

    void clear_text_input_selection();
};

bool load_font(Font* font, String_Builder& path, String font_folder, String font_file, float size);

void render_text_size(SDL_Renderer* renderer, Text text, vec2 where, vec2 absolute_scale = vec2(0, 0));
void render_text_scale(SDL_Renderer* renderer, Text text, vec2 where, vec2 scale_factor = vec2(0,0));

// polygon drawing
void draw_arrowhead(SDL_Renderer* renderer, vec2 position, vec2 direction, float scale, ColorF color);
void draw_plus(SDL_Renderer* renderer, vec2 position, vec2 scale, float thickness, ColorF color);

// create a signal from the given sampler
Signal create_signal(St_Sampler* sampler, float time_start, int sample_count, int sample_rate);

float get_signal_sample(void* user, int frame, int channel);  // Signal*
float get_audio_sample(void* data, int frame, int channel);   // AudioData*