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

    const T& front() const { return data_[0]; }
    const T& back() const { return data_[size_ - 1]; }

    T* begin() { return data_.begin(); }
    T* end() { return data_.begin() + size_; }
    const T* begin() const { return data_.begin(); }
    const T* end() const { return data_.begin() + size_; }

    auto operator<=>(const FlatVector<T, N>& rhs) const = default;
    bool operator==(const FlatVector<T, N>& rhs) const {
        if (size() != rhs.size()) return false;
        for (size_t i = 0; i < size(); ++i) {
            if (data_[i] != rhs[i]) return false;
        }
        return true;
    }

private:
    size_t size_ = 0;
    std::array<T, N> data_;
};

