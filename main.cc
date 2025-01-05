#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <sstream>
#include <fstream>
#include <set>

#include <filesystem>

std::vector<std::string> load_words(std::filesystem::path fname) {
    std::vector<std::string> words;
    std::ifstream file(fname);
    std::string word;

    while (file >> word) {
        words.push_back(word);
    }

    return words;

}


class Lookup {
public:
    Lookup(const std::vector<std::string>& words, size_t max_len) : data_(max_len + 1) {
        lengths_.resize(max_len + 1);
        for (size_t i = 0; i < words.size(); ++i) {
            const std::string& word = words[i];
            if (word.size() <= max_len) {
                for (size_t l = 0; l < word.size(); ++l) {
                    data_[l][to_index(word[l])].push_back(i);
                }
            }
            lengths_[word.size()].push_back(i);
        }
    }

    const std::vector<size_t>& position(size_t pos, char c) const { return data_.at(pos)[to_index(c)]; }

    std::vector<size_t> in_positions(const std::vector<std::pair<size_t, char>>& entries, size_t opening) {
        std::vector<size_t> merged = lengths_[opening];
        for (size_t i = 0; i < entries.size(); i ++) {
            if (merged.empty()) {
                return {};
            }
            merged = logical_and(merged, position(entries[i].first, entries[i].second));
        }
        return merged;
    }


private:
    static std::vector<size_t> logical_and(const std::vector<size_t>& lhs, const std::vector<size_t>& rhs) {
        size_t lhs_i = 0;
        size_t rhs_i = 0;

        std::vector<size_t> result;
        while (lhs_i < lhs.size() && rhs_i < rhs.size()) {
            const size_t& l = lhs[lhs_i];
            const size_t& r = rhs[rhs_i];

            if (l == r) {
                result.push_back(l);
                lhs_i++;
            } else if (l < r) {
                lhs_i++;
            } else {
                rhs_i++;
            }
        }
        return result;
    }

    static size_t to_index(char c) { return std::tolower(c) - 'a'; }

    std::vector<std::array<std::vector<size_t>, 26>> data_;
    std::vector<std::vector<size_t>> lengths_;
};

class Board {
public:
    static constexpr char OPEN = ' ';
    static constexpr char BLOCKED = '#';

    Board(size_t dim) : dim_(dim), board_(dim_ * dim_, OPEN) {}

    void set(size_t row, size_t col, char c) { board_.at(to_index(row, col)) = c; }
    void block(size_t row, size_t col) { set(row, col, BLOCKED); }

    void fill(const std::vector<size_t>& indicies, const std::string& word) {
        if (indicies.size() != word.size()) throw std::runtime_error("Differnet lengths");

        for (size_t i = 0; i < indicies.size(); ++i) {
            board_[indicies[i]] = word[i];
        }
    }

    struct WordPair {
        std::vector<size_t> row_indicies;
        std::vector<size_t> col_indicies;
    };

    WordPair word_pairs(const size_t row, const size_t col) const {
        WordPair word_pair;

        int64_t start_row = row;
        for (; start_row >= 0; --start_row) {
            size_t index = to_index(start_row, col);
            if (board_[index] == BLOCKED) break;
        }
        start_row++;

        int64_t start_col = col;
        for (; start_col >= 0; --start_col) {
            size_t index = to_index(row, start_col);
            if (board_[index] == BLOCKED) break;
        }
        start_col++;

        for (size_t r = start_row; r < dim_; ++r) {
            size_t index = to_index(r, col);
            if (board_[index] == BLOCKED) break;
            word_pair.col_indicies.push_back(index);
        }

        for (size_t c = start_col; c < dim_; ++c) {
            size_t index = to_index(row, c);
            if (board_[index] == BLOCKED) break;
            word_pair.row_indicies.push_back(index);
        }
        return word_pair;
    }

    struct IndexSets {
        std::set<std::vector<size_t>> rows;
        std::set<std::vector<size_t>> cols;
    };

    IndexSets index_sets() const {
        IndexSets sets;
        for (size_t row = 0; row < dim_; ++row) {
            for (size_t col = 0; col < dim_; ++col) {
                auto wp = word_pairs(row, col);
                if (!wp.row_indicies.empty()) {
                    sets.rows.emplace(std::move(wp.row_indicies));
                }
                if (!wp.col_indicies.empty()) {
                    sets.cols.emplace(std::move(wp.col_indicies));
                }
            }
        }
        return sets;
    }

    size_t dim() const { return dim_; }

    std::string word(const std::vector<size_t>& indicies) const {
        std::string s;
        for (const auto& i : indicies) s += board_.at(i);
        return s;
    }

    std::vector<std::pair<size_t, char>> get_positions(const std::vector<size_t>& indicies) const {
        std::vector<std::pair<size_t, char>> result;
        std::string w = word(indicies);
        for (size_t i = 0; i < w.size(); ++i) {
            char c = w[i];
            if (c == BLOCKED) throw std::runtime_error("Found blocked!");
            if (c != OPEN) {
                result.push_back(std::make_pair(i, c));
            }
        }
        return result;
    }

    std::string to_string() const {
        std::stringstream ss;
        ss << "  ";
        for (size_t col = 0; col < dim_; col++) ss << col;
        ss << "\n";
        for (size_t row = 0; row < dim_; row++) {
            ss << row << "|";
            for (size_t col = 0; col < dim_; col++) {
                ss << board_.at(to_index(row, col));
            }
            ss << "|\n";
        }
        for (size_t col = 0; col < dim_; col++) ss << "-";
        ss << "----";
        return ss.str();
    }

private:
    size_t to_index(size_t row, size_t col) const { return row + col * dim_; }

    size_t dim_;

    std::vector<char> board_;
};


int main() {
    std::vector<std::string> words = load_words("/tmp/words.txt");

    const std::set<std::string> word_set{words.begin(), words.end()};

    Lookup lookup(words, 5);

    Board b(5);
    b.block(0, 0);
    b.block(4, 4);

    std::cout << b.to_string() << "\n";

    auto index_sets = b.index_sets();

    struct Dfs {
        bool row;
        std::set<std::vector<size_t>>::iterator it;
        Board board;
    };

    std::stack<Dfs> dfs;
    dfs.push(Dfs{false, index_sets.cols.begin(), b});
    while (!dfs.empty()) {
        auto [r, it, board] = std::move(dfs.top());
        // std::cout << "\n\n" << (r ? "ROW" : "COL") << ": ";
        // for (const auto i : *it) std::cout << i << " ";
        // std::cout << "\n";
        // std::cout << board.to_string() << "\n";
        dfs.pop();

        auto next = [&](Board new_board) {
            auto next = std::next(it);
            if (next == index_sets.cols.end()) {
                // std::cout << new_board.to_string() << "\n";
                dfs.push({true, index_sets.rows.begin(), std::move(new_board)});
                return;
            }
            if (next == index_sets.rows.end()) {
                std::cout << "DONE\n";
                std::cout << new_board.to_string() << "\n";
                return;
            }

            dfs.push({r, next, std::move(new_board)});
        };

        if (it->size() == 1) {
            next(std::move(board));
            continue;
        }

        const auto positions = board.get_positions(*it);
        for (size_t index : lookup.in_positions(positions, it->size())) {
            const std::string& candidate = words.at(index);
            Board new_board = board;
            new_board.fill(*it, candidate);
            next(std::move(new_board));
        }
    }

    return 0;
}