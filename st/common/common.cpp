#include "common.h"

#include <cmath>
#include <array>


unsigned int pop_count(u64 x)
{
    x = (x & (u64)0x5555555555555555) + ((x >> 1)  & (u64)0x5555555555555555);
    x = (x & (u64)0x3333333333333333) + ((x >> 2)  & (u64)0x3333333333333333);
    x = (x & (u64)0x0F0F0F0F0F0F0F0F) + ((x >> 4)  & (u64)0x0F0F0F0F0F0F0F0F);
    x = (x & (u64)0x00FF00FF00FF00FF) + ((x >> 8)  & (u64)0x00FF00FF00FF00FF);
    x = (x & (u64)0x0000FFFF0000FFFF) + ((x >> 16) & (u64)0x0000FFFF0000FFFF);
    x = (x & (u64)0x00000000FFFFFFFF) + ((x >> 32) & (u64)0x00000000FFFFFFFF);
    return x;
}

NORETURN
void panic(char const* const msg)
{
    fprintf(stderr, "[PANIC]: %s\n", msg);
    exit(1);
}

float snap_value(float val, float bound1, float bound2, float threshold)
{
  if (fabsf(val - bound1) <= threshold) {
    val = bound1;
  }
  else if (fabsf(val - bound2) <= threshold) {
    val = bound2;
  }
  else if (fabsf(val - (bound1 + bound2) / 2) <= threshold) {
    val = (bound1 + bound2) / 2;
  }

  return val;
}

Color::Color(const ColorF& color) {
    float coef = 255.0;
    r = int(color.r * coef);
    g = int(color.g * coef);
    b = int(color.b * coef);
    a = int(color.a * coef);
}

ColorF::ColorF(const Color& color) {
    float coef = 1.0 / 255.0;
    r = (float)color.r * coef;
    g = (float)color.g * coef;
    b = (float)color.b * coef;
    a = (float)color.a * coef;
}


int pop_lsb(u64* x) {
    int index = TRAILING_ZEROS(*x);
    *x &= *x - 1;
    return index;
}

int pop_msb(u64* x) {
    int index = LEADING_ZEROS(*x);
    *x &= ~BIT(index);
    return index;
}

int string_length(const char* cstr) {
    return strlen(cstr);
}

String make_string(const char* s)
{
    int len = strlen(s);
    return { s, len };
}

bool string_compare(String s1, String s2)
{
    if (s1.size != s2.size) return false;
    for (int i = 0; i < s1.size; i++)
    {
        if (s1.data[i] != s2.data[i]) return false;
    }
    return true;
}

String string_slice(String s, int start, int end)
{
    return String { s.data + start, end - start };
}

String string_slice_to_character(String s, char c) {
    int cursor = 0;
    while (cursor < s.size && s.data[cursor] != c) {
        cursor += 1;
    }

    return String(s.data + cursor, s.size - cursor);
}

std::array<String, 2> string_cut_from_character(String s, char c) {
    int cursor = 0;
    while (cursor < s.size && s.data[cursor] != c) {
        cursor += 1;
    }

    String first;
    String second;

    first = String(s.data, cursor);
    if (cursor == s.size) {
        second = String(s.data + cursor, s.size - cursor);  // point at the en
    }
    else {
        second = String(s.data + cursor + 1, s.size - cursor - 1);
    }

    return std::array<String, 2>({first, second});
}

String string_get_extension(String s)
{
    for (int i = s.size - 1; i >= 0; i--)
    {
        if (s.data[i] == '.')
        {
            return string_slice(s, i, s.size);
        }
    }

    return String{NULL,0};
}

String string_copy(String s)
{
    char* data = (char*)malloc(s.size + 1);
    if (!data)
    {
        panic("Malloc fail");
    }
    memcpy(data, s.data, s.size);
    data[s.size] = '\0';
    return { data, s.size };
}

int string_to_integer(String s, bool* success)
{
    int accum = 0;
    int power = 1;
    for (int i = s.size - 1; i >= 0; i--)
    {
        if (!(s.data[i] >= '0' && s.data[i] <= '9'))
        {
            *success = false;
            return 0;
        }

        accum += (s.data[i] - '0') * power;

        power *= 10;
    }

    *success = true;
    return accum;
}

double string_to_real(String s, bool* success)
{
    char* end_ptr = NULL;
    double res = strtod(s.data, &end_ptr);
    if (end_ptr)
    {
        if (success)
            *success = false;
        return 0.0;
    }

    if (success)
        *success = true;
    return res;
}

long get_file_size(FILE* file) {
	long pos = ftell(file);
	fseek(file, 0, SEEK_END);
	long len = ftell(file);
	fseek(file, 0, SEEK_SET);
	return len;
}

bool load_file(const char* filepath, BinaryData& data) {
	FILE* handle = fopen(filepath, "r");

	auto filesize = get_file_size(handle);

	u8* mem = (u8*) malloc(filesize);
	if (!mem) {
		panic("malloc fail");
	}

	size_t written = fread(mem, sizeof(u8), filesize, handle);
	if (filesize != written) {
		free(mem);
		return false;
	}

	fclose(handle);

	data.data = mem;
	data.size = filesize;

	return true;
}

