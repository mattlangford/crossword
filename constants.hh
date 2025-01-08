#pragma once
#include <cstddef>

// Size of the board
constexpr size_t DIM = 5;

// Frequently we index based on length, so include one extra (since index 0 isn't used)
constexpr size_t SIZE = DIM + 1;

using WordIndex = uint16_t;