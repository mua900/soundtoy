#pragma once

#include "common.h"
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

struct Audio {
    SDL_AudioDeviceID playback = 0;
    SDL_AudioStream* audio_stream = NULL;

    double time = 0;
    int sample_rate = 0;

    float volume = 0;
    bool paused = true;

    void pause();
    void unpause();
};

struct UiState {
    Rectangle volume_slider = {100, 100, 100, 10};
    vec2 volume_slider_knob_scale = {0.1, 2};

    Rectangle pause_button = {INIT_WINDOW_WIDTH / 2 - 50, INIT_WINDOW_HEIGHT / 2 - 50, 100, 100};
};

struct Application {
public:
    Window window;
    Mouse_State mouse;

    Assets assets;
    Audio audio;

    UiState ui;

    bool quit;

    void handle_events();
    void update();
    void draw();
private:
    bool load_assets();

    void draw_ui();

    bool mouse_input_ui();
};

bool initialize(Application* app);
