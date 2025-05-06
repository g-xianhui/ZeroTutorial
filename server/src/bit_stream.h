#pragma once

#include <cstddef>
#include <string>
#include <cassert>
#include <exception>
#include <vector>
#include <concepts>

class StreamException : public std::exception {
public:
    StreamException(const std::string& message) : message_(message) {}
    virtual const char* what() const noexcept override {
        return message_.c_str();
    }
private:
    std::string message_;
};

class OutputBitStream;
class InputBitStream;

template <typename T>
concept NetSerializable =
std::is_class_v<T> &&
    requires (T t, OutputBitStream & bs) {
        { t.net_serialize(bs) } -> std::same_as<void>;
        { t.net_delta_serialize(bs) } -> std::same_as<bool>;
};

class OutputBitStream {
public:
    OutputBitStream() = default;
    ~OutputBitStream() {
        if (m_buffer != nullptr) {
            std::free(m_buffer);
            m_capacity = 0;
            m_pos = 0;
            m_buffer = nullptr;
        }
    }
    OutputBitStream(const OutputBitStream&) = delete;
    OutputBitStream& operator=(const OutputBitStream&) = delete;
    OutputBitStream(OutputBitStream&& rhs) noexcept {
        m_pos = rhs.m_pos;
        rhs.m_pos = 0;
        m_capacity = rhs.m_capacity;
        rhs.m_capacity = 0;
        m_buffer = rhs.m_buffer;
        rhs.m_buffer = nullptr;
    }
    OutputBitStream& operator=(OutputBitStream&& rhs) noexcept {
        m_pos = rhs.m_pos;
        rhs.m_pos = 0;
        m_capacity = rhs.m_capacity;
        rhs.m_capacity = 0;
        m_buffer = rhs.m_buffer;
        rhs.m_buffer = nullptr;
        return *this;
    }

    inline size_t tellp() { return m_pos; }
    inline void seekp(size_t pos) { m_pos = pos; }

    inline const char* get_buffer() const { return m_buffer; }

    void write(const void* data, size_t bytes);
    void write(const std::string& str);

    template<NetSerializable T>
    void write(const T& data) {
        data.net_serialize(*this);
    }

    template<typename T>
        requires std::is_arithmetic_v<T>
    void write(T data) {
        write(&data, sizeof(T));
    }

    template<typename T>
    void write(const std::vector<T>& v) {
        size_t n = v.size();
        assert(n < 65536);
        write((unsigned short)n);
        for (auto& item : v) {
            write(item);
        }
    }

    template<typename T, std::size_t N>
    void write(const std::array<T, N>& v) {
        for (size_t i = 0; i < N; i++) {
            write(v[i]);
        }
    }

    template<typename T>
    void net_delta_serialize(T& data) {
        write(data);
    }

    template<NetSerializable T>
    void net_delta_serialize(T& data) {
        data.net_delta_serialize(*this);
    }

private:
    void realloc_buffer(size_t new_lenght);

private:
    char* m_buffer = nullptr;
    size_t m_pos = 0;
    size_t m_capacity = 0;
};

class InputBitStream {
public:
    InputBitStream(const char* data, size_t n) : m_buffer(data), m_capacity(n) {}

    inline bool is_end() { return m_pos >= m_capacity; }
    inline void bypass(size_t bytes) { m_pos += bytes; }
    inline size_t tellp() { return m_pos; }
    inline void seekp(size_t pos) { m_pos = pos; }
    inline const char* get_buffer() const { return m_buffer; }
    void check_overflow(size_t bytes);

    void read(void* dest, size_t bytes);
    void read(std::string& str);

    std::string read_string();

    template<NetSerializable T>
    T read() {
        T ref;
        ref.net_serialize(*this);
        return ref;
    }

    template<NetSerializable T>
    void read(T& ref) {
        ref.net_serialize(*this);
    }

    template<typename T>
        requires std::is_arithmetic_v<T>
    T read() {
        T ref;
        read(&ref, sizeof(T));
        return ref;
    }

    template<typename T>
        requires std::is_arithmetic_v<T>
    void read(T& ref) {
        read(&ref, sizeof(T));
    }

    template<typename T>
    void read(std::vector<T>& v) {
        unsigned short n = 0;
        read(n);
        v.resize(n);
        for (size_t i = 0; i < n; i++) {
            read(v[i]);
        }
    }

    template<typename T, std::size_t N>
    void read(std::array<T, N>& v) {
        for (size_t i = 0; i < N; i++) {
            read(v[i]);
        }
    }

private:
    const char* m_buffer = nullptr;
    size_t m_pos = 0;
    size_t m_capacity = 0;
};

struct OutputStreamGuard {
    size_t pos = 0;
    OutputBitStream* s = nullptr;

    OutputStreamGuard(OutputBitStream& stream) : s(&stream) {
        pos = stream.tellp();
    }

    void revert() {
        s->seekp(pos);
    }
};
