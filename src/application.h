#pragma once

#include "common.h"
#include "audio.h"
#include "evaluator.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#define INIT_WINDOW_WIDTH 1440.0
#define INIT_WINDOW_HEIGHT 810.0

struct Window {
    SDL_Window* window;
    SDL_Renderer* renderer; // @todo custom renderer maybe?
};

struct Font {
    TTF_Font* font = NULL;
    float size = 0;
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

struct Text_Field
{
    Rectangle m_area = {};
    String_Builder m_text = {};
    int m_cursor_character = 0;
    int m_cursor_pixel = 0;
    int m_line_count = 0;

    int m_selection_start = 0;
    int m_selection_end = 0;

    SDL_Texture* m_texture = nullptr;  // cached texture the text is rendered on, updated every text input event

    String get_string()
    {
        return m_text.to_string();
    }

    void delete_text()
    {
        m_text.remove_slice(m_selection_start, m_selection_end);
    }

    void delete_last()
    {
        m_text.remove(1);
    }

    bool render_text_field_texture(SDL_Renderer* renderer, String s, Font font, bool wrapped);
};


enum Text_Input_Target : u8 {
    TEXT_INPUT_TEXT_FIELD,
    TEXT_INPUT_SAMPLE_RATE,
};

struct UiState {
    Rectangle m_volume_slider = { 100, 100, 100, 10 };
    vec2 m_volume_slider_knob_scale = { 0.1, 2 };

    Rectangle m_pause_button = { INIT_WINDOW_WIDTH / 2 - 50, INIT_WINDOW_HEIGHT / 2 - 50, 100, 100 };
    Text_Field m_text_field = { { INIT_WINDOW_WIDTH / 2 - 500, INIT_WINDOW_HEIGHT * (4.0 / 5.0) - 100, 1000, 200 } };
    Text_Field m_sample_rate_box = { { INIT_WINDOW_WIDTH * (3.0 / 5.0), INIT_WINDOW_HEIGHT * (1.0 / 5.0),
                                INIT_WINDOW_WIDTH / 5.0, INIT_WINDOW_HEIGHT / 5.0 } };

    Text_Input_Target m_text_input_target = TEXT_INPUT_TEXT_FIELD;

    void update(Window window);
    Text_Field* get_selected_text_field();
};

#define DEFAULT_BACKGROUND_COLOR Color{ 0x88, 0x66, 0x33, 0xff }

struct Text {
    SDL_Texture* texture = NULL;
    String string = {};

    Text() {}
    Text(SDL_Texture* p_texture, String p_string) : texture(p_texture), string(p_string) {}
};

enum {
    // static text
    TEXT_TEXT_PAUSED = 0,
    TEXT_TEXT_PLAYING,
    TEXT_INVALID_EXPRESSION,
    TEXT_VALID_EXPRESSION,

    // dynamic text
    TEXT_VOLUME_VALUE,

    TEXT_COUNT,
};

#define NS_PER_SECONDS 1'000'000'000

// remaining ticks (nanoseconds) for events that will have to stay alive for a certain time
struct Event_Timeout {
    s64 event = 0;
    bool active = false;
};

enum Events {
    EVENT_INVALID_EXPRESSION,
    EVENT_COUNT,
};

class Application {
public:
    Window m_window = {};
    Mouse_State m_mouse = {};

    Assets m_assets = {};
    Audio m_audio = {};

    UiState m_ui = {};
    Color m_background_color = DEFAULT_BACKGROUND_COLOR;

    Evaluator m_evaluator = {};

    s64 m_time = 0;
    double m_time_seconds = 0;

    Event_Timeout m_events[EVENT_COUNT] = {};

    Array<Text> m_rendered_text_cache = {};

    bool quit = false;
    bool doing_text_input = false;
    bool input_valid = false;

    DArray<String> m_error_log = {};

    bool initialize();

    void handle_events();
    void update();
    void draw();

    void cleanup();
private:
    void timeout();
    void set_event_active(int event_index, double timeout_time);

    bool load_assets();

    void draw_ui();

    bool mouse_input_ui();

    bool gen_static_text(Color color);
    bool gen_text(Color color);

    void update_audio_spec();

    void set_volume(float volume);

    void text_input_start();
    void text_input_stop();
    void toggle_text_input();
    bool update_input_string();

    Text create_text(String text, Color color);
    void render_text_at(Text text, vec2 center);  // @todo

    bool set_eval_string(String s);
};

void render_text(SDL_Renderer* renderer, Font font, Text text, vec2 where, vec2 scale = vec2(0, 0));