#pragma once

#include <cstdint>
#include <cstddef>

size_t write_7bit_encoded_int(uint32_t value, char bytes[5]);
uint32_t read_7bit_encoded_int(const char* bytes, int* out_pos=nullptr);

#define UINT8_GETSET(name) \
    inline uint8_t get_##name() { return _##name; }\
    inline void set_##name(uint8_t val) {\
        _##name = val;\
        _dirty_flag |= (uint32_t)DirtyFlag::name;\
    }

#define INT_GETSET(name) \
    inline int get_##name() { return _##name; }\
    inline void set_##name(int val) {\
        _##name = val;\
        _dirty_flag |= (uint32_t)DirtyFlag::name;\
    }
