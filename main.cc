#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <sstream>
#include <fstream>
#include <random>
#include <set>

#include <filesystem>

constexpr size_t SIZE = 5;

std::vector<std::string> load_words(std::filesystem::path fname) {
    std::vector<std::string> words;
    std::ifstream file(fname);
    std::string word;

    while (file >> word) {
        if (word.size() > 1 && word.size() <= SIZE) {
            words.push_back(word);
        }
    }

    return words;
}

class Lookup {
public:
    Lookup(const std::vector<std::string>& words) {
        for (size_t i = 0; i < words.size(); ++i) {
            const std::string& word = words[i];
            if (word.size() > SIZE) {
                throw std::runtime_error("Word is too long");
            }
            WordsSplitByCharacterAtPosition& entry = by_length_.at(word.size());
            entry.all_words.push_back(i);

            for (size_t l = 0; l < word.size(); ++l) {
                entry.by_char_pos[l][to_index(word[l])].push_back(i);
            }
        }
    }

    std::vector<size_t> words_with_characters_at(const std::vector<std::pair<size_t, char>>& entries, size_t opening) {
        if (entries.empty()) {
            return by_length_[opening].all_words;
        }
        if (entries.size() == 1) {
            return words_with_character_at(entries[0].first, entries[0].second, opening);
        }

        std::vector<size_t> merged = logical_and(
            words_with_character_at(entries[0].first, entries[0].second, opening),
            words_with_character_at(entries[1].first, entries[1].second, opening)
        );
        for (size_t i = 2; i < entries.size(); i ++) {
            if (merged.empty()) {
                return {};
            }
            merged = logical_and(merged, words_with_character_at(entries[i].first, entries[i].second, opening));
        }
        return merged;
    }


private:
    const std::vector<size_t>& words_with_character_at(size_t pos, char c, size_t opening) const {
        return by_length_.at(opening).by_char_pos.at(pos).at(to_index(c));
    }

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

    struct WordsSplitByCharacterAtPosition {
        using WordsSplitByCharacter = std::array<std::vector<size_t>, 26>;
        std::array<WordsSplitByCharacter, SIZE + 1> by_char_pos;
        std::vector<size_t> all_words;
    };
    std::array<WordsSplitByCharacterAtPosition, SIZE + 1> by_length_;
};

class Board {
public:
    static constexpr char OPEN = ' ';
    static constexpr char BLOCKED = '#';

    Board() {
        for (auto& c : board_) c = OPEN;
    }

    void set(size_t row, size_t col, char c) { board_.at(to_index(row, col)) = c; }
    void block(size_t row, size_t col) { set(row, col, BLOCKED); }

    void fill(const std::vector<size_t>& indicies, const std::string& word) {
        if (indicies.size() != word.size()) throw std::runtime_error(std::format("Different lengths {} != {}", indicies.size(), word.size()));

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

        for (size_t r = start_row; r < SIZE; ++r) {
            size_t index = to_index(r, col);
            if (board_[index] == BLOCKED) break;
            word_pair.col_indicies.push_back(index);
        }

        for (size_t c = start_col; c < SIZE; ++c) {
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
        for (size_t row = 0; row < SIZE; ++row) {
            for (size_t col = 0; col < SIZE; ++col) {
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

    std::string word(const std::vector<size_t>& indicies) const {
        std::string s;
        for (const auto& i : indicies) s += board_.at(i);
        return s;
    }

    std::vector<std::pair<size_t, char>> get_characters_at(const std::vector<size_t>& indicies) const {
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
        for (size_t col = 0; col < SIZE; col++) ss << col;
        ss << "\n";
        for (size_t row = 0; row < SIZE; row++) {
            ss << row << "|";
            for (size_t col = 0; col < SIZE; col++) {
                ss << board_.at(to_index(row, col));
            }
            ss << "|\n";
        }
        for (size_t col = 0; col < SIZE; col++) ss << "-";
        ss << "----";
        return ss.str();
    }

private:
    size_t to_index(size_t row, size_t col) const { return row + col * SIZE; }

    std::array<char, SIZE * SIZE> board_;
};

static std::mt19937 gen(42);

template <typename T>
std::set<T>::iterator get_random_iterator(const std::set<T>& set) {
    if (set.empty()) {
        return set.end();
    }

    std::uniform_int_distribution<> dist(0, set.size() - 1);

    auto it = set.begin();
    std::advance(it, dist(gen));
    return it;
}

bool rand_bool(double p = 0.5) {
    return std::bernoulli_distribution(p)(gen);
}


int main() {
    std::vector<std::string> words = load_words("/Users/mattlangford/Downloads/google-10000-english-usa.txt");
    std::cout << "Loaded " << words.size() << " words\n";
    
    /*
        "noun",
        "house",
        "owner",
        "macro",
        "eyes",
        "home",
        "noway",
        "ounce",
        "users",
        "nero"
    */

    const std::set<std::string> word_set{words.begin(), words.end()};

std::cerr << "1\n";
    Lookup lookup(words);

    Board b;
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
        std::cout << "\n\n" << (r ? "ROW" : "COL") << ": ";
        for (const auto i : *it) std::cout << i << " ";
        std::cout << "\n";
        std::cout << board.to_string() << "\n";
        dfs.pop();

        auto next = [&](Board new_board) {
            bool rows = rand_bool();
            auto next = get_random_iterator(rows ? index_sets.rows : index_sets.cols);
            dfs.push({rows, next, std::move(new_board)});
        };

        if (it->size() == 1) {
            next(std::move(board));
            continue;
        }

        const auto positions = board.get_characters_at(*it);
        constexpr size_t MAX_ATTEMPTS = 10;
        size_t attempts = 0;
        for (size_t index : lookup.words_with_characters_at(positions, it->size())) {
            const std::string& candidate = words.at(index);
            Board new_board = board;
            std::cout << "candidate " << candidate << "\n";
            new_board.fill(*it, candidate);
            next(std::move(new_board));

            if (attempts++ > MAX_ATTEMPTS) {
                break;
            }
        }
    }

    return 0;
}