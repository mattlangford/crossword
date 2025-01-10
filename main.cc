#include <iostream>
#include <vector>
#include <string>
#include <format>
#include <array>
#include <sstream>
#include <map>
#include <random>
#include <set>
#include <fstream>

#include <filesystem>

#include "flat_vector.hh"
#include "lookup.hh"
#include "constants.hh"

class Board {
public:
    static constexpr char OPEN = ' ';
    static constexpr char BLOCKED = '#';

    using Index = uint16_t;

    Board() {
        for (auto& c : board_) c = OPEN;
    }

    void set_index(Index index, char c) { board_.at(index) = c; }
    void set(uint8_t row, uint8_t col, char c) { set_index(to_index(row, col), c); }
    void block(uint8_t row, uint8_t col) { set(row, col, BLOCKED); }
    char at_index(Index index) const { return board_.at(index); }
    char at(uint8_t row, uint8_t col) const { return at_index(to_index(row, col)); }
    void reset_nonblocked_to_open() {
        for (auto& c : board_) if (c != BLOCKED) c = OPEN;
    }

    std::string read(const std::vector<WordIndex>& index) const {
        std::string s;
        s.reserve(index.size());
        for (const WordIndex i : index) {
            s.push_back(board_[i]);
        }
        return s;
    }

    struct WordIndicies {
        std::map<size_t, std::vector<Index>> rows;
        std::map<size_t, std::vector<Index>> cols;
    };

    WordIndicies generate_word_index() const {
        WordIndicies result;

        Board col_index = *this;
        col_index.reset_nonblocked_to_open();
        Board row_index = col_index;
        size_t current_index = 1;

        auto set_if_open = [&](Index index, char c, Board& b) -> bool {
            if (b.at_index(index) != OPEN) {
                return false;
            }
            b.set_index(index, c);
            return true;
        };

        // Iteration order is important!
        for (uint8_t row = 0; row < DIM; ++row) {
            for (uint8_t col = 0; col < DIM; ++col) {
                const Index board_index = to_index(row, col);

                char c = at_index(board_index);
                if (c == Board::BLOCKED) {
                    continue;
                }

                bool used = false;
                if (set_if_open(board_index, current_index, col_index)) {
                    used = true;
                    result.cols[current_index].push_back(board_index);
                    for (uint8_t inner = row + 1; inner < DIM; ++inner) {
                        Index inner_index = to_index(inner, col);

                        if (!set_if_open(inner_index, '-', col_index)) break;
                        result.cols[current_index].push_back(inner_index);
                    }
                }
                if (set_if_open(board_index, current_index, row_index)) {
                    used = true;
                    result.rows[current_index].push_back(board_index);
                    for (uint8_t inner = col + 1; inner < DIM; ++inner) {
                        Index inner_index = to_index(row, inner);

                        if (!set_if_open(inner_index, '-', row_index)) break;
                        result.rows[current_index].push_back(inner_index);
                    }
                }

                if (used) {
                    current_index++;
                }
            }
        }

        return result;
    }

    FlatVector<std::pair<WordIndex, char>, SIZE> get_characters_at(const std::vector<Index>& indicies) const {
        FlatVector<std::pair<WordIndex, char>, SIZE> result;
        for (WordIndex i = 0; i < indicies.size(); ++i) {
            char c = board_[indicies[i]];
            if (c != OPEN) {
                if (c == BLOCKED) throw std::runtime_error("Found blocked!");
                result.push_back(std::make_pair(i, c));
            }
        }
        return result;
    }

    std::string to_string() const {
        std::stringstream ss;
        ss << "  ";
        for (uint8_t col = 0; col < DIM; col++) ss << (int)col;
        ss << "\n";
        for (uint8_t row = 0; row < DIM; row++) {
            ss << (int) row << "|";
            for (uint8_t col = 0; col < DIM; col++) {
                ss << board_.at(to_index(row, col));
            }
            ss << "|\n";
        }
        for (uint8_t col = 0; col < DIM; col++) ss << "-";
        ss << "----";
        return ss.str();
    }

private:
    uint16_t to_index(uint8_t row, uint8_t col) const { return static_cast<uint16_t>(row) + col * DIM; }

    std::array<char, DIM * DIM> board_;
};

void write_ipuz(const Board& final_board, const Board::WordIndicies& index, const std::filesystem::path& output) {
    Board puzzle_board = final_board;
    puzzle_board.reset_nonblocked_to_open();
    for (const auto& [i, e] : index.rows) puzzle_board.set_index(e.front(), i);
    for (const auto& [i, e] : index.cols) puzzle_board.set_index(e.front(), i);

    std::ofstream file(output);
    if (!file.is_open()) throw std::runtime_error(std::format("Unable to open '{}' for writing", output.string()));
    file << "{\n";
    file << "  \"version\": \"http://ipuz.org/v2\",\n";
    file << "  \"kind\": \"http://ipuz.org/crofileword\",\n";
    file << std::format(R"(  "dimensions": {{"width": {}, "height": {}}},)", DIM, DIM);
    file << "\n  \"puzzle\": [\n";
    for (size_t row = 0; row < DIM; ++row) {
        file << "    [";
        for (size_t col = 0; col < DIM; ++col) {
            char c = puzzle_board.at(row, col);
            if (c == Board::BLOCKED) file << "\"#\"";
            else if (c == Board::OPEN) file << 0;
            else file << (size_t)c;
            if (col != DIM - 1) file << ",";
        }
        file << "]";
        if (row != DIM - 1) file << ",";
        file << "\n";
    }
    file << "  ],\n  \"solution\": [\n";
    for (size_t row = 0; row < DIM; ++row) {
        file << "    [";
        for (size_t col = 0; col < DIM; ++col) {
            char c = final_board.at(row, col);
            file << '"' << (char)std::toupper(c) << '"';
            if (col != DIM - 1) file << ",";
        }
        file << "]";
        if (row != DIM - 1) file << ",";
        file << "\n";
    }
    file << "  ],\n  \"clues\": {\n    \"Across\": [\n";
    for (const auto& [i, e] : index.rows) {
        file << "      [" << i << ", \"Clue for '" << final_board.read(e) << "'\"]";
        if (i != index.rows.rbegin()->first) file << ",";
        file << "\n";
    }
    file << "  ],\n  \"Down\": [\n";
    for (const auto& [i, e] : index.cols) {
        file << "      [" << i << ", \"Clue for '" << final_board.read(e) << "'\"]";
        if (i != index.cols.rbegin()->first) file << ",";
        file << "\n";
    }
    file << "    ]\n  }\n}";

    file.close();
}

