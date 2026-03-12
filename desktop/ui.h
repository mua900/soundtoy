#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "common.h"
#include "template.h"

#define INIT_WINDOW_WIDTH  1440.0f
#define INIT_WINDOW_HEIGHT 810.0f

#define DEFAULT_BACKGROUND_COLOR Color{ 0x88, 0x33, 0x66, 0xff }

struct Text {
    SDL_Texture* texture = NULL;
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

struct GapBuffer {
    char* buffer = nullptr;
    int buffer_size = 0;
    int length = 0;
    int gap_index = 0;
    int end_gap = 0;

    GapBuffer() {
        initialize(256);
    }
    ~GapBuffer() {
        reset();
    }

    void initialize(int init_buffer_size);
    void append(String string, int where);
    void remove(int where, int amount);
    char get_character(int index);
    void move_gap(int position);
    void resize(int size);
    void get_string(String_Builder& sb);

    void reset();
};

enum Text_Input_Target : u8 {
    NO_TARGET,
    EXPRESSION_INPUT_LEFT,
    EXPRESSION_INPUT_RIGHT,
    VARIABLE_NAME,
    VARIABLE_VALUE,
};

struct Text_Field
{
    Rectangle m_area = {};
    GapBuffer m_buffer = {};
    String_Builder m_text = {};
    int m_cursor_pixel_x = 0;
    int m_cursor_pixel_y = 0;
    int m_cursor_line = 0;
    int m_line_count = 0;

    int m_selection_start = 0;
    int m_selection_end = 0;

    float m_font_size = 0.0;
    SDL_Texture* m_texture = nullptr;  // cached texture the text is rendered on, updated every text input event

    Text_Field() {}

    Text_Field(Rectangle area)
    {
        m_area = area;
    }

    Text_Field(Text_Field&& other) = default;
    Text_Field& operator=(Text_Field&& other) = default;

    String get_string()
    {
        m_buffer.get_string(m_text);
        return m_text.to_string();
    }

    void append_string(String s)
    {
        if (m_selection_start != m_selection_end)
        {
            m_buffer.remove(m_selection_start, m_selection_end - m_selection_start);
            m_buffer.append(s, m_selection_start);
        }
        else
        {
            m_buffer.append(s, m_selection_start);
            m_selection_start += s.size;
        }

        m_selection_end = m_selection_start;
    }

    bool update_text(SDL_Renderer* renderer, Font font, bool wrapped)
    {
        return render_text_field_texture(renderer, font, Color { 0x11, 0x22, 0x11, 0xff }, wrapped);
    }

    void clear() {
        m_buffer.remove(0, m_buffer.length);
        SDL_DestroyTexture(m_texture);
        m_texture = nullptr;
        m_cursor_pixel_x = 0;
        m_cursor_pixel_y = 0;
        m_cursor_line = 0;
        m_line_count = 0;
        m_selection_start = 0;
        m_selection_end = 0;
        m_font_size = 0;
    }

    void reset()
    {
        clear();
        m_buffer.reset();
        m_text.free_buffer();
    }

    void delete_text()
    {
        if (m_selection_end < m_selection_start)
            return;
        int amount = m_selection_end - m_selection_start;
        m_buffer.remove(m_selection_start, amount);

        m_selection_end = m_selection_start;
    }

    void delete_at_cursor()
    {
        if (m_selection_start != m_selection_end)
            return;
        if (m_selection_start == 0)
            return;

        m_selection_end = m_selection_start;
        m_selection_start -= 1;

        delete_text();
    }

    void delete_after_cursor()
    {
        if (m_selection_start != m_selection_end)
            return;
        if (m_selection_start == m_buffer.length)
            return;

        m_selection_end = m_selection_start + 1;

        delete_text();
    }

    void delete_at_character(int character)
    {
        if (character >= m_buffer.length)
            return;

        m_selection_start = character;
        m_selection_end = character + 1;
        delete_text();
    }

    void set_text_input_area(SDL_Window* window, int line_skip)
    {
        const SDL_Rect area = { area.x, area.y + m_cursor_line * line_skip, area.w, line_skip};
        SDL_SetTextInputArea(window, &area, m_cursor_pixel_x);
    }

    // both of these assume wrapped text
    void calculate_cursor_from_selection(String string, Font font);
    size_t calculate_cursor_from_mouse(vec2 mouse_position, String string, Font font);

private:
    bool render_text_field_texture(SDL_Renderer* renderer, Font font, Color color, bool wrapped);
};


#define DROP_DOWN_LIST_SELECTED_SENTINEL -1

struct Drop_Down_List {
	// owns the text object inside it
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
        title.clear();
        for (auto entry : options)
        {
            entry.label.clear();
        }
        options.reset();
    }
};

enum GraphToShow {
    GRAPH_AUDIO_DATA,
    GRAPH_FOURIER_TRANSFORM,
};

struct Ui_State {
    Text_Input_Target text_input_target = NO_TARGET;

    // graph mode ui
    Rectangle playback_pause = {0,0,100,100};
    Drop_Down_List graph_to_show = {};

    // sound mode ui
    Rectangle volume_slider = { 100, 100, 100, 10 };

    Text_Field variable_name = {};
    Rectangle add_variable_button = { 500, 100, 50, 50 };

    Rectangle pause_button = { INIT_WINDOW_WIDTH / 2 - 50, INIT_WINDOW_HEIGHT / 2 - 50, 100, 100 };
    Rectangle graphs_button = { INIT_WINDOW_WIDTH * (4.0 / 5.0), 0, INIT_WINDOW_WIDTH * (1.0 / 5.0), 100 };

    Text_Field expression_input_left = {};
    Text_Field expression_input_right = {};

    DArray<Text_Field> variable_values = {};
    int selected_variable_value_index = 0;  // if a variable value from the list is selected

    Drop_Down_List channel_count = {};
    Drop_Down_List playback_device = {};

    Text_Field* get_selected_text_field();
};
