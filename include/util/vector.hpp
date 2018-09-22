/**
 * Automatically resizable arrays
 * 
 * NOTE! Don't use this Vector on datatypes that cannot be re-located in 
 * memory (with realloc)
 */

#ifndef _MYYURASAT_VECTOR_H
#define _MYYURASAT_VECTOR_H

#include <limits>
#include <new>
#include <stdexcept>
#include <cstdlib>

#include "alloc.hpp"

namespace MyyuraSat {

template<class T, class _Size = int>
class Vector {
public:
    using SizeType = _Size;

private:
    T* _data;
    SizeType _size;
    SizeType _capacity;

    // Don't allow copying (error prone):
    Vector<T>& operator=(Vector<T>& rhs);

    Vector(Vector<T>& vec);

public:
    // Constructors:
    Vector(void): _data(NULL), _size(0), _capacity(0) {}

    explicit Vector(SizeType size): _data(NULL), _size(0), _capacity(0) { grow_to(size); }

    Vector(SizeType size, const T& pad): _data(NULL), _size(0), _capacity(0) { grow_to(size, pad); }

    Vector(std::initializer_list<T> l): _data(NULL), _size(0), _capacity(0) {
        grow_to(l.size());
        SizeType i = 0;
        for (auto& it : l) { _data[i++] = it; }
    }

    ~Vector(void) { clear(true); }

    // Pointer to first element:
    operator T*(void) { return _data; }

    // Size operations:
    SizeType size(void) const { return _size; }

    bool empty(void) const { return _size == 0; }

    void shrink(SizeType nelems) {
        if (nelems > _size) { throw std::length_error("Vector<T>::shrink : out of size"); }

        for (SizeType i = 0; i < nelems; i++) { _data[--_size].~T(); }
    }

    void shrink_lazy(SizeType nelems) {
        if (nelems > _size) { throw std::length_error("Vector<T>::shrink : out of size"); }

        _size -= nelems;
    }

    int capacity(void) const { return _capacity; }

    void reserve(SizeType min_cap) {
        if (_capacity >= min_cap) return;
        SizeType add = std::max((min_cap - _capacity + 1) & ~1, ((_capacity >> 1) + 2) & ~1);
        // NOTE: grow by approximately 3/2
        const SizeType size_max = std::numeric_limits<SizeType>::max();
        if (((size_max <= std::numeric_limits<int>::max()) && (add > size_max - _capacity))
        ||  (((_data = (T*)std::realloc(_data, (_capacity += add) * sizeof(T))) == NULL))) { throw std::bad_alloc(); }
    }

    void grow_to(SizeType size) {
        if (_size >= size) { return; }
        reserve(size);
        for (SizeType i = _size; i < size; i++) { new (&_data[i]) T(); }
        _size = size;
    }

    void grow_to(SizeType size, const T& pad) {
        if (_size >= size) { return; }
        reserve(size);
        for (SizeType i = _size; i < size; i++) { _data[i] = pad; }
        _size = size;
    }

    void clear(bool dealloc = false) {
        if (_data != NULL) {
            for (SizeType i = 0; i < _size; i++) { _data[i].~T(); }
            _size = 0;
            if (dealloc) {
                free(_data);
                _data = NULL;
                _capacity = 0;
            }
        }
    }

    // Stack interface:
    void push(void) { 
        if (_size == _capacity) { reserve(_size + 1); }
        new (&_data[_size++]) T();
    }

    void push(const T& elem) {
        if (_size == _capacity) { reserve(_size + 1); }
        new (&_data[_size++]) T(elem);
        // _data[_size++] = elem;
    }

    // void push(const T&& elem) {
    //     if (_size == _capacity) { reserve(_size + 1); }
    //     _data[_size++] = std::move(elem);
    // }

    void insert(const T& elem) {
        if (_size >= _capacity) { throw std::length_error("Vector<T>::insert : out of capacity"); }

        _data[_size++] = elem;
    }

    void pop(void) {
        if (_size <= 0) { throw std::logic_error("Vector<T>::pop : no elements left"); }

        _data[--_size].~T();
    }

    /**
     * NOTE: it seems possible that overflow can happen in the 'sz+1' expression
     * of 'push()', but in fact it can not since it requires that 'cap' is 
     * equal to INT_MAX. This in turn can not happen given the way capacities 
     * are calculated. Essentially, all capacities are even, but INT_MAX is 
     * odd.
     */

    // Element access
    const T& back(void) const { return _data[_size - 1]; }
    T& back(void) { return _data[_size - 1]; }

    const T& front(void) const { return _data[0]; }
    T& front(void) { return _data[0]; }

    const T& operator[](SizeType index) const { return _data[index]; }
    T& operator[](SizeType index) { return _data[index]; }

    // Iterators
    using Iterator = T*;
    using ConstIterator = const T*;

    Iterator begin(void) { return _data; }
    ConstIterator cbegin(void) const { return _data; }
    Iterator end(void) { return _data + _size; }
    ConstIterator end(void) const { return _data + _size; }

    // Duplicatation (preferred instead):
    void copy_to(Vector<T>& to) const {
        to.clear();
        to.grow_to(_size);
        for (SizeType i = 0; i < _size; i++) { to[i] = _data[i]; }
    }
    void move_to(Vector<T>& to) {
        to.clear(true);
        to._data = _data, _data = NULL;
        to._size = _size, _size = 0;
        to._capacity = _capacity, _capacity = 0;
    }
};

};

#endif