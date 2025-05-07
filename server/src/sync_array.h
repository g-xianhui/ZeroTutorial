#pragma once

#include <vector>
#include <cassert>

#include "bit_stream.h"

enum class SyncArrayOperation : uint8_t {
    update,
    push_back,
    pop_back,
    insert,
    erase,
    clear,
    resize,
    replace
};

template<typename T>
class TSyncArray {
public:
    using iterator = std::vector<T>::iterator;
    using const_iterator = std::vector<T>::const_iterator;
    using reference = std::vector<T>::reference;
    using const_reference = std::vector<T>::const_reference;
    using value_type = T;
    using size_type = size_t;

    TSyncArray() {}
    TSyncArray(size_type count) : _vec(count) {}
    TSyncArray(size_type count, const T& value = T()) : _vec(count, value) {}

    template< class InputIt >
    TSyncArray(InputIt first, InputIt last) : _vec(first, last) {}

    TSyncArray(const TSyncArray& other) : _vec(other._vec) {}
    TSyncArray(TSyncArray&& other) : _vec(std::move(other._vec)) {}

    TSyncArray(std::initializer_list<T> init) : _vec(init) {}

    TSyncArray& operator=(const TSyncArray& other) {
        _vec = other._vec;

        _dirty_log.write((uint8_t)SyncArrayOperation::replace);
        net_serialize(_dirty_log);

        return *this;
    }

    TSyncArray& operator=(TSyncArray&& other) {
        _vec = std::move(other._vec);

        _dirty_log.write((uint8_t)SyncArrayOperation::replace);
        net_serialize(_dirty_log);

        return *this;
    }

    TSyncArray& operator=(std::initializer_list<value_type> ilist) {
        _vec = ilist;

        _dirty_log.write((uint8_t)SyncArrayOperation::replace);
        net_serialize(_dirty_log);

        return *this;
    }

    iterator begin() {
        return _vec.begin();
    }

    const_iterator cbegin() const {
        return _vec.begin();
    }

    iterator end() {
        return _vec.end();
    }

    const_iterator cend() const {
        return _vec.end();
    }

    reference operator[](size_type pos) {
        return _vec[pos];
    }

    const_reference operator[](size_type pos) const {
        return _vec[pos];
    }

    bool empty() const {
        return _vec.empty();
    }

    size_type size() noexcept {
        return _vec.size();
    }

    void reserve(size_type new_cap) {
        _vec.reserve(new_cap);
    }

    size_type capacity() const {
        return _vec.capacity();
    }

    void clear() {
        _dirty_log.write((uint8_t)SyncArrayOperation::clear);

        _vec.clear();
    }

    iterator insert(const_iterator pos, const T& value) {
        _dirty_log.write((uint8_t)SyncArrayOperation::insert);
        size_t index = pos - begin();
        _dirty_log.write((uint16_t)index);
        _dirty_log.write(value);

        return _vec.insert(pos, value);
    }

    iterator insert(const_iterator pos, T&& value) {
        _dirty_log.write((uint8_t)SyncArrayOperation::insert);
        size_t index = pos - cbegin();
        _dirty_log.write((uint16_t)index);
        _dirty_log.write(value);

        return _vec.insert(pos, std::move(value));
    }

    iterator erase(iterator pos) {
        _dirty_log.write((uint8_t)SyncArrayOperation::erase);
        size_t index = pos - begin();
        _dirty_log.write((uint16_t)index);

        return _vec.erase(pos);
    }

    iterator erase(const_iterator pos) {
        _dirty_log.write((uint8_t)SyncArrayOperation::erase);
        size_t index = pos - cbegin();
        _dirty_log.write((uint16_t)index);

        return _vec.erase(pos);
    }

    void push_back(const T& value) {
        _dirty_log.write((uint8_t)SyncArrayOperation::push_back);
        _dirty_log.write(value);

        _vec.push_back(value);
    }

    void push_back(T&& value) {
        _dirty_log.write((uint8_t)SyncArrayOperation::push_back);
        _dirty_log.write(value);

        _vec.push_back(std::move(value));
    }

    template< class... Args >
    reference emplace_back(Args&&... args) {
        auto& ret = _vec.emplace_back(std::forward<Args...>(args...));

        _dirty_log.write((uint8_t)SyncArrayOperation::push_back);
        _dirty_log.write(ret);

        return ret;
    }

    void pop_back() {
        _dirty_log.write((uint8_t)SyncArrayOperation::pop_back);

        _vec.pop_back();
    }

    void resize(size_type count) {
        _dirty_log.write((uint8_t)SyncArrayOperation::resize);
        _dirty_log.write((uint16_t)count);

        _vec.resize(count);
    }

    void swap(TSyncArray& other) {
        _vec.swap(other._vec);

        _dirty_log.write((uint8_t)SyncArrayOperation::replace);
        net_serialize(_dirty_log);
    }

    void mark_dirty(size_type pos) {
        _dirty_log.write((uint8_t)SyncArrayOperation::update);
        _dirty_log.write((uint16_t)pos);
        _dirty_log.net_delta_serialize(_vec[pos]);
    }

    void mark_dirty(iterator pos) {
        mark_dirty(pos - begin());
    }

    void net_serialize(OutputBitStream& bs) const {
        bs.write(_vec);
    }

    bool net_delta_serialize(OutputBitStream& bs) {
        size_t dirty_size = _dirty_log.tellp();
        bs.write((uint32_t)dirty_size);
        if (dirty_size) {
            bs.write(_dirty_log.get_buffer(), dirty_size);
            _dirty_log.seekp(0);

            return true;
        }
        return false;
    }

    void net_delta_serialize(InputBitStream& bs) {
        uint32_t dirty_size = bs.read<uint32_t>();
        if (dirty_size) {
            size_t end_pos = bs.tellp() + dirty_size;
            while (bs.tellp() < end_pos) {
                SyncArrayOperation op = (SyncArrayOperation)bs.read<uint8_t>();
                switch (op) {
                case SyncArrayOperation::update:
                {
                    uint16_t pos = bs.read<uint16_t>();
                    _vec[pos] = bs.read<T>();
                }
                break;
                case SyncArrayOperation::push_back:
                {
                    _vec.push_back(bs.read<T>());
                }
                break;
                case SyncArrayOperation::pop_back:
                {
                    _vec.pop_back();
                }
                break;
                case SyncArrayOperation::insert:
                {
                    uint16_t pos = bs.read<uint16_t>();
                    _vec.insert(begin() + pos, bs.read<T>());
                }
                break;
                case SyncArrayOperation::erase:
                {
                    uint16_t pos = bs.read<uint16_t>();
                    _vec.erase(begin() + pos);
                }
                break;
                case SyncArrayOperation::resize:
                {
                    uint16_t new_size = bs.read<uint16_t>();
                    _vec.resize(new_size);
                }
                break;
                case SyncArrayOperation::replace:
                {
                    std::vector<T> new_vec;
                    bs.read(new_vec);
                    _vec = new_vec;
                }
                break;
                default:
                    assert(0);
                    break;
                }
            }
        }
    }

private:
    std::vector<T> _vec;
    OutputBitStream _dirty_log;
};