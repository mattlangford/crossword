#include <iostream>
#include <array>
#include <bitset>

std::vector<std::string> words {
    "hello",
    "aaaa",
    "abcd",
    "defg",
    "qrst",
};

class Lookup {
public:
    Lookup(const std::vector<std::string>& words, size_t max_len) : data_(max_len) {
        for (size_t i = 0; i < words[i].size(); ++i) {
            const std::string& word = words[i];
            if (word.size() >= max_len) continue;
            for (size_t l = 0; l < word.size(); ++l) {
                data_[l][to_index(word[l])].push_back(i);
            }
        }
    }

    const std::vector<size_t>& position(size_t pos, char c) const { return data_.at(pos)[to_index(c)]; }

private:
    static size_t to_index(char c) { return std::tolower(c) - 'a'; }
    std::vector<std::array<std::vector<size_t>, 26>> data_;
};


std::vector<size_t> logical_and(const std::vector<size_t>& lhs, const std::vector<size_t>& rhs) {
    size_t lhs_i = 0;
    size_t rhs_i = 0;

    std::vector<size_t> result;
    while (lhs_i < lhs.size() && rhs_i < rhs.size()) {
        const size_t& l = lhs[lhs_i];
        const size_t& r = rhs[rhs_i];

        if (l == r) {
            result.push_back(l);
        } else if (l < r) {
            lhs_i++;
        } else {
            rhs_i++;
        }
    }
    return result;
}

class Board {
    Board(size_t dim) : dim_(dim), board_(dim_ * dim_, OPEN) {}

    void block(size_t row, size_t col) { board_.at(to_index(row, col)) = BLOCKED; }

private:
    size_t to_index(size_t row, size_t col) const {
        return row * dim_ + col;
    }

    static constexpr char OPEN = ' ';
    static constexpr char BLOCKED = '#';
    size_t dim_;
    std::vector<char> board_;
};


int main() {
    Lookup lookup(words, 5);
    for (const auto& i : lookup.position(0, 'a')) {
        std::cout << i << ": " << words[i] << "\n";
    }
    return 0;
}