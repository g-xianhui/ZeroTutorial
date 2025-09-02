#pragma once

#include <cstdint>
#include <string>

#define BOOL_GETSET(name) \
    inline bool get_##name() const { return _##name; }\
    inline void set_##name(bool val) {\
        _##name = val;\
        _dirty_flag |= (uint32_t)DirtyFlag::name;\
    }

#define UINT8_GETSET(name) \
    inline uint8_t get_##name() const { return _##name; }\
    inline void set_##name(uint8_t val) {\
        _##name = val;\
        _dirty_flag |= (uint32_t)DirtyFlag::name;\
    }

#define UINT16_GETSET(name) \
    inline uint16_t get_##name() const { return _##name; }\
    inline void set_##name(uint16_t val) {\
        _##name = val;\
        _dirty_flag |= (uint32_t)DirtyFlag::name;\
    }

#define UINT32_GETSET(name) \
    inline uint32_t get_##name() const { return _##name; }\
    inline void set_##name(uint32_t val) {\
        _##name = val;\
        _dirty_flag |= (uint32_t)DirtyFlag::name;\
    }

#define INT_GETSET(name) \
    inline int get_##name() const { return _##name; }\
    inline void set_##name(int val) {\
        _##name = val;\
        _dirty_flag |= (uint32_t)DirtyFlag::name;\
    }

#define FLOAT_GETSET(name) \
    inline float get_##name() const { return _##name; }\
    inline void set_##name(float val) {\
        _##name = val;\
        _dirty_flag |= (uint32_t)DirtyFlag::name;\
    }

#define STR_GETSET(name) \
    inline const std::string& get_##name() const { return _##name; }\
    inline void set_##name(const std::string& val) {\
        _##name = val;\
        _dirty_flag |= (uint32_t)DirtyFlag::name;\
    }

#define BOOL_PROPERTY(name) \
public:\
    BOOL_GETSET(name)\
private:\
    bool _##name

#define UINT8_PROPERTY(name) \
public:\
    UINT8_GETSET(name)\
private:\
    uint8_t _##name

#define UINT16_PROPERTY(name) \
public:\
    UINT16_GETSET(name)\
private:\
    uint16_t _##name

#define UINT32_PROPERTY(name) \
public:\
    UINT32_GETSET(name)\
private:\
    uint32_t _##name

#define INT_PROPERTY(name) \
public:\
    INT_GETSET(name)\
private:\
    int _##name

#define FLOAT_PROPERTY(name) \
public:\
    FLOAT_GETSET(name)\
private:\
    float _##name

#define STR_PROPERTY(name) \
public:\
    STR_GETSET(name)\
private:\
    std::string _##name

#define WRITE_IF_DIRTY(name) \
    if (_dirty_flag & (uint32_t)DirtyFlag::name)\
        bs.write(_##name)