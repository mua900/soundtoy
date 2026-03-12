#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <array>

#define IS_MAX_UNSIGNED(x) ((x)+1==0)
#define BIT(x) (1U << (x))

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;


unsigned int pop_count(u64 x);

#ifdef _MSC_VER

#include <intrin.h>

#define NORETURN __declspec(noreturn)

static inline unsigned int msvc_trailing_zeros(u64 x)
{
    unsigned long pos = 0;
    unsigned char is_zero = _BitScanForward64(&pos, x);
    // @note no checking for zero here since we assume non-zero input.
    return pos;
}

static inline unsigned int msvc_leading_zeros(u64 x)
{
    unsigned long pos = 0;
    unsigned char is_zero = _BitScanReverse64(&pos, x);
    // @note no checking for zero here since we assume non-zero input.
    return pos;
}

#define POP_COUNT(x)      pop_count(x)
#define LEADING_ZEROS(x)  msvc_leading_zeros(x)
#define TRAILING_ZEROS(x) msvc_trailing_zeros(x)

#else // _MSC_VER

#define NORETURN __attribute__((noreturn))

#define POP_COUNT(x)      __builtin_popcountll(x)
#define LEADING_ZEROS(x)  __builtin_clzll(x)
#define TRAILING_ZEROS(x) __builtin_ctzll(x)

#endif

#define ASSERT(x)   do {    \
        if (!(x)) {             \
            fprintf(stderr, "-----*****----- Assertion failed at %s:%d   %s\n", __FILE__, __LINE__, #x); \
            exit(1);    \
        }               \
    } while(0)


NORETURN
void panic(char const* const msg);

#define NOT_IMPLEMENTED(x) panic(x " not implemeneted");

int pop_lsb(u64* x);
int pop_msb(u64* x);

struct BinaryData {
	u8* data = nullptr;
	size_t size = 0;

    ~BinaryData() {
        if (data) {
            free(data);

            data = nullptr;
            size = 0;
        }
    }

	void release() {
		if (data) {
			free(data);
			data = nullptr;
		}
	}
};

struct Find_Result {
	int index = 0;
	bool found = false;

    Find_Result(int index, bool found) : index(index), found(found) {}
    Find_Result() {}
};

int string_length(const char* cstr);

struct String {
    const char* data = NULL;
    int size = 0;

    String () {}
	explicit String (const char* d) : data(d), size(string_length(d)) {}
    String (const char* d, int s) : data(d), size(s) {}
	String (const BinaryData& b) : data((const char*)b.data), size(b.size) {}

    bool operator==(const String& other) const;
    void print(bool newline = false) const;
	void trim();
};

#define STRING_EMPTY ((String){.data=NULL,.size=0})
#define CSTRING_LENGTH(s) (sizeof(s)-1)
#define MAKE_STRING(s) (String){.data=s,.size=CSTRING_LENGTH(s)}  // not used

#define SCOPE_STRING_EXP(p_s, p_name, p_size)				\
	char p_name[p_size];  \
	memcpy(p_name, p_s.data, p_s.size);			\
	p_name[p_s.size] = '\0';

#define SCOPE_STRING(str, name) SCOPE_STRING_EXP(str, name, 256)

String make_string(const char* s);
bool string_compare(String s1, String s2);
String string_slice(String s, int start, int end);
String string_slice_to_character(String s, char c);
std::array<String, 2> string_cut_from_character(String s, char c);
String string_get_extension(String s);
String string_copy(String s);

int string_to_integer(String s, bool* success);
double string_to_real(String s, bool* success);

struct String_Builder {
    char* buffer = NULL;
    int buffer_capacity = 0;
    int cursor = 0;

    String_Builder() {
        create(128);
    }

    String_Builder(int initial_capacity);

    ~String_Builder() {
        if (buffer) {
            free(buffer);
        }
    }

