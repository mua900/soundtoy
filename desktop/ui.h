#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "common.h"
#include "template.h"

#define INIT_WINDOW_WIDTH  1440.0f
#define INIT_WINDOW_HEIGHT 810.0f

#define DEFAULT_BACKGROUND_COLOR Color{ 0x88, 0x33, 0x66, 0xff }

struct Text {
    SDL_Texture* texture = NULL;  // @todo make ownership of this more clear
    String string = {};

    Text() {}
    Text(SDL_Texture* p_texture, String p_string) : texture(p_texture), string(p_string) {}

    void clear()
    {
        if (texture)
        {
            SDL_DestroyTexture(texture);
            texture = nullptr;
        }
    }
};

enum Text_Id : int {
    // static text
    TEXT_PAUSED = 0,
    TEXT_PLAYING,
    TEXT_PAUSE,
    TEXT_RESUME,
    TEXT_SAMPLE_RATE,
    TEXT_INVALID_EXPRESSION,
    TEXT_VALID_EXPRESSION,
    TEXT_INVALID_SAMPLE_RATE,
    TEXT_SOUND_MODE,
    TEXT_GRAPH_MODE,

    // dynamic text
    TEXT_VOLUME_VALUE,
    TEXT_PAN_VALUE,

    TEXT_COUNT,
};

struct Font {
    TTF_Font* font = NULL;
    float size = 0;
};

enum Text_Input_Target : u8 {
    NO_TARGET,
    EXPRESSION_INPUT_LEFT,
    EXPRESSION_INPUT_RIGHT,
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

    void append_string(String s)
    {
        m_text.append(s);
    }

    bool update_text(SDL_Renderer* renderer, Font font, bool wrapped)
    {
        return render_text_field_texture(renderer, get_string(), font, wrapped);
    }

    void delete_text()
    {
        m_text.remove_slice(m_selection_start, m_selection_end);
    }

    void delete_last()
    {
        m_text.remove(1);
    }

private:
    bool render_text_field_texture(SDL_Renderer* renderer, String s, Font font, bool wrapped);
};


#define DROP_DOWN_LIST_SELECTED_SENTINEL -1

// @todo fix possible memory leaks

struct Drop_Down_List {
    struct Entry {
        Text label = {};
        union {
            void* data;
            int index;
        };

        Entry() : label(), data(nullptr) {}
        Entry(Text text, void* p_data) : label(text), data(p_data) {}
        Entry(Text text, int p_index) : label(text), index(p_index) {}
    };

    vec2 pos = {};
    vec2 scale = {};
    int selected = DROP_DOWN_LIST_SELECTED_SENTINEL;
    Text title = {};
    DArray<Entry> options = {};
    bool open = false;

    void toggle() {
        open = !open;
    }

    void set_area(vec2 p_pos, vec2 p_scale) {
        pos = p_pos; scale = p_scale;
    }

    void set_title(Text text) {
        title = text;
    }

    void add_option(Text text, void* data) {
        options.add(Entry(text, data));
    }

    void add_option(Text text, int index) {
        options.add(Entry(text, index));
    }

    Text get_option_label(int index) const {
        return options.get(index).label;
    }

    String get_option_name(int index) const {
        return options.get(index).label.string;
    }

    String get_selected_option_name() const {
        if (selected == DROP_DOWN_LIST_SELECTED_SENTINEL)
        {
            return String();
        }

        return get_option_name(selected);
    }

    void* get_option_data(int index) const {
        return options.get(index).data;
    }

    int get_option_data_index(int index) const {
        return options.get(index).index;
    }

    void remove_option(int index) {
        if (index == selected)
        {
            selected = DROP_DOWN_LIST_SELECTED_SENTINEL;
        }
        options.get_ref(index).label.clear();
        options.remove_shift(index);
    }

    Drop_Down_List() {}
    Drop_Down_List(vec2 p_pos, vec2 p_scale) : pos(p_pos), scale(p_scale) {}
    ~Drop_Down_List() {
        options.reset();
    }
};

struct Ui_State {
    // graph mode ui
    Rectangle playback_pause = {0,0,100,100};

    Drop_Down_List graph_to_show = {};

    // sound mode ui
    Rectangle volume_slider = { 100, 100, 100, 10 };
    Rectangle pan_slider = {
        INIT_WINDOW_WIDTH * (1.0 / 2.0) - INIT_WINDOW_WIDTH * (5.0 / 16.0), INIT_WINDOW_HEIGHT * (1.0 / 5.0) - INIT_WINDOW_HEIGHT * (1.0 / 32.0),
        INIT_WINDOW_WIDTH * (5.0 / 8.0), INIT_WINDOW_HEIGHT * (1.0 / 16.0) };

    Rectangle pause_button = { INIT_WINDOW_WIDTH / 2 - 50, INIT_WINDOW_HEIGHT / 2 - 50, 100, 100 };
    Rectangle graphs_button = { INIT_WINDOW_WIDTH * (4.0 / 5.0), 0, INIT_WINDOW_WIDTH * (1.0 / 5.0), 100 };

    Text_Input_Target text_input_target = NO_TARGET;
    Text_Field expression_input_left = {};
    Text_Field expression_input_right = {};

    Drop_Down_List channel_count = {};
    Drop_Down_List playback_device = {};

    Text_Field* get_selected_text_field();
};
