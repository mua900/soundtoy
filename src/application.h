#pragma once

#include "common.h"
#include "audio.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#define INIT_WINDOW_WIDTH 1440
#define INIT_WINDOW_HEIGHT 810

struct Window {
    SDL_Window* window;
    SDL_Renderer* renderer;
};

struct Font {
    TTF_Font* font = NULL;
    float size = 0;
};

struct Assets {
    Font font = {};

    SDL_Texture* pause_texture = NULL;
    SDL_Texture* resume_textrue = NULL;
};

typedef SDL_MouseButtonFlags Mouse_Flags;

struct Mouse_State {
    vec2 pos;
    Mouse_Flags flags;
};

struct Text_Field
{
    Rectangle area = {};
    String_Builder text = {};
    int cursor_character = 0;
    int cursor_pixel = 0;
    int line_count = 0;

    SDL_Texture* m_texture = NULL;  // cached texture the text is rendered on, updated every text input event

    String get_string()
    {
        return text.to_string();
    }
};

struct UiState {
    Rectangle volume_slider = { 100, 100, 100, 10 };
    vec2 volume_slider_knob_scale = { 0.1, 2 };

    Rectangle pause_button = { INIT_WINDOW_WIDTH / 2 - 50, INIT_WINDOW_HEIGHT / 2 - 50, 100, 100 };
    Text_Field text_field = { { INIT_WINDOW_WIDTH / 2 - 500, INIT_WINDOW_HEIGHT * (4.0 / 5.0) - 100, 1000, 200 } };

    void update(Window window);
};

#define DEFAULT_BACKGROUND_COLOR Color{ 0xaa, 0x66, 0x33, 0xff }

struct Application {
public:
    Window m_window = {};
    Mouse_State m_mouse = {};

    Assets m_assets = {};
    Audio m_audio = {};

    UiState m_ui = {};
    Color m_background_color = DEFAULT_BACKGROUND_COLOR;

    bool m_quit = false;
    bool doing_text_input = false;

    bool initialize();

    void handle_events();
    void update();
    void draw();
private:
    bool load_assets();

    void draw_ui();

    bool mouse_input_ui();

    void text_input_start();
    void text_input_stop();
    bool update_input_string(String s);
};