bool load_file_text(const char* filepath, String& s)
{
	BinaryData binary_data;
	if (!load_file(filepath, binary_data)) {
		return false;
	}

	s = String(binary_data);

	return true;
}

void File::write_string(String s) {
	fwrite(&s.size, sizeof(s.size), 1, handle);
	fwrite(s.data, sizeof(s.data[0]), s.size, handle);
}

void File::write_number(double n) {
	fwrite(&n, sizeof(n), 1, handle);
}

void File::write_integer(u64 n) {
	fwrite(&n, sizeof(n), 1, handle);
}

String File::read_string() {
	u32 size = 0;  // type must match String.size
	fread(&size, sizeof(size), 1, handle);

	char* data = (char*) malloc(size + 1);

	fread(data, sizeof(data[0]), size, handle);
	data[size] = '\0';

	return String(data, size);
}

double File::read_number() {
	double n = 0;
	fread(&n, sizeof(n), 1, handle);
	return n;
}

u64 File::read_integer() {
	u64 n = 0;
	fread(&n, sizeof(n), 1, handle);
	return n;
}

void String_Builder::create(int initial_capacity)
{
    buffer = (char*)malloc(initial_capacity);
    if (!buffer) panic("Malloc fail");
    buffer_capacity = initial_capacity;
    cursor = 0;
    buffer[0] = '\0';
}

String_Builder::String_Builder(int initial_capacity) {
    create(initial_capacity);
}

void String_Builder::remove(int amount)
{
    cursor = MAX(0, cursor - amount);
}

void String_Builder::remove_slice(int start, int end)
{
    if (start >= cursor || start >= end)
        return;

    if (end >= cursor)
        end = cursor;

    for (int i = end; i < cursor; i++)
    {
        buffer[start + i] = buffer[i];
    }

    cursor -= (end - start);
}

void String_Builder::resize() {
    char* nbuff = (char*)malloc(buffer_capacity * 2 * sizeof(char));
    if (!nbuff) panic("Malloc fail");
    if (buffer)
    {
        memcpy(nbuff, buffer, cursor);
        free(buffer);
    }
    buffer = nbuff;
    buffer_capacity *= 2;
}

int String_Builder::grow_to_size(int size) {
    int count = 0;
    while (size >= buffer_capacity) {
        resize();
        count++;
        if (count > 5) {
            fprintf(stderr, "String builder buffer resize failed repeatedly: Possible memory allocation issue or corrupted buffer state.\n"
                "Relevant: buffer_capacity: %d, cursor: %d, provided string size: %d",
                buffer_capacity, cursor, size);
            return 1;
        }
    }

    return 0;
}

void String_Builder::append(String string) {
    grow_to_size(cursor + string.size);

    memcpy(buffer + cursor, string.data, string.size);
    cursor += string.size;
}

void String_Builder::append_char(char ch) {
    grow_to_size(cursor + 1);

    buffer[cursor] = ch;
    cursor += 1;
}

void String_Builder::append_integer(int n)
{
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%d", n);
    append(make_string(buffer));
}

void String_Builder::append_hex(int n) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%x", n);
    append(make_string(buffer));
}

void String_Builder::append_float(float n) {
	char buffer[128];
	snprintf(buffer, sizeof(buffer), "%1.3f", n);
	append(make_string(buffer));
}

void String_Builder::clear_and_append(String s) {
    cursor = 0;
    append(s);
}

void String_Builder::append_many(String* strings, int n) {
    int total_length = 0;
    for (int i = 0; i < n; i++) {
        total_length += strings[i].size;
    }

    grow_to_size(this->cursor + total_length);
    for (int i = 0; i < n; i++) {
        memcpy(this->buffer + this->cursor, strings[i].data, strings[i].size);
        cursor += strings[i].size;
    }
}

const char* String_Builder::c_string() {
    this->buffer[this->cursor] = '\0';
    return this->buffer;
}

void String_Builder::free_buffer() {
    free(this->buffer);
    cursor = 0;
    buffer_capacity = 0;
    buffer = NULL;
}

void String_Builder::clear() {
    cursor = 0;
    buffer[0] = '\0';
}

String String_Builder::to_string()
{
    return String(buffer, cursor);
}

bool Rectangle::contains(vec2 p) const
{
    return p.x >= x && p.y >= y && p.x <= x + w && p.y <= y + h;
}

void String::trim() {
	while (size > 0 && is_space(data[size-1])) {
		size--;
	}

	while (size > 0 && is_space(data[0])) {
		size--;
		data++;
	}
}

void String::print(bool newline) const
{
    int size_nt = size + 1;
    char* mem = (char*)malloc(size_nt);
    if (!mem)
    {
        panic("Malloc fail");
    }
    memcpy(mem, data, size);
    mem[size] = '\0';
    if (newline)
    {
        printf("%s\n", mem);
    }
    else {
        printf("%s", mem);
    }
    free(mem);
}

bool String::operator==(const String& other) const
{
    return string_compare(*this, other);
}


float Complex::magnitude() const
{
    return sqrtf(real*real+imaginary*imaginary);
}

float Complex::winding() const
{
	return atan2f(imaginary, real);
}
