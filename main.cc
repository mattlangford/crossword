#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <sstream>
#include <fstream>
#include <random>
#include <set>
#include <list>

#include <filesystem>

constexpr size_t DIM = 5;
constexpr size_t SIZE = DIM + 1;

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

std::vector<std::string> load_words(std::filesystem::path fname) {
    std::vector<std::string> words;
    std::ifstream file(fname);
    std::string word;

    while (file >> word) {
        if (word.size() > 1 && word.size() < SIZE) {
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

    std::vector<size_t> words_with_characters_at(const FlatVector<std::pair<size_t, char>, SIZE>& entries, size_t opening) {
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
        std::array<WordsSplitByCharacter, SIZE> by_char_pos;
        std::vector<size_t> all_words;
    };
    std::array<WordsSplitByCharacterAtPosition, SIZE> by_length_;
};

class Board {
public:
    static constexpr char OPEN = ' ';
    static constexpr char BLOCKED = '#';

    Board() {
        for (auto& c : board_) c = OPEN;
    }

    void set_index(size_t index, char c) { board_.at(index) = c; }
    void set(size_t row, size_t col, char c) { set_index(to_index(row, col), c); }
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

        for (size_t r = start_row; r < DIM; ++r) {
            size_t index = to_index(r, col);
            if (board_[index] == BLOCKED) break;
            word_pair.col_indicies.push_back(index);
        }

        for (size_t c = start_col; c < DIM; ++c) {
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
        for (size_t col = 0; col < DIM; ++col) {
            for (size_t row = 0; row < DIM; ++row) {
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

    FlatVector<std::pair<size_t, char>, SIZE> get_characters_at(const std::vector<size_t>& indicies) const {
        FlatVector<std::pair<size_t, char>, SIZE> result;
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
        for (size_t col = 0; col < DIM; col++) ss << col;
        ss << "\n";
        for (size_t row = 0; row < DIM; row++) {
            ss << row << "|";
            for (size_t col = 0; col < DIM; col++) {
                ss << board_.at(to_index(row, col));
            }
            ss << "|\n";
        }
        for (size_t col = 0; col < DIM; col++) ss << "-";
        ss << "----";
        return ss.str();
    }

private:
    size_t to_index(size_t row, size_t col) const { return row + col * DIM; }

    std::array<char, DIM * DIM> board_;
};

static std::mt19937 gen(42);

int main() {
    std::vector<std::string> words = load_words("/Users/mattlangford/Downloads/google-10000-english-usa.txt");
    std::cout << "Loaded " << words.size() << " words\n";
    
    const std::set<std::string> word_set{words.begin(), words.end()};

    Lookup lookup(words);

    Board b;
    b.block(0, 0);
    b.block(2, 2);
    b.block(4, 4);

    auto index_sets = b.index_sets();
    std::cout << "row:\n";
    for (auto& e : index_sets.rows) {
        std::cout << " ";
        for (auto i : e) std::cout << i << " ";
        std::cout << "\n";
    }
    std::cout << "\ncol:\n";
    for (auto& e : index_sets.cols) {
        std::cout << " ";
        for (auto i : e) std::cout << i << " ";
        std::cout << "\n";
    }
    std::cout << "\n";

    std::cout << b.to_string() << "\n";

    const size_t total_words = index_sets.rows.size() + index_sets.cols.size();

    std::vector<const std::vector<size_t>*> to_visit;
    for (const auto& e : index_sets.rows) to_visit.push_back(&e);
    for (const auto& e : index_sets.cols) to_visit.push_back(&e);
    std::shuffle(to_visit.begin(), to_visit.end(), gen);

    struct Dfs {
        Board board;
        std::vector<const std::vector<size_t>*> to_visit;
        std::set<size_t> used;
    };

    std::uniform_int_distribution<> start_index_dist(0, 1000);

    size_t boards_checked = 0;
    using Timer = std::chrono::high_resolution_clock;
    const Timer::time_point start = Timer::now();
    Timer::time_point previous = start;

    std::stack<Dfs> dfs;
    dfs.push(Dfs{b, std::move(to_visit), {}});
    while (!dfs.empty()) {
        boards_checked++;

        const Timer::time_point now = Timer::now();
        if (now - previous > std::chrono::seconds(1)) {
            std::cout << std::format("Tested {} boards at {:.2f}/s\n", boards_checked,
                boards_checked / std::chrono::duration_cast<std::chrono::duration<double>>(now - start).count());
            previous = now;
        }

        auto [board, to_visit, used] = std::move(dfs.top());
        dfs.pop();

        //std::cout << "\n";
        //std::cout << board.to_string() << "\n";
        //if (!to_visit.empty()) {
        //    for (const auto i : to_visit.back()) std::cout << i << " ";
        //}
        //std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (to_visit.empty()) {
            std::cout << "DONE\n";
            std::cout << board.to_string() << "\n";
            continue;
        }

        const std::vector<size_t>& indicies = *to_visit.back();

        if (indicies.size() == 1) {
            dfs.push({std::move(board), std::move(to_visit), std::move(used)});
            continue;
        }

        const auto positions = board.get_characters_at(indicies);
        const auto candidates = lookup.words_with_characters_at(positions, indicies.size());
        size_t start_index = start_index_dist(gen);
        for (size_t i = 0; i < candidates.size(); ++i) {
            const size_t index = candidates[(start_index + i) % candidates.size()];
            if (used.contains(index)) {
                continue;
            }

            auto next_to_visit = to_visit;
            next_to_visit.pop_back();

            const std::string& candidate = words.at(index);
            auto new_used = used;
            new_used.insert(index);

            Board new_board = board;
            new_board.fill(indicies, candidate);
            dfs.push({std::move(new_board), std::move(next_to_visit), std::move(new_used)});
        }
    }

    return 0;
}