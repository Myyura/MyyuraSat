/**
 * A simple region-based memory allocator.
 */

#ifndef _MYYURASAT_ALLOC_H
#define _MYYURASAT_ALLOC_H

#include <stdexcept>
#include <new>
#include <cstdlib>
#include <cstdint>

namespace MyyuraSat {

template<class T>
class RegionAllocator {
private:
    T *_memory;
    std::uint32_t _size;
    std::uint32_t _capacity;
    std::uint32_t _wasted;

    void _realloc(uint32_t min_cap);

public:
    const std::size_t UNIT_SIZE = sizeof(T);

    using RARef = std::uint32_t;
    const RARef RAREF_UNDEF = UINT32_MAX;

    explicit RegionAllocator(std::uint32_t start_cap = 1024 * 1024):
        _memory(NULL), _size(0), _capacity(0), _wasted(0) {
        _realloc(start_cap);
    }

    ~RegionAllocator(void) { if (_memory != NULL) { std::free(_memory); } }

    std::uint32_t size(void) const { return _size; }
    std::uint32_t wasted(void) const { return _wasted; }

    RARef alloc (int size);
    void free(int size) { _wasted += size; }

    // Deref, Load Effective Address (LEA), Inverse of LEA (AEL):
    T& operator[](RARef r) {
        if (r >= _size) { 
            throw std::out_of_range("RegionAllocator<T>::operator[] : index is out of range");
        }

        return _memory[r];
    }
    const T& operator[](RARef r) const {
        if (r >= _size) { 
            throw std::out_of_range("RegionAllocator<T>::operator[] : index is out of range");
        }

        return _memory[r];
    }

    T *lea(RARef r) {
        if (r >= _size) { 
            throw std::out_of_range("RegionAllocator<T>::lea() : index is out of range");
        }

        return &_memory[r];
    }
    const T *lea(RARef r) const {
        if (r >= _size) { 
            throw std::out_of_range("RegionAllocator<T>::lea() : index is out of range");
        }

        return &_memory[r];
    }
    RARef ael (const T* t) {
        // Modified >= to >
        if ((void *)t < (void *)&_memory[0] || (void *)t > (void *)&_memory[_size - 1]) {
            throw std::out_of_range("RegionAllocator<T>::ael() : index is out of range");
        }

        return (RARef)(t - &_memory[0]);
    }

    void move_to(RegionAllocator& to) {
        if (to._memory != NULL) { std::free(to._memory); }
        to._memory = _memory;
        to._size = _size;
        to._capacity = _capacity;
        to._wasted = _wasted;

        _memory = NULL;
        _size = _capacity = _wasted = 0;
    }
};

template<class T>
void RegionAllocator<T>::_realloc(uint32_t min_cap) {
    if (_capacity >= min_cap) { return; }

    uint32_t prev_cap = _capacity;
    for (; _capacity < min_cap;) {
        /**
         * NOTE: Multiply by a factor (13/8) without causing overflow, then 
         * add 2 and make the result even by clearing the least significant 
         * bit. The resulting sequence of capacities is carefully chosen to 
         * hit a maximum capacity that is close to the '2^32-1' limit when
         * using 'uint32_t' as indices so that as much as possible of this 
         * space can be used.
         */
        uint32_t delta = ((_capacity >> 1) + (_capacity >> 3) + 2) & ~1;
        _capacity += delta;

        if (_capacity <= prev_cap) {
            throw std::bad_alloc();
        }
    }

    _memory = (T *)std::realloc(_memory, UNIT_SIZE * _capacity);
    if (_memory == NULL) { throw std::bad_alloc(); }
}

template<class T>
typename RegionAllocator<T>::RARef RegionAllocator<T>::alloc(int size) {
    if (size <= 0) {
        throw std::invalid_argument("RegionAllocator<T>::alloc(int size) : the argument size must be greater than 0");
    }

    _realloc(_size + size);

    uint32_t prev_size = _size;
    _size += size;

    // Handle overflow
    if (_size < prev_size) { throw std::bad_alloc(); }

    return prev_size;
}

};

#endif