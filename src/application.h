#pragma once

#include "common.h"
#include "audio.h"

#include <SDL3/SDL.h>

#define INIT_WINDOW_WIDTH 1440
#define INIT_WINDOW_HEIGHT 810

// @todo text rendering

struct Window {
    SDL_Window* window;
    SDL_Renderer* renderer;
};

struct Assets {
    // @todo shaders
    SDL_Texture* pause_texture;
    SDL_Texture* resume_textrue;
};

typedef SDL_MouseButtonFlags Mouse_Flags;

struct Mouse_State {
    vec2 pos;
    Mouse_Flags flags;
};

struct UiState {
    Rectangle volume_slider = {100, 100, 100, 10};
    vec2 volume_slider_knob_scale = {0.1, 2};

    Rectangle pause_button = {INIT_WINDOW_WIDTH / 2 - 50, INIT_WINDOW_HEIGHT / 2 - 50, 100, 100};
};

struct Application {
public:
    Window m_window = {};
    Mouse_State m_mouse = {};

    Assets m_assets = {};
    Audio m_audio = {};

    UiState m_ui = {};
    Color m_background_color = { 0xaa, 0x66, 0x33, 0xff };

    bool m_quit = false;

    bool initialize();

    void handle_events();
    void update();
    void draw();
private:
    bool load_assets();

    void draw_ui();

    bool mouse_input_ui();
};