class DfsHelper {
private:
    struct Dfs {
        Board board;

        uint16_t used_words = 0;
        uint16_t visited = 0;

        union Contained {
            const std::vector<WordIndex>* to_visit;
            WordIndex word;
        };
        std::vector<Contained> contained;
    };
public:
    DfsHelper(Board b, const std::vector<const std::vector<WordIndex>*>& to_visit) {
        Dfs d;
        d.board = b;
        for (const auto& w : to_visit) {
            d.contained.push_back({.to_visit=w});
        }
        data_.push(std::move(d));
    }

    bool done() const { return data_.empty(); }

    const std::vector<WordIndex>* indicies(const Dfs& current) const {
        if (current.visited >= current.contained.size()) return nullptr;
        return current.contained[current.visited].to_visit;
    }

    bool used_word(const Dfs& current, size_t candidate) const {
        for (uint16_t i = 0; i < current.used_words; ++i) {
            if (current.contained[i].word == candidate) return true;
        }
        return false;
    }

    const Board& board(const Dfs& current) const {
        return current.board;
    }

    void push(Dfs current, Board new_board, size_t index) {
        current.board = std::move(new_board);
        current.contained[current.used_words++].word = index;
        current.visited++;
        data_.push(std::move(current));
    }

    std::optional<Dfs> pop() {
        if (data_.empty()) return std::nullopt;
        Dfs next = std::move(data_.top());
        data_.pop();
        return next;
    }

private:
    std::stack<Dfs> data_;
};

static std::mt19937 gen(42);

int main() {
    Lookup lookup("/Users/mattlangford/Downloads/google-10000-english-usa.txt");

    Board b;
    b.block(0, 0);
    b.block(4, 4);
    // b.block(0, 5);
    // b.block(5, 0);

    std::cout << b.to_string() << "\n";
    const auto word_index = b.generate_word_index();

    const size_t total_words = word_index.rows.size() + word_index.cols.size();

    std::uniform_int_distribution<> start_index_dist(0, 1000);
    std::vector<const std::vector<WordIndex>*> to_visit;
    for (const auto& [_, e] : word_index.rows) to_visit.push_back(&e);
    for (const auto& [_, e] : word_index.cols) to_visit.push_back(&e);
    std::shuffle(to_visit.begin(), to_visit.end(), gen);

    DfsHelper dfs_helper(b, to_visit);

    size_t boards_checked = 0;
    using Timer = std::chrono::high_resolution_clock;
    const Timer::time_point start = Timer::now();
    Timer::time_point previous = start;

    while (auto current = dfs_helper.pop()) {
        boards_checked++;

        const Timer::time_point now = Timer::now();
        if (now - previous > std::chrono::seconds(5)) {
            std::cout << std::format("Tested {} boards at {:.2f}/ms\n", boards_checked,
                boards_checked / std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(now - start).count());
            previous = now;
        }

        const std::vector<WordIndex>* indicies = dfs_helper.indicies(*current);

        // std::cout << "\n";
        // std::cout << dfs_helper.board(*current).to_string() << "\n";
        // std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (indicies == nullptr) {
            std::cout << "\nDONE\n";
            std::cout << dfs_helper.board(*current).to_string() << "\n";
            std::filesystem::path path = std::format("/tmp/crossword_{}x{}_{}.ipuz", DIM, DIM, boards_checked);
            write_ipuz(dfs_helper.board(*current), word_index, path);
            std::cout << "Wrote data to " << path.string() << "\n\n";
            dfs_helper.pop();
            continue;
        }

        const auto positions = dfs_helper.board(*current).get_characters_at(*indicies);
        const auto& candidates = lookup.words_with_characters_at(positions, indicies->size());
        size_t start_index = start_index_dist(gen);
        for (size_t i = 0; i < candidates.size(); ++i) {
            const size_t index = candidates[(start_index + i) % candidates.size()];
            if (dfs_helper.used_word(*current, index)) {
                continue;
            }

            const std::string& candidate = lookup.word(index);
            Board new_board = dfs_helper.board(*current);
            if (indicies->size() != candidate.size()) throw std::runtime_error("Invalid candidate length");
            for (size_t j = 0; j < indicies->size(); ++j) {
                new_board.set_index((*indicies)[j], candidate[j]);
            }

            if (i == candidates.size() - 1) {
                dfs_helper.push(std::move(*current), new_board, index);
            } else {
                dfs_helper.push(*current, new_board, index);
            }
        }
    }

    return 0;
}