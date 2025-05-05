#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

size_t write_7bit_encoded_int(uint32_t value, char bytes[5]);
uint32_t read_7bit_encoded_int(const char* bytes, int* out_pos=nullptr);

#define UINT8_GETSET(name) \
    inline uint8_t get_##name() { return _##name; }\
    inline void set_##name(uint8_t val) {\
        _##name = val;\
        _dirty_flag |= (uint32_t)DirtyFlag::name;\
    }

#define UINT16_GETSET(name) \
    inline uint16_t get_##name() { return _##name; }\
    inline void set_##name(uint16_t val) {\
        _##name = val;\
        _dirty_flag |= (uint32_t)DirtyFlag::name;\
    }

#define UINT32_GETSET(name) \
    inline uint32_t get_##name() { return _##name; }\
    inline void set_##name(uint32_t val) {\
        _##name = val;\
        _dirty_flag |= (uint32_t)DirtyFlag::name;\
    }

#define INT_GETSET(name) \
    inline int get_##name() { return _##name; }\
    inline void set_##name(int val) {\
        _##name = val;\
        _dirty_flag |= (uint32_t)DirtyFlag::name;\
    }

#define FLOAT_GETSET(name) \
    inline float get_##name() { return _##name; }\
    inline void set_##name(float val) {\
        _##name = val;\
        _dirty_flag |= (uint32_t)DirtyFlag::name;\
    }

#define STR_GETSET(name) \
    inline const std::string& get_##name() { return _##name; }\
    inline void set_##name(const std::string& val) {\
        _##name = val;\
        _dirty_flag |= (uint32_t)DirtyFlag::name;\
    }