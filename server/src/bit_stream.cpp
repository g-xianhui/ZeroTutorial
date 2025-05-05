#include "bit_stream.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cassert>

void OutputBitStream::realloc_buffer(size_t new_lenght) {
    void* new_buffer = std::realloc(m_buffer, new_lenght);
    if (new_buffer == nullptr) {
        throw std::bad_alloc();
    }
    else {
        m_buffer = static_cast<char*>(new_buffer);
        m_capacity = new_lenght;
    }
}

void OutputBitStream::write(const void* data, size_t bytes) {
    if (m_pos + bytes > m_capacity) {
        realloc_buffer(std::max(m_pos + bytes, m_capacity * 2));
    }
    std::memcpy(m_buffer + m_pos, data, bytes);
    m_pos += bytes;
}

void OutputBitStream::write(const std::string& str) {
    size_t n = str.size();
    assert(n < 65536);
    write((unsigned short)n);
    write(str.c_str(), n);
}

void InputBitStream::check_overflow(size_t bytes)
{
    if (m_pos + bytes > m_capacity) {
        char buffer[128];
        int n = std::snprintf(buffer, sizeof(buffer), "read out of range, want %zu bytes, left space: %zu", bytes, m_capacity - m_pos);
        std::string s{ buffer, (size_t)n };
        throw StreamException(s);
    }
}

void InputBitStream::read(void* dest, size_t bytes) {
    check_overflow(bytes);
    std::memcpy(dest, m_buffer + m_pos, bytes);
    m_pos += bytes;
}

void InputBitStream::read(std::string& str) {
    uint16_t bytes = read<uint16_t>();
    check_overflow(bytes);
    str.assign(m_buffer + m_pos, bytes);
    m_pos += bytes;
}

std::string InputBitStream::read_string()
{
    uint16_t bytes = read<uint16_t>();
    check_overflow(bytes);
    std::string ret{ m_buffer + m_pos, bytes };
    m_pos += bytes;
    return ret;
}
