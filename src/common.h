#pragma once

#include <cstdint>
#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdarg>

#define IS_MAX_UNSIGNED(x) ((x)+1==0)
#define BIT(x) (1U << (x))

#ifdef _MSC_VER
#include <intrin.h>

#define NORETURN __declspec(noreturn)

#else

#define NORETURN __attribute__((noreturn))

#endif

NORETURN
void panic(char const* const msg);

#define NOT_IMPLEMENTED(x) panic(x " not implemeneted");

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

struct String {
    const char* data = NULL;
    int size = 0;

    String () {}
    String (const char* d, int s) : data(d), size(s) {}

    void print();
};

#define STRING_EMPTY ((String){.data=NULL,.size=0})
#define CSTRING_LENGTH(s) (sizeof(s)-1)
#define MAKE_STRING(s) (String){.data=s,.size=CSTRING_LENGTH(s)}

String make_string(const char* s);
bool string_compare(String s1, String s2);
String string_slice(String s, int start, int end);
String string_get_extension(String s);
String string_copy(String s);

struct ivec2 {
    int x, y;
};

struct vec2 {
    float x, y;
};

struct Rectangle {
    float x, y, w, h;

    bool contains(vec2 p);
};

struct Color {
    unsigned char r, b, g, a;
};

#define COLOR_WHITE ((Color){0xff,0xff,0xff,0xff})
#define COLOR_BLACK ((Color){0,0,0,0xff})
#define COLOR_RED   ((Color){0xff,0,0,0xff})
#define COLOR_GREEN ((Color){0,0xff,0,0xff})
#define COLOR_BLUE  ((Color){0,0,0xff,0xff})

#define COLOR_ARG(color) color.r,color.g,color.b,color.a

enum Log_Level {
    Log_Level_Message,
    Log_Level_Warning,
    Log_Level_Error,
};

void log_log(enum Log_Level ll, char const* const msg, ...);

#define LOG_ERROR(msg)    log_log(Log_Level_Error, msg)
#define LOG_WARNING(msg)  log_log(Log_Level_Warning, msg)
#define LOG_MSG(msg)      log_log(Log_Level_Message, msg)
#define LOG_ERRORF(msg, ...)    log_log(Log_Level_Error, msg, __VA_ARGS__)
#define LOG_WARNINGF(msg, ...)  log_log(Log_Level_Warning, msg, __VA_ARGS__)
#define LOG_MSGF(msg, ...)      log_log(Log_Level_Message, msg, __VA_ARGS__)

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define MIN(x,y) (((x) > (y)) ? (y) : (x))
#define MAX(x,y) (((x) > (y)) ? (x) : (y))

struct String_Builder {
    char* buffer = NULL;
    int buffer_capacity = 0;
    int cursor = 0;

    String_Builder() {
        create(128);
    }

    String_Builder(int initial_capacity);

    void create(int initial_capacity);
    void append(String string);
    void append_char(char ch);
    const char* c_string();
    void remove(int amount);  // remove the last n characters from the buffer
    void clear_and_append(String s);
    void append_many(String* strings, int n);
    void free_buffer();
    void clear();
    String to_string();
private:
    void resize();
    int grow_to_size(int size);
};

static inline bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static inline bool is_alpha_lower(char c)
{
    return c >= 'a' && c <= 'z';
}

static inline bool is_alpha_upper(char c)
{
    return c >= 'A' && c <= 'Z';
}

static inline bool is_alpha(char c)
{
    return is_alpha_lower(c) || is_alpha_upper(c);
}

static inline bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n';
}

static inline char to_lower_ascii(char c)
{
    return is_alpha_upper(c) ? (c - 'A' + 'a') : (c);
}

#define BOOL_STRING(b) ((b) ? ("true") : ("false"))