    void create(int initial_capacity);
    void append(String string);
    void append_char(char ch);
    void append_integer(int n);
    void append_hex(int n);
	void append_float(float n);
    const char* c_string();
    void remove(int amount);  // remove the last n characters from the buffer
    void remove_slice(int start, int end);
    void clear_and_append(String s);
    void append_many(String* strings, int n);
    void free_buffer();
    void clear();
    String to_string();
private:
    void resize();
    int grow_to_size(int size);
};

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define MIN(x,y) (((x) > (y)) ? (y) : (x))
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define CLAMP(x, lower, upper) (MIN(upper, MAX(x, lower)))

bool load_file(const char* filepath, BinaryData& bdata);
bool load_file_text(const char* filepath, String& s);

long get_file_size(FILE* file);

struct File {
	FILE* handle = nullptr;

    File(FILE* handle) : handle(handle) {}
	File(String filepath, const char* access) {
		SCOPE_STRING(filepath, buffer);

		handle = fopen(buffer, access);
	}
	~File() {
		fclose(handle);
	}

	void write_string(String s);
	void write_number(double n);
	void write_integer(u64 n);

	String read_string();
	double read_number();
	u64 read_integer();
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

float snap_value(float val, float bound1, float bound2, float threshold);

struct ivec2 {
    int x, y;
};

struct vec2 {
    float x = 0, y = 0;
    vec2() {}
    vec2(float p_x, float p_y) : x(p_x), y(p_y) {}

    vec2 normalized() const
    {
        float mag = sqrt(x*x+y*y);
        return vec2(x/mag,y/mag);
    }
};

inline vec2 operator+(const vec2 a, const vec2 b)
{
    return vec2(a.x + b.x, a.y + b.y);
}
inline vec2 operator-(const vec2 a, const vec2 b)
{
    return vec2(a.x - b.x, a.y - b.y);
}

struct Rectangle {
    float x, y, w, h;

    Rectangle() {}
    Rectangle(vec2 pos, vec2 scale) : x(pos.x), y(pos.y), w(scale.x), h(scale.y) {}
    Rectangle(float p_x, float p_y, float p_w, float p_h)
        : x(p_x), y(p_y), w(p_w), h(p_h)
    {}

    bool contains(vec2 p) const;
    // the current rectangle assumes center to be the origin so shift it to be the top left corner
    Rectangle center() const {
        return Rectangle(x - w / 2, y - h / 2, w, h);
    }
};

struct ColorF;

struct Color {
    unsigned char r, g, b, a;
    Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) : r(r), g(g), b(b), a(a) {}
    Color(const ColorF& color);
};

struct ColorF {
    float r, g, b, a;
    ColorF(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
    ColorF(const Color& color);
};

#define COLOR_WHITE ((Color){0xff,0xff,0xff,0xff})
#define COLOR_BLACK ((Color){0,0,0,0xff})
#define COLOR_RED   ((Color){0xff,0,0,0xff})
#define COLOR_GREEN ((Color){0,0xff,0,0xff})
#define COLOR_BLUE  ((Color){0,0,0xff,0xff})

#define COLOR_ARG(color) color.r,color.g,color.b,color.a

// simple custom complex number
struct Complex {
	float real = 0.0;
	float imaginary = 0.0;

	Complex() {}
    Complex(float r, float i) : real(r), imaginary(i) {}

    float magnitude() const;
    float winding() const;
};

// overloads for complex

inline Complex operator+(const Complex lhs, const Complex rhs)
{
	return Complex(lhs.real + rhs.real, lhs.imaginary + rhs.imaginary);
}

inline Complex operator-(const Complex lhs, const Complex rhs)
{
	return Complex(lhs.real - rhs.real, lhs.imaginary - rhs.imaginary);
}

inline Complex operator*(const Complex lhs, const Complex rhs)
{
	return Complex(lhs.real * rhs.real - lhs.imaginary * rhs.imaginary, lhs.real * rhs.imaginary + lhs.imaginary * rhs.real);
}

#define CONSTANT_PI  3.14159265359
#define CONSTANT_E   2.71828182846
#define CONSTANT_TAU (CONSTANT_PI * 2.0)
