#include "ui.h"

void GapBuffer::initialize(int init_buffer_size) {
    buffer = new char[init_buffer_size];
    buffer_size = init_buffer_size;
    gap_index = 0;
    end_gap = init_buffer_size;
}

void GapBuffer::reset() {
    delete[] buffer;
    buffer = nullptr;
    buffer_size = 0;
    length = 0;
    gap_index = 0;
    end_gap = 0;
}

void GapBuffer::append(String string, int where)
{
    if (!(where <= length && where >= 0)) {
        return;
    }

    if (length + string.size > buffer_size) {
        resize(buffer_size * 2);
    }

    if (where != gap_index)
    {
        move_gap(where);
    }

    for (int i = 0; i < string.size; i++)
    {
        buffer[gap_index + i] = string.data[i];
    }

    length += string.size;
    gap_index += string.size;
}

void GapBuffer::remove(int where, int amount)
{
    if (where + amount > length)
    {
        amount = length - where;
    }

    if (!(where < length && where >= 0))
        return;

    move_gap(where);
    end_gap += amount;
    length -= amount;
}

char GapBuffer::get_character(int index)
{
    if (index < gap_index)
    {
        return buffer[index];
    }
    else
    {
        return buffer[end_gap + index - gap_index];
    }
}

void GapBuffer::move_gap(int position)
{
    if (!(position <= length && position >= 0)) {
        return;
    }

    int start = 0;
    int dest = 0;
    int amount = 0;
    if (position < gap_index) {
        amount = gap_index - position;
        start = position;
        dest = end_gap - amount;
    }
    else {
        amount = position - gap_index;
        start = end_gap;
        dest = gap_index;
    }

    memmove(buffer + dest, buffer + start, amount);

    int gap_size = end_gap - gap_index;
    gap_index = position;
    end_gap = gap_index + gap_size;
}

void GapBuffer::resize(int size)
{
    if (size < length) {
        return;  // failure
    }

    int start_chars = gap_index;
    int end_chars = buffer_size - end_gap;

    char* nbuffer = new char[size];
    memcpy(nbuffer, buffer, start_chars);
    memcpy(nbuffer + (size - end_chars), buffer + end_gap, end_chars);
    delete[] buffer;

    buffer = nbuffer;
    end_gap = size - end_chars;
    buffer_size = size;
}

void GapBuffer::get_string(String_Builder& sb)
{
	sb.clear_and_append(String(buffer, gap_index));
	sb.append(String(buffer + end_gap, length - gap_index));
}

void Text_Field::calculate_cursor_from_selection(String string, Font font, bool wrapped)
{
    int line_skip = TTF_GetFontLineSkip(font.font);

    // calculate cursor position
    int cursor_line = 0;
    int cursor_pixel_x = 0;
    size_t cursor_character = 0;

    Rectangle area = m_area;

    if (wrapped)
    {
        while (cursor_character < m_selection_start)
        {
            size_t cursor_character_this_line = 0;
            TTF_MeasureString(font.font, string.data + cursor_character, m_selection_start - cursor_character, area.w, &cursor_pixel_x, &cursor_character_this_line);

            if (cursor_character_this_line == 0)
            {
                break;
            }

            cursor_character += cursor_character_this_line;

            cursor_line += 1;
        }

        if (cursor_line)
        {
            cursor_line -= 1;  // 0 based indexing instead of 1 based indexing
        }

        int cursor_pixel_y = cursor_line * line_skip;

        m_cursor_line = cursor_line;
        m_cursor_pixel_x = cursor_pixel_x;
        m_cursor_pixel_y = cursor_pixel_y;
    }
    else {
        TTF_MeasureString(font.font, string.data, m_selection_start, MAX_INTEGER, &cursor_pixel_x, nullptr);

        m_cursor_line = 0;
        m_cursor_pixel_x = cursor_pixel_x;
        m_cursor_pixel_y = 0;
    }
}

size_t Text_Field::calculate_cursor_from_mouse(vec2 position, String string, Font font, bool wrapped)
{
    int line_skip = TTF_GetFontLineSkip(font.font);
    Rectangle area = m_area;
    int line_count = m_line_count;

    int cursor_line = position.y / line_skip;

    if (cursor_line >= line_count)
    {
        m_cursor_line = m_line_count - 1;
        return m_buffer.length;
    }

    size_t cursor_character = 0;
    int pixel_x = 0;
    int pixel_y = cursor_line * line_skip;

    if (wrapped)
    {
        // calculate what the lines above us add up to in character count
        // @todo which can maybe cached
        for (int i = 0; i < cursor_line; i++)
        {
            size_t cursor_character_this_line = 0;

            TTF_MeasureString(font.font, string.data + cursor_character, string.size - cursor_character, area.w, nullptr, &cursor_character_this_line);

            cursor_character += cursor_character_this_line;
        }
    }

    size_t last_line_character = 0;
    TTF_MeasureString(font.font, string.data + cursor_character, string.size - cursor_character, position.x, &pixel_x, &last_line_character);

    if (cursor_character + last_line_character < string.size)
    {
        int next_pixel_x = 0;

        TTF_MeasureString(font.font, string.data + cursor_character, last_line_character + 1, area.w, &next_pixel_x, NULL);

        if (position.x > (pixel_x + next_pixel_x) / 2)
        {
            last_line_character += 1;
            pixel_x = next_pixel_x;
        }
    }

    cursor_character += last_line_character;

    m_cursor_line = cursor_line;
    m_cursor_pixel_x = pixel_x;
    m_cursor_pixel_y = pixel_y;

    return cursor_character;
}
