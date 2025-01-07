#pragma once

#include <array>
#include <cstddef>

template <class T, size_t N>
class FlatVector {
public:
    void push_back(T t) { data_[size_++] = t; }
    T& operator[](const size_t i) { return data_[i]; }
    const T& operator[](const size_t i) const { return data_[i]; }
    T& at(const size_t i) { if (i >= size_) throw std::out_of_range("i >= size_"); return data_[i]; }
    const T& at(const size_t i) const { if (i >= size_) throw std::out_of_range("i >= size_"); return data_[i]; }
    bool empty() const { return size_ == 0; }
    size_t size() const { return size_; }

private:
    size_t size_ = 0;
    std::array<T, N> data_;
};

