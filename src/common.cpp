#include "common.h"

NORETURN
void panic(char const* const msg)
{
    fprintf(stderr, "[PANIC]: %s\n", msg);
    exit(1);
}

// @todo fix
void log_log(enum Log_Level ll, char const* const msg, ...)
{
    char buff[1024];
    va_list vargs;
    va_start(vargs, msg);
    vsnprintf(buff, sizeof(buff), msg, vargs);
    va_end(vargs);
    switch (ll)
    {
    case Log_Level_Message:
        fprintf(stderr, "[MSG]: %s\n", buff); break;
    case Log_Level_Warning:
        fprintf(stderr, "[WARNING]: %s\n", buff); break;
    case Log_Level_Error:
        fprintf(stderr, "[ERROR]: %s\n", buff); break;
    default:
        fprintf(stderr, "%s\n", buff); break;
    }
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
    char* data = (char*)malloc(s.size);
    if (!data)
    {
        panic("Malloc fail");
    }
    memcpy(data, s.data, s.size);
    return { data, s.size };
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

bool Rectangle::contains(vec2 p)
{
    return p.x >= x && p.y >= y && p.x <= x + w && p.y <= y + h;
}

void String::print()
{
    String_Builder sb(size);
    sb.append(*this);
    printf("%s\n", sb.c_string());
    sb.free_buffer();
}
