#pragma once

#include "common.h"
#include "template.h"
#include "audio.h"
#include "api.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#define INIT_WINDOW_WIDTH  1440.0f
#define INIT_WINDOW_HEIGHT 810.0f

struct Window {
    SDL_Window* window;
    SDL_Renderer* renderer;
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

struct Text {
    SDL_Texture* texture = NULL;
    String string = {};

    Text() {}
    Text(SDL_Texture* p_texture, String p_string) : texture(p_texture), string(p_string) {}
};

enum Text_Id : int {
    // static text
    TEXT_PAUSED = 0,
    TEXT_PLAYING,
    TEXT_SAMPLE_RATE,
    TEXT_INVALID_EXPRESSION,
    TEXT_VALID_EXPRESSION,
	TEXT_INVALID_SAMPLE_RATE,

    // dynamic text
    TEXT_VOLUME_VALUE,

    TEXT_COUNT,
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

struct Drop_Down_List {
	struct Entry {
		Text label = {};
		union {
			void* data;
			int index;
		};

		Entry() : label(), data(nullptr) {}
		Entry(Text p_label, void* p_data) : label(p_label), data(p_data) {}
		Entry(Text p_label, int p_data) : label(p_label), index(p_data) {}
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
		return get_option_name(selected);
	}

	void* get_option_data(int index) const {
		return options.get(index).data;
	}

	int get_option_data_index(int index) const {
		return options.get(index).index;
	}
	
    void remove_option(int index) {
        options.remove_shift(index);
    }

    Drop_Down_List() {}
    Drop_Down_List(vec2 p_pos, vec2 p_scale) : pos(p_pos), scale(p_scale) {}
    ~Drop_Down_List() {
        options.reset();
    }
};

enum Text_Input_Target : u8 {
    TEXT_INPUT_TEXT_FIELD,
    TEXT_INPUT_SAMPLE_RATE,
};

struct Ui_State {
    Rectangle volume_slider = { 100, 100, 100, 10 };
    vec2 volume_slider_knob_scale = { 0.1, 2 };

    Rectangle pause_button = { INIT_WINDOW_WIDTH / 2 - 50, INIT_WINDOW_HEIGHT / 2 - 50, 100, 100 };
    Text_Field input_text_field = { { INIT_WINDOW_WIDTH / 2 - 500, INIT_WINDOW_HEIGHT * (4.0 / 5.0) - 100, 1000, 200 } };
    Text_Field sample_rate_box = { { INIT_WINDOW_WIDTH * (3.0 / 5.0), INIT_WINDOW_HEIGHT * (1.0 / 5.0), INIT_WINDOW_WIDTH / 5.0, INIT_WINDOW_HEIGHT / 5.0 } };

    Drop_Down_List channel_count = {};
    Drop_Down_List playback_device = {};

    Text_Input_Target text_input_target = TEXT_INPUT_TEXT_FIELD;

    void update(Window window);
    Text_Field* get_selected_text_field();
};

#define DEFAULT_BACKGROUND_COLOR Color{ 0x88, 0x33, 0x66, 0xff }

#define NS_PER_SECONDS 1'000'000'000

struct Event_Timeout {
    s64 event = 0;
    bool active = false;
};

enum Events {
    EVENT_INVALID_EXPRESSION,
	EVENT_INVALID_SAMPLE_RATE,
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
    Window m_window = {};
    Mouse_State m_mouse = {};

    Assets m_assets = {};
    Audio m_audio = {};

	Array<float> sample_buffer = {};  // audio sample buffer
	Array<SDL_FPoint> waveform_sample_buffer = {};

	St_Sampler* sampler_audio_left      = nullptr;
	St_Sampler* sampler_audio_right     = nullptr;
	St_Sampler* sampler_waveform_left   = nullptr;
	St_Sampler* sampler_waveform_right  = nullptr;
	
    Ui_State m_ui = {};
    Color m_background_color = DEFAULT_BACKGROUND_COLOR;

    s64 m_time = 0;
    double m_time_seconds = 0;

    Event_Timeout m_events[EVENT_COUNT] = {};

    Array<Text> m_rendered_text_cache = {};

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

    void draw_ui();
    void draw_imgui();

    bool mouse_input_ui();

    bool gen_static_text(Color color);

	bool save_app_state(String filepath);
	bool load_app_state(String filepath);

	bool save_ui_layout(String filepath);
	bool load_ui_layout(String filepath);

    void update_audio_spec();
    bool update_channel_count(int count);

    void text_input_start();
    void text_input_stop();
    void toggle_text_input();
    bool update_input_string();

    Text create_text(String text, Color color);

    bool set_eval_string(String s);
    bool select_playback_device(SDL_AudioDeviceID device);

    void render_waveform(St_Sampler* sampler, vec2 area_center, vec2 area_scale, Color waveform_color);
    void render_dropdown(const Drop_Down_List& list, Color title_color, Color option_color);
};

void render_text(SDL_Renderer* renderer, Font font, Text text, vec2 where, vec2 scale = vec2(0, 0));
